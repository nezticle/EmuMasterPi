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

#include "mapper012.h"
#include "disk.h"
#include "ppu.h"

static u32  vb0, vb1;
static u8	reg[8];
static u8	prg0, prg1;
static u8	chr01, chr23, chr4, chr5, chr6, chr7;

static u8	irq_enable;
static u8	irq_counter;
static u8	irq_latch;
static u8	irq_request;
static u8	irq_preset;
static u8	irq_preset_vbl;

static void updateCpuBanks()
{
	if (reg[0] & 0x40)
		nesSetRom8KBanks(nesRomSize8KB-2, prg1, prg0, nesRomSize8KB-1);
	else
		nesSetRom8KBanks(prg0, prg1, nesRomSize8KB-2, nesRomSize8KB-1);
}

static void updatePpuBanks()
{
	if (nesVromSize1KB) {
		if (reg[0] & 0x80) {
			nesSetVrom1KBank(4, vb1 + chr01);
			nesSetVrom1KBank(5, vb1 + chr01+1);
			nesSetVrom1KBank(6, vb1 + chr23);
			nesSetVrom1KBank(7, vb1 + chr23+1);
			nesSetVrom1KBank(0, vb0 + chr4);
			nesSetVrom1KBank(1, vb0 + chr5);
			nesSetVrom1KBank(2, vb0 + chr6);
			nesSetVrom1KBank(3, vb0 + chr7);
		} else {
			nesSetVrom1KBank(0, vb0 + chr01);
			nesSetVrom1KBank(1, vb0 + chr01+1);
			nesSetVrom1KBank(2, vb0 + chr23);
			nesSetVrom1KBank(3, vb0 + chr23+1);
			nesSetVrom1KBank(4, vb1 + chr4);
			nesSetVrom1KBank(5, vb1 + chr5);
			nesSetVrom1KBank(6, vb1 + chr6);
			nesSetVrom1KBank(7, vb1 + chr7);
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

static u8 readLow(u16 addr)
{
	Q_UNUSED(addr)
	return 0x01;
}

static void writeLow(u16 addr, u8 data)
{
	if (addr > 0x4100 && addr < 0x6000) {
		vb0 = (data&0x01)<<8;
		vb1 = (data&0x10)<<4;
		updatePpuBanks();
	} else {
		nesDefaultCpuWriteLow(addr, data);
	}
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
		irq_latch = data;
		break;
	case 0xc001:
		reg[5] = data;
		if (nesPpuScanline < NesPpu::VisibleScreenHeight) {
			irq_counter |= 0x80;
			irq_preset = 0xff;
		} else {
			irq_counter |= 0x80;
			irq_preset_vbl = 0xff;
			irq_preset = 0;
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
	if (nesPpuScanline < NesPpu::VisibleScreenHeight && nesPpuIsDisplayOn()) {
		if (irq_preset_vbl) {
			irq_counter = irq_latch;
			irq_preset_vbl = 0;
		}
		if (irq_preset) {
			irq_counter = irq_latch;
			irq_preset = 0;
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

void Mapper012::reset()
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

	vb0 = 0;
	vb1 = 0;
	chr01 = 0;
	chr23 = 2;
	chr4  = 4;
	chr5  = 5;
	chr6  = 6;
	chr7  = 7;
	updatePpuBanks();
}

void Mapper012::extSl()
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
	emsl.var("vb0", vb0);
	emsl.var("vb1", vb1);
}
