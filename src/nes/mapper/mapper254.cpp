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

#include "mapper254.h"
#include "disk.h"
#include "ppu.h"

static u8 reg[8];
static u8 prg0, prg1;
static u8 chr01, chr23, chr4, chr5, chr6, chr7;

static u8 irq_type;
static u8 irq_enable;
static u8 irq_counter;
static u8 irq_latch;
static u8 irq_request;
static u8 protectflag;

static void setBankCpu()
{
	if (reg[0] & 0x40)
		nesSetRom8KBanks(nesRomSize8KB-2, prg1, prg0, nesRomSize8KB-1);
	else
		nesSetRom8KBanks(prg0, prg1, nesRomSize8KB-2, nesRomSize8KB-1);
}

static void setBankPpu()
{
	if (nesVromSize1KB) {
		if (reg[0] & 0x80) {
			nesSetVrom1KBank(4, (chr01+0));
			nesSetVrom1KBank(5, (chr01+1));
			nesSetVrom1KBank(6, (chr23+0));
			nesSetVrom1KBank(7, (chr23+1));
			nesSetVrom1KBank(0, chr4);
			nesSetVrom1KBank(1, chr5);
			nesSetVrom1KBank(2, chr6);
			nesSetVrom1KBank(3, chr7);
		} else {
			nesSetVrom1KBank(0, (chr01+0));
			nesSetVrom1KBank(1, (chr01+1));
			nesSetVrom1KBank(2, (chr23+0));
			nesSetVrom1KBank(3, (chr23+1));
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

static u8 readLow(u16 addr)
{
	if (addr >= 0x6000) {
		if (protectflag)
			return nesCpuReadDirect(addr);
		else
			return nesCpuReadDirect(addr) ^ 0x01;
	}
	return nesDefaultCpuReadLow(addr);
}

static void writeLow(u16 addr, u8 data)
{
	switch (addr & 0xF000) {
	case 0x6000:
	case 0x7000:
		nesCpuWriteDirect(addr, data);
		break;
	}
}

static void writeHigh(u16 addr, u8 data)
{
	switch (addr & 0xE001) {
	case 0x8000:
		protectflag = 0xFF;
		reg[0] = data;
		setBankCpu();
		setBankPpu();
		break;
	case 0x8001:
		reg[1] = data;

		switch (reg[0] & 0x07) {
		case 0x00: chr01 = data & 0xFE; setBankPpu(); break;
		case 0x01: chr23 = data & 0xFE; setBankPpu(); break;
		case 0x02: chr4 = data; setBankPpu(); break;
		case 0x03: chr5 = data; setBankPpu(); break;
		case 0x04: chr6 = data; setBankPpu(); break;
		case 0x05: chr7 = data; setBankPpu(); break;
		case 0x06: prg0 = data; setBankCpu(); break;
		case 0x07: prg1 = data; setBankCpu(); break;
		}
		break;
	case 0xA000:
		reg[2] = data;
		if (nesMirroring != FourScreenMirroring)
			nesSetMirroring(static_cast<NesMirroring>(data & 0x01));
		break;
	case 0xA001:
		reg[3] = data;
		break;
	case 0xC000:
		reg[4] = data;
		irq_counter = data;
		irq_request = 0;
		nesMapperSetIrqSignalOut(false);
		break;
	case 0xC001:
		reg[5] = data;
		irq_latch = data;
		irq_request = 0;
		nesMapperSetIrqSignalOut(false);
		break;
	case 0xE000:
		reg[6] = data;
		irq_enable = 0;
		irq_request = 0;
		nesMapperSetIrqSignalOut(false);
		break;
	case 0xE001:
		reg[7] = data;
		irq_enable = 1;
		irq_request = 0;
		nesMapperSetIrqSignalOut(false);
		break;
	}
}

static void horizontalSync()
{
	if (nesPpuScanline < NesPpu::VisibleScreenHeight && nesPpuIsDisplayOn()) {
		if (irq_enable && !irq_request) {
			if (nesPpuScanline == 0) {
				if (irq_counter)
					irq_counter--;
			}
			if (!(irq_counter--)) {
				irq_request = 0xFF;
				irq_counter = irq_latch;
				nesMapperSetIrqSignalOut(true);
			}
		}
	}
}

void Mapper254::reset()
{
	NesMapper::reset();
	readLow = ::readLow;
	writeLow = ::writeLow;
	writeHigh = ::writeHigh;
	horizontalSync = ::horizontalSync;

	for (int i = 0; i < 8; i++)
		reg[i] = 0x00;

	protectflag = 0;

	prg0 = 0;
	prg1 = 1;
	setBankCpu();

	chr01 = 0;
	chr23 = 2;
	chr4  = 4;
	chr5  = 5;
	chr6  = 6;
	chr7  = 7;
	setBankPpu();

	irq_enable = 0;	// Disable
	irq_counter = 0;
	irq_latch = 0;
	irq_request = 0;
}

void Mapper254::extSl()
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
	emsl.var("irq_type", irq_type);
	emsl.var("irq_enable", irq_enable);
	emsl.var("irq_counter", irq_counter);
	emsl.var("irq_latch", irq_latch);
	emsl.var("irq_request", irq_request);
	emsl.var("protectflag", protectflag);
}
