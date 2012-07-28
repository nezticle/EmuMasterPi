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

#include "mapper019.h"
#include "disk.h"

static u8 patch;
static u8 exsound_enable;

static u8 reg[3];
static u8 exram[128];

static u8 irq_enable;
static u16 irq_counter;

static u8 readLow(u16 addr)
{
	u8 data = 0;
	switch (addr & 0xF800) {
	case 0x4800:
		if (addr == 0x4800) {
			if (exsound_enable) {
				// TODO exsound nes->apu->ExRead(address);
				data = exram[reg[2]&0x7F];
			} else {
				data = nesWram[reg[2]&0x7F];
			}
			if (reg[2]&0x80)
				reg[2] = (reg[2]+1)|0x80;
		}
		break;
	case 0x5000:
		data = irq_counter & 0x00FF;
		break;
	case 0x5800:
		data = (irq_counter>>8) & 0x7F;
		break;
	case 0x6000:
	case 0x6800:
	case 0x7000:
	case 0x7800:
		data = nesDefaultCpuReadLow(addr);
		break;
	default:
		data = addr >> 8;
		break;
	}
	return data;
}

static void writeLow(u16 addr, u8 data)
{
	switch (addr & 0xF800) {
	case 0x4800:
		if (addr == 0x4800) {
			if (exsound_enable) {
				// TODO exsound nes->apu->ExWrite(address, data);
				exram[reg[2]&0x7F] = data;
			} else {
				nesWram[reg[2]&0x7F] = data;
			}
			if (reg[2]&0x80)
				reg[2] = (reg[2]+1)|0x80;
		}
		break;
	case 0x5000:
		irq_counter = (irq_counter & 0xFF00) | data;
		nesMapperSetIrqSignalOut(false);
		break;
	case 0x5800:
		irq_counter = (irq_counter & 0x00FF) | ((data & 0x7F) << 8);
		irq_enable  = data & 0x80;
		nesMapperSetIrqSignalOut(false);
		break;
	case 0x6000:
	case 0x6800:
	case 0x7000:
	case 0x7800:
		nesDefaultCpuWriteLow(addr, data);
		break;
	default:
		break;
	}
}

static void writeHigh(u16 addr, u8 data)
{
	switch (addr & 0xF800) {
	case 0x8000:
		if ((data < 0xE0) || (reg[0] != 0)) {
			nesSetVrom1KBank(0, data);
		} else {
			nesSetCram1KBank(0, data&0x1F);
		}
		break;
	case 0x8800:
		if ((data < 0xE0) || (reg[0] != 0)) {
			nesSetVrom1KBank(1, data);
		} else {
			nesSetCram1KBank(1, data&0x1F);
		}
		break;
	case 0x9000:
		if ((data < 0xE0) || (reg[0] != 0)) {
			nesSetVrom1KBank(2, data);
		} else {
			nesSetCram1KBank(2, data&0x1F);
		}
		break;
	case 0x9800:
		if ((data < 0xE0) || (reg[0] != 0)) {
			nesSetVrom1KBank(3, data);
		} else {
			nesSetCram1KBank(3, data&0x1F);
		}
		break;
	case 0xA000:
		if ((data < 0xE0) || (reg[1] != 0)) {
			nesSetVrom1KBank(4, data);
		} else {
			nesSetCram1KBank(4, data&0x1F);
		}
		break;
	case 0xA800:
		if ((data < 0xE0) || (reg[1] != 0)) {
			nesSetVrom1KBank(5, data);
		} else {
			nesSetCram1KBank(5, data&0x1F);
		}
		break;
	case 0xB000:
		if ((data < 0xE0) || (reg[1] != 0)) {
			nesSetVrom1KBank(6, data);
		} else {
			nesSetCram1KBank(6, data&0x1F);
		}
		break;
	case 0xB800:
		if ((data < 0xE0) || (reg[1] != 0)) {
			nesSetVrom1KBank(7, data);
		} else {
			nesSetCram1KBank(7, data&0x1F);
		}
		break;
	case 0xC000:
		if (!patch) {
			if (data <= 0xDF) {
				nesSetVrom1KBank(8, data);
			} else {
				nesSetVram1KBank(8, data & 0x01);
			}
		}
		break;
	case 0xC800:
		if (!patch) {
			if (data <= 0xDF) {
				nesSetVrom1KBank(9, data);
			} else {
				nesSetVram1KBank(9, data & 0x01);
			}
		}
		break;
	case 0xD000:
		if (!patch) {
			if (data <= 0xDF) {
				nesSetVrom1KBank(10, data);
			} else {
				nesSetVram1KBank(10, data & 0x01);
			}
		}
		break;
	case 0xD800:
		if (!patch) {
			if (data <= 0xDF) {
				nesSetVrom1KBank(11, data);
			} else {
				nesSetVram1KBank(11, data & 0x01);
			}
		}
		break;
	case 0xE000:
		nesSetRom8KBank(4, data & 0x3F);
		if (patch == 2) {
			if (data & 0x40)
				nesSetMirroring(VerticalMirroring);
			else
				nesSetMirroring(SingleLow);
		}
		if (patch == 3) {
			if (data & 0x80)
				nesSetMirroring(HorizontalMirroring);
			else
				nesSetMirroring(VerticalMirroring);
		}
		break;
	case 0xE800:
		reg[0] = data & 0x40;
		reg[1] = data & 0x80;
		nesSetRom8KBank(5, data & 0x3F);
		break;
	case 0xF000:
		nesSetRom8KBank(6, data & 0x3F);
		break;
	case 0xF800:
		if (addr == 0xF800) {
			if (exsound_enable) {
				// TODO exsound nes->apu->ExWrite( address, data);
			}
			reg[2] = data;
		}
		break;
	default:
		break;
	}
}

static void clock(int cycles)
{
	if (irq_enable) {
		if ((irq_counter+=cycles) >= 0x7fff) {
			irq_enable  = 0;
			irq_counter = 0x7fff;
			nesMapperSetIrqSignalOut(true);
		}
	}
}

void Mapper019::reset()
{
	NesMapper::reset();
	readLow = ::readLow;
	writeLow = ::writeLow;
	writeHigh = ::writeHigh;
	clock = ::clock;

	patch = 0;

	reg[0] = reg[1] = reg[2] = 0;

	memset(exram, 0, sizeof(exram));

	irq_enable = 0;
	irq_counter = 0;

	nesSetRom8KBanks(0, 1, nesRomSize8KB-2, nesRomSize8KB-1);

	if (nesVromSize1KB)
		nesSetVrom8KBank(nesVromSize8KB-1);

	exsound_enable = 0xFF;

	u32 crc = nesDiskCrc;

	if (crc == 0xb62a7b71) {	// Family Circuit '91(J)
		patch = 1;
	}

	if (crc == 0x02738c68) {	// Wagan Land 2(J)
		patch = 3;
	}
	if (crc == 0x14942c06) {	// Wagan Land 3(J)
		patch = 2;
	}

	if (crc == 0x968dcf09) {	// Final Lap(J)
		nesEmuSetRenderMethod(NesEmu::PreAllRender);
	}
	if (crc == 0x3deac303) {	// Rolling Thunder(J)
		nesEmuSetRenderMethod(NesEmu::PostAllRender);
	}

	if (crc == 0xb1b9e187) {	// For Kaijuu Monogatari(J)
		nesEmuSetRenderMethod(NesEmu::PreAllRender);
	}

	if (crc == 0x6901346e) {	// For Sangokushi 2 - Haou no Tairiku(J)
		nesEmuSetRenderMethod(NesEmu::TileRender);
	}

	if (crc == 0xaf15338f		// For Mindseeker(J)
	 || crc == 0xb1b9e187		// For Kaijuu Monogatari(J)
	 || crc == 0x96533999		// Dokuganryuu Masamune(J)
	 || crc == 0x3296ff7a		// Battle Fleet(J)
	 || crc == 0xdd454208) {	// Hydlide 3(J)
		exsound_enable = 0;
	}

	if (crc == 0x429fd177) {	// Famista '90(J)
		exsound_enable = 0;
	}

	if (exsound_enable) {
		// TODO exsound nes->apu->SelectExSound( 0x10);
	}
}

void Mapper019::extSl()
{
	emsl.array("reg", reg, sizeof(reg));
	emsl.array("exram", exram, sizeof(exram));
	emsl.var("irq_enable", irq_enable);
	emsl.var("irq_counter", irq_counter);
}
