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

#include "mapper047.h"
#include "disk.h"
#include "ppu.h"

static u8 reg[8];
static u8 patch;
static u8 bank;
static u8 prg0, prg1;
static u8 chr01,chr23,chr4,chr5,chr6,chr7;
static u8 irq_enable;
static u8 irq_counter;
static u8 irq_latch;

static void setBankCpu()
{
	if (reg[0] & 0x40) {
		nesSetRom8KBank(4, bank * 8 + ((patch && bank != 2)?6:14));
		nesSetRom8KBank(5, bank * 8 + prg1);
		nesSetRom8KBank(6, bank * 8 + prg0);
		nesSetRom8KBank(7, bank * 8 + ((patch && bank != 2)?7:15));
	} else {
		nesSetRom8KBank(4, bank * 8 + prg0);
		nesSetRom8KBank(5, bank * 8 + prg1);
		nesSetRom8KBank(6, bank * 8 + ((patch && bank != 2)?6:14));
		nesSetRom8KBank(7, bank * 8 + ((patch && bank != 2)?7:15));
	}
}

static void setBankPpu()
{
	if (nesVromSize1KB) {
		if (reg[0] & 0x80) {
			nesSetVrom1KBank(0, (bank & 0x02) * 64 + chr4);
			nesSetVrom1KBank(1, (bank & 0x02) * 64 + chr5);
			nesSetVrom1KBank(2, (bank & 0x02) * 64 + chr6);
			nesSetVrom1KBank(3, (bank & 0x02) * 64 + chr7);
			nesSetVrom1KBank(4, (bank & 0x02) * 64 + chr01 + 0);
			nesSetVrom1KBank(5, (bank & 0x02) * 64 + chr01 + 1);
			nesSetVrom1KBank(6, (bank & 0x02) * 64 + chr23 + 0);
			nesSetVrom1KBank(7, (bank & 0x02) * 64 + chr23 + 1);
		} else {
			nesSetVrom1KBank(0, (bank & 0x02) * 64 + chr01 + 0);
			nesSetVrom1KBank(1, (bank & 0x02) * 64 + chr01 + 1);
			nesSetVrom1KBank(2, (bank & 0x02) * 64 + chr23 + 0);
			nesSetVrom1KBank(3, (bank & 0x02) * 64 + chr23 + 1);
			nesSetVrom1KBank(4, (bank & 0x02) * 64 + chr4);
			nesSetVrom1KBank(5, (bank & 0x02) * 64 + chr5);
			nesSetVrom1KBank(6, (bank & 0x02) * 64 + chr6);
			nesSetVrom1KBank(7, (bank & 0x02) * 64 + chr7);
		}
	}
}

static void writeLow(u16 addr, u8 data)
{
	if (addr == 0x6000) {
		if (patch) {
			bank = (data & 0x06) >> 1;
		} else {
			bank = (data & 0x01) << 1;
		}
		setBankCpu();
		setBankPpu();
	}
}

static void writeHigh(u16 addr, u8 data)
{
	switch (addr & 0xE001) {
	case 0x8000:
		reg[0] = data;
		setBankCpu();
		setBankPpu();
		break;
	case 0x8001:
		reg[1] = data;
		switch( reg[0] & 0x07) {
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
		bank = data & 0x07;
		if (bank == 7) {
			bank = 6;
		}
		setBankCpu();
		setBankPpu();
		break;
	case 0xC000:
		reg[4] = data;
		irq_counter = data;
		break;
	case 0xC001:
		reg[5] = data;
		irq_latch = data;
		break;
	case 0xE000:
		reg[6] = data;
		irq_enable = 0;
		nesMapperSetIrqSignalOut(false);
		break;
	case 0xE001:
		reg[7] = data;
		irq_enable = 1;
		break;
	}
}

static void horizontalSync()
{
	if (nesPpuScanline < NesPpu::VisibleScreenHeight && nesPpuIsDisplayOn()) {
		if (irq_enable) {
			if (!(--irq_counter)) {
				irq_counter = irq_latch;
				nesMapperSetIrqSignalOut(true);
			}
		}
	}
}

void Mapper047::reset()
{
	NesMapper::reset();
	writeLow = ::writeLow;
	writeHigh = ::writeHigh;
	horizontalSync = ::horizontalSync;

	patch = 0;

	u32 crc = nesDiskCrc;
	if (crc == 0x7eef434c) {
		patch = 1;
	}

	memset(reg, 0, sizeof(reg));

	bank = 0;
	prg0 = 0;
	prg1 = 1;

	// set VROM banks
	if (nesVromSize1KB) {
		chr01 = 0;
		chr23 = 2;
		chr4  = 4;
		chr5  = 5;
		chr6  = 6;
		chr7  = 7;
	} else {
		chr01 = chr23 = chr4 = chr5 = chr6 = chr7 = 0;
	}

	setBankCpu();
	setBankPpu();

	irq_enable = 0;
	irq_counter = 0;
	irq_latch = 0;
}

void Mapper047::extSl()
{
	emsl.array("reg", reg, sizeof(reg));
	emsl.var("bank", bank);
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
}
