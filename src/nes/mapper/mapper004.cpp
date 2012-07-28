/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "mapper004.h"
#include "disk.h"
#include "ppu.h"

enum Irq {
	IrqDefault,
	IrqKlax,
	IrqShougimeikan,
	IrqDai2JiSuper,
	IrqDBZ2,
	IrqRockman3
};

static u8	reg[8];
static u8	prg0, prg1;
static u8	chr01, chr23, chr4, chr5, chr6, chr7;

static u8	irq_type;
static u8	irq_enable;
static u8	irq_counter;
static u8	irq_latch;
static u8	irq_request;
static u8	irq_preset;
static u8	irq_preset_vbl;

static u8	vs_patch;
static u8	vs_index;

static void updatePpuBanks()
{
	if (nesVromSize1KB) {
		if (reg[0] & 0x80) {
			nesSetVrom1KBank(4, chr01);
			nesSetVrom1KBank(5, chr01+1);
			nesSetVrom1KBank(6, chr23);
			nesSetVrom1KBank(7, chr23+1);
			nesSetVrom1KBank(0, chr4);
			nesSetVrom1KBank(1, chr5);
			nesSetVrom1KBank(2, chr6);
			nesSetVrom1KBank(3, chr7);
		} else {
			nesSetVrom1KBank(0, chr01);
			nesSetVrom1KBank(1, chr01+1);
			nesSetVrom1KBank(2, chr23);
			nesSetVrom1KBank(3, chr23+1);
			nesSetVrom1KBank(4, chr4);
			nesSetVrom1KBank(5, chr5);
			nesSetVrom1KBank(6, chr6);
			nesSetVrom1KBank(7, chr7);
		}
	} else {
		if (reg[0] & 0x80) {
			nesSetCram1KBank(4, (chr01+0)&0x07);
			nesSetCram1KBank(5, (chr01+1)&0x07);
			nesSetCram1KBank(6, (chr23+0)&0x07);
			nesSetCram1KBank(7, (chr23+1)&0x07);
			nesSetCram1KBank(0, chr4&0x07);
			nesSetCram1KBank(1, chr5&0x07);
			nesSetCram1KBank(2, chr6&0x07);
			nesSetCram1KBank(3, chr7&0x07);
		} else {
			nesSetCram1KBank(0, (chr01+0)&0x07);
			nesSetCram1KBank(1, (chr01+1)&0x07);
			nesSetCram1KBank(2, (chr23+0)&0x07);
			nesSetCram1KBank(3, (chr23+1)&0x07);
			nesSetCram1KBank(4, chr4&0x07);
			nesSetCram1KBank(5, chr5&0x07);
			nesSetCram1KBank(6, chr6&0x07);
			nesSetCram1KBank(7, chr7&0x07);
		}
	}
}

static void updateCpuBanks()
{
	if (reg[0] & 0x40)
		nesSetRom8KBanks(nesRomSize8KB-2, prg1, prg0, nesRomSize8KB-1);
	else
		nesSetRom8KBanks(prg0, prg1, nesRomSize8KB-2, nesRomSize8KB-1);
}

static u8 readLow(u16 addr)
{
	if (!vs_patch) {
		if (addr >= 0x5000 && addr < 0x6000)
			return nesXram[addr - 0x4000];
	} else {
		if (vs_patch == 1) {
			// VS TKO Boxing Security
			if (addr == 0x5e00) {
				vs_index = 0;
				return 0x00;
			} else if (addr == 0x5e01) {
				static u8 vsTKOBoxingSecurity[32] = {
					0xff, 0xbf, 0xb7, 0x97, 0x97, 0x17, 0x57, 0x4f,
					0x6f, 0x6b, 0xeb, 0xa9, 0xb1, 0x90, 0x94, 0x14,
					0x56, 0x4e, 0x6f, 0x6b, 0xeb, 0xa9, 0xb1, 0x90,
					0xd4, 0x5c, 0x3e, 0x26, 0x87, 0x83, 0x13, 0x00
				};
				return vsTKOBoxingSecurity[(vs_index++) & 0x1f];
			}
		} else if (vs_patch == 2) {
			// VS Atari RBI Baseball Security
			if (addr == 0x5e00) {
				vs_index = 0;
				return 0x00;
			} else if (addr == 0x5e01) {
				if (vs_index++ == 9)
					return 0x6f;
				else
					return 0xb4;
			}
		} else if (vs_patch == 3) {
			// VS Super Xevious
			switch (addr) {
			case 0x54ff:
				return 0x05;
			case 0x5678:
				if (vs_index)
					return 0x00;
				else
					return 0x01;
				break;
			case 0x578f:
				if (vs_index)
					return 0xd1;
				else
					return 0x89;
				break;
			case 0x5567:
				if (vs_index) {
					vs_index = 0;
					return 0x3e;
				} else {
					vs_index = 1;
					return 0x37;
				}
				break;
			default:
				break;
			}
		}
	}
	return nesDefaultCpuReadLow(addr);
}

static void writeLow(u16 addr, u8 data)
{
	if (addr >= 0x5000 && addr < 0x6000)
		nesXram[addr - 0x4000] = data;
	else
		nesDefaultCpuWriteLow(addr, data);
}

static void writeHigh(u16 addr, u8 data)
{
	switch (addr & 0xe001) {
	case 0x8000:
		reg[0] = data;
		updateCpuBanks();
		updatePpuBanks();
		break;
	case 0x8001:
		reg[1] = data;
		switch (reg[0] & 0x07) {
		case 0x00: chr01 = data & 0xfe; updatePpuBanks(); break;
		case 0x01: chr23 = data & 0xfe; updatePpuBanks(); break;
		case 0x02: chr4 = data; updatePpuBanks(); break;
		case 0x03: chr5 = data; updatePpuBanks(); break;
		case 0x04: chr6 = data; updatePpuBanks(); break;
		case 0x05: chr7 = data; updatePpuBanks(); break;
		case 0x06: prg0 = data; updateCpuBanks(); break;
		case 0x07: prg1 = data; updateCpuBanks(); break;
		}
		break;
	case 0xa000:
		reg[2] = data;
		if (nesMirroring != FourScreenMirroring) {
			if (data & 0x01)
				nesSetMirroring(HorizontalMirroring);
			else
				nesSetMirroring(VerticalMirroring);
		}
		break;
	case 0xa001:
		reg[3] = data;
		break;
	case 0xc000:
		reg[4] = data;
		if (irq_type == IrqKlax || irq_type == IrqRockman3)
			irq_counter = data;
		else
			irq_latch = data;
		if (irq_type == IrqDBZ2)
			irq_latch = 0x07;
		break;
	case 0xc001:
		reg[5] = data;
		if (irq_type == IrqKlax || irq_type == IrqRockman3) {
			irq_latch = data;
		} else {
			if ((nesPpuScanline < NesPpu::VisibleScreenHeight) || (irq_type == IrqShougimeikan)) {
				irq_counter |= 0x80;
				irq_preset = 0xff;
			} else {
				irq_counter |= 0x80;
				irq_preset_vbl = 0xff;
				irq_preset = 0;
			}
		}
		break;
	case 0xe000:
		reg[6] = data;
		irq_enable = 0;
		irq_request = 0;
		nesMapperSetIrqSignalOut(false);
		break;
	case 0xe001:
		reg[7] = data;
		irq_enable = 1;
		irq_request = 0;
		break;
	}
}

static void horizontalSync()
{
	if (irq_type == IrqKlax) {
		if (nesPpuScanline < NesPpu::VisibleScreenHeight && nesPpuIsDisplayOn()) {
			if (irq_enable) {
				if (irq_counter == 0) {
					irq_counter = irq_latch;
					irq_request = 0xff;
				}
				if (irq_counter > 0)
					irq_counter--;
			}
		}
		if (irq_request)
			nesMapperSetIrqSignalOut(true);
	} else if (irq_type == IrqRockman3) {
		if (nesPpuScanline < NesPpu::VisibleScreenHeight && nesPpuIsDisplayOn()) {
			if (irq_enable) {
				if (!(--irq_counter)) {
					irq_request = 0xff;
					irq_counter = irq_latch;
				}
			}
		}
		if (irq_request)
			nesMapperSetIrqSignalOut(true);
	} else {
		if (nesPpuScanline < NesPpu::VisibleScreenHeight && nesPpuIsDisplayOn()) {
			if (irq_preset_vbl) {
				irq_counter = irq_latch;
				irq_preset_vbl = 0;
			}
			if (irq_preset) {
				irq_counter = irq_latch;
				irq_preset = 0;
				if (irq_type == IrqDai2JiSuper && nesPpuScanline == 0) {
					irq_counter--;
				}
			} else if (irq_counter > 0) {
				irq_counter--;
			}
			if (irq_counter == 0) {
				if (irq_enable) {
					irq_request = 0xff;
					nesMapperSetIrqSignalOut(true);
				}
				irq_preset = 0xff;
			}
		}
	}
}

void Mapper004::reset()
{
	NesMapper::reset();
	readLow = ::readLow;
	writeLow = ::writeLow;
	writeHigh = ::writeHigh;
	horizontalSync = ::horizontalSync;

	memset(reg, 0, sizeof(reg));
	prg0 = 0;
	prg1 = 1;
	updateCpuBanks();

	irq_enable = 0;	// Disable
	irq_counter = 0;
	irq_latch = 0xff;
	irq_request = 0;
	irq_preset = 0;
	irq_preset_vbl = 0;

	irq_type = IrqDefault;

	chr01 = 0;
	chr23 = 2;
	chr4  = 4;
	chr5  = 5;
	chr6  = 6;
	chr7  = 7;
	updatePpuBanks();

	u32 crc = nesDiskCrc;

	if (crc == 0x5c707ac4 || // Mother(J)
		crc == 0xcb106f49 || // F-1 Sensation(J)
		crc == 0x14a01c70 || // Gun-Dec(J)
		crc == 0xc17ae2dc || // God Slayer - Haruka Tenkuu no Sonata(J)
		crc == 0xe19a2473 || // Sugoro Quest - Dice no Senshi Tachi(J)
		crc == 0xa9a0d729 || // Dai Kaijuu - Deburas(J)
		crc == 0xd852c2f7 || // Time Zone(J)
		crc == 0xecfd3c69 || // Taito Chase H.Q.(J)
		crc == 0xaafe699c || // Ninja Ryukenden 3 - Yomi no Hakobune(J)
		crc == 0x6cc62c06 || // Hoshi no Kirby - Yume no Izumi no Monogatari(J)
		crc == 0x8685f366 || // Matendouji(J)
		crc == 0x8635fed1 || // Mickey Mouse 3 - Yume Fuusen(J)
		crc == 0x7c7ab58e || // Walkuere no Bouken - Toki no Kagi Densetsu(J)
		crc == 0x26ff3ea2 || // Yume Penguin Monogatari(J)
		crc == 0x126ea4a0 || // Summer Carnival '92 Recca(J)
		crc == 0xA67EA466) { // Alien 3(U)
		nesEmuSetRenderMethod(NesEmu::TileRender);
	}
	if (crc == 0xeffeea40) {	// For Klax(J)
		irq_type = IrqKlax;
		nesEmuSetRenderMethod(NesEmu::TileRender);
	}
	if (crc == 0x5a6860f1 || // Shougi Meikan '92(J)
		crc == 0xae280e20) { // Shougi Meikan '93(J)
		irq_type = IrqShougimeikan;
	}
	if (crc == 0xc5fea9f2) {	// Dai 2 Ji - Super Robot Taisen(J)
		irq_type = IrqDai2JiSuper;
	}

	if (crc == 0x1d2e5018		// Rockman 3(J)
	 || crc == 0x6b999aaf		// Megaman 3(U)
	 || crc == 0xd88d48d7) {	// Kick Master(U)
		irq_type = IrqRockman3;
	}

	if (crc == 0xe763891b) {	// DBZ2
		irq_type = IrqDBZ2;
	}

	// VS-Unisystem
	vs_patch = 0;
	vs_index = 0;

	if (crc == 0xeb2dba63		// VS TKO Boxing
	 || crc == 0x98cfe016) {	// VS TKO Boxing (Alt)
		vs_patch = 1;
	}
	if (crc == 0x135adf7c) {	// VS Atari RBI Baseball
		vs_patch = 2;
	}
	if (crc == 0xf9d3b0a3		// VS Super Xevious
	 || crc == 0x9924980a		// VS Super Xevious (b1)
	 || crc == 0x66bb838f) {	// VS Super Xevious (b2)
		vs_patch = 3;
	}
}

void Mapper004::extSl()
{
	emsl.array("reg", reg, sizeof(reg));
	emsl.var("prg0", prg0);
	emsl.var("prg1", prg1);
	emsl.var("chr01", chr01);
	emsl.var("chr23", chr23);
	emsl.var("chr4", chr4);
	emsl.var("chr5", chr5);
	emsl.var("chr6", chr6);
	emsl.var("chr7", chr7);
	emsl.var("irq_enable", irq_enable);
	emsl.var("irq_counter", irq_counter);
	emsl.var("irq_latch", irq_latch);
	emsl.var("irq_request", irq_request);
	emsl.var("irq_preset", irq_preset);
	emsl.var("irq_preset_vbl", irq_preset_vbl);
}
