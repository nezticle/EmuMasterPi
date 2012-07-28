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

#include "mapper018.h"
#include "disk.h"

static u8 reg[11];
static u8 irq_enable;
static u8 irq_mode;
static int irq_latch;
static int irq_counter;

static void writeHigh(u16 addr, u8 data)
{
	switch (addr) {
	case 0x8000:
		reg[0] = (reg[0] & 0xF0) | (data & 0x0F);
		nesSetRom8KBank(4, reg[0]);
		break;
	case 0x8001:
		reg[0] = (reg[0] & 0x0F) | ((data & 0x0F) << 4);
		nesSetRom8KBank(4, reg[0]);
		break;
	case 0x8002:
		reg[1] = (reg[1] & 0xF0) | (data & 0x0F);
		nesSetRom8KBank(5, reg[1]);
		break;
	case 0x8003:
		reg[1] = (reg[1] & 0x0F) | ((data & 0x0F) << 4);
		nesSetRom8KBank(5, reg[1]);
		break;
	case 0x9000:
		reg[2] = (reg[2] & 0xF0) | (data & 0x0F);
		nesSetRom8KBank(6, reg[2]);
		break;
	case 0x9001:
		reg[2] = (reg[2] & 0x0F) | ((data & 0x0F) << 4);
		nesSetRom8KBank(6, reg[2]);
		break;

	case 0xA000:
		reg[3] = (reg[3] & 0xF0) | (data & 0x0F);
		nesSetVrom1KBank(0, reg[3]);
		break;
	case 0xA001:
		reg[3] = (reg[3] & 0x0F) | ((data & 0x0F) << 4);
		nesSetVrom1KBank(0, reg[3]);
		break;
	case 0xA002:
		reg[4] = (reg[4] & 0xF0) | (data & 0x0F);
		nesSetVrom1KBank(1, reg[4]);
		break;
	case 0xA003:
		reg[4] = (reg[4] & 0x0F) | ((data & 0x0F) << 4);
		nesSetVrom1KBank(1, reg[4]);
		break;

	case 0xB000:
		reg[5] = (reg[5] & 0xF0) | (data & 0x0F);
		nesSetVrom1KBank(2, reg[5]);
		break;
	case 0xB001:
		reg[5] = (reg[5] & 0x0F) | ((data & 0x0F) << 4);
		nesSetVrom1KBank(2, reg[5]);
		break;
	case 0xB002:
		reg[6] = (reg[6] & 0xF0) | (data & 0x0F);
		nesSetVrom1KBank(3, reg[6]);
		break;
	case 0xB003:
		reg[6] = (reg[6] & 0x0F) | ((data & 0x0F) << 4);
		nesSetVrom1KBank(3, reg[6]);
		break;

	case 0xC000:
		reg[7] = (reg[7] & 0xF0) | (data & 0x0F);
		nesSetVrom1KBank(4, reg[7]);
		break;
	case 0xC001:
		reg[7] = (reg[7] & 0x0F) | ((data & 0x0F) << 4);
		nesSetVrom1KBank(4, reg[7]);
		break;
	case 0xC002:
		reg[8] = (reg[8] & 0xF0) | (data & 0x0F);
		nesSetVrom1KBank(5, reg[8]);
		break;
	case 0xC003:
		reg[8] = (reg[8] & 0x0F) | ((data & 0x0F) << 4);
		nesSetVrom1KBank(5, reg[8]);
		break;

	case 0xD000:
		reg[9] = (reg[9] & 0xF0) | (data & 0x0F);
		nesSetVrom1KBank(6, reg[9]);
		break;
	case 0xD001:
		reg[9] = (reg[9] & 0x0F) | ((data & 0x0F) << 4);
		nesSetVrom1KBank(6, reg[9]);
		break;
	case 0xD002:
		reg[10] = (reg[10] & 0xF0) | (data & 0x0F);
		nesSetVrom1KBank(7, reg[10]);
		break;
	case 0xD003:
		reg[10] = (reg[10] & 0x0F) | ((data & 0x0F) << 4);
		nesSetVrom1KBank(7, reg[10]);
		break;

	case 0xE000:
		irq_latch = (irq_latch & 0xFFF0) | (data & 0x0F);
		break;
	case 0xE001:
		irq_latch = (irq_latch & 0xFF0F) | ((data & 0x0F) << 4);
		break;
	case 0xE002:
		irq_latch = (irq_latch & 0xF0FF) | ((data & 0x0F) << 8);
		break;
	case 0xE003:
		irq_latch = (irq_latch & 0x0FFF) | ((data & 0x0F) << 12);
		break;

	case 0xF000:
		irq_counter = irq_latch;
		break;
	case 0xF001:
		irq_mode = (data>>1) & 0x07;
		irq_enable = (data & 0x01);
		nesMapperSetIrqSignalOut(false);
		break;

	case 0xF002:
		data &= 0x03;
		if (data == 0)
			nesSetMirroring(HorizontalMirroring);
		else if (data == 1)
			nesSetMirroring(VerticalMirroring);
		else
			nesSetMirroring(SingleLow);
		break;
	}
}

static void clock(int cycles)
{
	bool bIRQ = false;
	int	irq_counter_old = irq_counter;

	if (irq_enable && irq_counter) {
		irq_counter -= cycles;

		switch( irq_mode) {
		case 0:
			if (irq_counter <= 0) {
				bIRQ = TRUE;
			}
			break;
		case 1:
			if ((irq_counter & 0xf000) != (irq_counter_old & 0xf000)) {
				bIRQ = TRUE;
			}
			break;
		case 2:
		case 3:
			if ((irq_counter & 0xff00) != (irq_counter_old & 0xff00)) {
				bIRQ = TRUE;
			}
			break;
		case 4:
		case 5:
		case 6:
		case 7:
			if ((irq_counter & 0xfff0) != (irq_counter_old & 0xfff0)) {
				bIRQ = TRUE;
			}
			break;
		}

		if (bIRQ) {
			irq_counter = 0;
			irq_enable = 0;
			nesMapperSetIrqSignalOut(true);
		}
	}
}

void Mapper018::reset()
{
	NesMapper::reset();
	writeHigh = ::writeHigh;
	clock = ::clock;

	for (int i = 0; i < 11; i++)
		reg[i] = 0;
	reg[2] = nesRomSize8KB-2;
	reg[3] = nesRomSize8KB-1;

	nesSetRom8KBanks(0, 1, nesRomSize8KB-2, nesRomSize8KB-1);

	irq_enable  = 0;
	irq_mode	= 0;
	irq_counter = 0xFFFF;
	irq_latch   = 0xFFFF;

	u32 crc = nesDiskCrc;

	if (crc == 0xefb1df9e) {	// The Lord of King(J)
		nesEmuSetRenderMethod(NesEmu::PreAllRender);
	}
	if (crc == 0x3746f951) {	// Pizza Pop!(J)
		nesEmuSetRenderMethod(NesEmu::PreAllRender);
	}
}

void Mapper018::extSl()
{
	emsl.array("reg", reg, sizeof(reg));
	emsl.var("irq_enable", irq_enable);
	emsl.var("irq_mode", irq_mode);
	emsl.var("irq_counter", irq_counter);
	emsl.var("irq_latch", irq_latch);
}
