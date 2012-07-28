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

#include "mapper045.h"
#include "disk.h"
#include "ppu.h"

static u8 patch;
static u8 reg[8];
static u8 prg0, prg1, prg2, prg3;
static u8 chr0, chr1, chr2, chr3, chr4, chr5, chr6, chr7;
static u8 p[4];
static s32 c[8];
static u8 irq_enable;
static u8 irq_counter;
static u8 irq_latch;
static u8 irq_latched;
static u8 irq_reset;

static void setBankCpu(uint page, uint bank)
{
	Q_ASSERT(bank >= 4 && bank < 8);
	bank &= (reg[3] & 0x3F) ^ 0xFF;
	bank &= 0x3F;
	bank |= reg[1];
	nesSetRom8KBank(page, bank);
	p[bank - 4] = bank;
}

static void setBankPpu()
{
	static u8 table[16] = {
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x01,0x03,0x07,0x0F,0x1F,0x3F,0x7F,0xFF
	};

	c[0] = chr0;
	c[1] = chr1;
	c[2] = chr2;
	c[3] = chr3;
	c[4] = chr4;
	c[5] = chr5;
	c[6] = chr6;
	c[7] = chr7;

	for (int i = 0; i < 8; i++) {
		c[i] &= table[reg[2] & 0x0F];
		c[i] |= reg[0] & ((patch!=1)?0xFF:0xC0);
		c[i] += (reg[2] & ((patch!=1)?0x10:0x30))<<4;
	}

	if (reg[6] & 0x80) {
		for (int i = 0; i < 8; i++)
			nesSetVrom1KBank(i, c[i ^ 4]);
	} else {
		for (int i = 0; i < 8; i++)
			nesSetVrom1KBank(i, c[i]);
	}
}

static void horizontalSync()
{
	irq_reset = 0;
	if (nesPpuScanline < NesPpu::VisibleScreenHeight && nesPpuIsDisplayOn()) {
		if (irq_counter) {
			irq_counter--;
			if (irq_counter == 0) {
				if (irq_enable)
					nesMapperSetIrqSignalOut(true);
			}
		}
	}
}

static void writeLow(u16 addr, u8 data) {
	Q_UNUSED(addr)
	if (!(reg[3]&0x40)) {
		reg[reg[5]] = data;
		reg[5] = (reg[5]+1) & 0x03;

		setBankCpu(4, prg0);
		setBankCpu(5, prg1);
		setBankCpu(6, prg2);
		setBankCpu(7, prg3);
		setBankPpu();
	}
}

static void writeHigh(u16 addr, u8 data) {
	switch (addr & 0xE001) {
	case 0x8000:
		if ((data&0x40)!=(reg[6]&0x40)) {
			u8 swp;
			swp = prg0; prg0 = prg2; prg2 = swp;
			swp = p[0]; p[0] = p[2]; p[2] = swp;
			setBankCpu(4, p[0]);
			setBankCpu(5, p[1]);
		}
		if (nesVromSize1KB) {
			if ((data&0x80)!=(reg[6]&0x80)) {
				u32 swp;
				swp = chr4; chr4 = chr0; chr0 = swp;
				swp = chr5; chr5 = chr1; chr1 = swp;
				swp = chr6; chr6 = chr2; chr2 = swp;
				swp = chr7; chr7 = chr3; chr3 = swp;
				swp = c[4]; c[4] = c[0]; c[0] = swp;
				swp = c[5]; c[5] = c[1]; c[1] = swp;
				swp = c[6]; c[6] = c[2]; c[2] = swp;
				swp = c[7]; c[7] = c[3]; c[3] = swp;
				for (int i = 0; i < 8; i++)
					nesSetVrom1KBank(i, c[i]);
			}
		}
		reg[6] = data;
		break;
	case 0x8001:
		switch( reg[6] & 0x07) {
			case 0x00:
				chr0 = (data & 0xFE)+0;
				chr1 = (data & 0xFE)+1;
				setBankPpu();
				break;
			case 0x01:
				chr2 = (data & 0xFE)+0;
				chr3 = (data & 0xFE)+1;
				setBankPpu();
				break;
			case 0x02:
				chr4 = data;
				setBankPpu();
				break;
			case 0x03:
				chr5 = data;
				setBankPpu();
				break;
			case 0x04:
				chr6 = data;
				setBankPpu();
				break;
			case 0x05:
				chr7 = data;
				setBankPpu();
				break;
			case 0x06:
				if (reg[6] & 0x40) {
					prg2 = data & 0x3F;
					setBankCpu(6, data);
				} else {
					prg0 = data & 0x3F;
					setBankCpu(4, data);
				}
				break;
			case 0x07:
				prg1 = data & 0x3F;
				setBankCpu(5, data);
				break;
		}
		break;
	case 0xA000:
		nesSetMirroring(static_cast<NesMirroring>(data & 0x01));
		break;
	case 0xC000:
		if (patch == 2) {
			if (data == 0x29 || data == 0x70)
				data = 0x07;
		}
		irq_latch = data;
		irq_latched = 1;
		if (irq_reset) {
			irq_counter = data;
			irq_latched = 0;
		}
		break;
	case 0xC001:
		irq_counter = irq_latch;
		break;
	case 0xE000:
		irq_enable = 0;
		irq_reset = 1;
		nesMapperSetIrqSignalOut(false);
		break;
	case 0xE001:
		irq_enable = 1;
		if (irq_latched)
			irq_counter = irq_latch;
		break;
	}
}

void Mapper045::reset()
{
	NesMapper::reset();
	writeLow = ::writeLow;
	writeHigh = ::writeHigh;
	horizontalSync = ::horizontalSync;

	patch = 0;
	memset(reg, 0, sizeof(reg));

	prg0 = 0;
	prg1 = 1;
	prg2 = nesRomSize8KB-2;
	prg3 = nesRomSize8KB-1;

	u32 crc = nesDiskCrc;
	if (crc == 0x58bcacf6		// Kunio 8-in-1 (Pirate Cart)
	 || crc == 0x9103cfd6		// HIK 7-in-1 (Pirate Cart)
	 || crc == 0xc082e6d3) {	// Super 8-in-1 (Pirate Cart)
		patch = 1;
		prg2 = 62;
		prg3 = 63;
	}
	if (crc == 0xe0dd259d) {	// Super 3-in-1 (Pirate Cart)
		patch = 2;
	}

	nesSetRom8KBanks(prg0, prg1, prg2, prg3);

	p[0] = prg0;
	p[1] = prg1;
	p[2] = prg2;
	p[3] = prg3;

	nesSetVrom8KBank(0);

	chr0 = c[0] = 0;
	chr1 = c[1] = 1;
	chr2 = c[2] = 2;
	chr3 = c[3] = 3;
	chr4 = c[4] = 4;
	chr5 = c[5] = 5;
	chr6 = c[6] = 6;
	chr7 = c[7] = 7;

	irq_enable = 0;
	irq_counter = 0;
	irq_latch = 0;
	irq_latched = 0;
	irq_reset = 0;
}

void Mapper045::extSl()
{
	emsl.array("reg", reg, sizeof(reg));
	emsl.var("prg0", prg0);
	emsl.var("prg1", prg1);
	emsl.var("prg2", prg2);
	emsl.var("prg3", prg3);
	emsl.var("chr0", chr0);
	emsl.var("chr1", chr1);
	emsl.var("chr2", chr2);
	emsl.var("chr3", chr3);
	emsl.var("chr4", chr4);
	emsl.var("chr5", chr5);
	emsl.var("chr6", chr6);
	emsl.var("chr7", chr7);
	emsl.array("p", p, sizeof(p));
	emsl.array("c", c, sizeof(c));
	emsl.var("irq_enable", irq_enable);
	emsl.var("irq_counter", irq_counter);
	emsl.var("irq_latch", irq_latch);
	emsl.var("irq_latched", irq_latched);
	emsl.var("irq_reset", irq_reset);
}
