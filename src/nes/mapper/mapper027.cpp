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

#include "mapper027.h"
#include "disk.h"

static u8 reg[9];
static u8 irq_enable;
static u8 irq_counter;
static u8 irq_latch;
static int irq_clock;

static void writeHigh(u16 addr, u8 data)
{
	switch (addr & 0xF0CF) {
	case 0x8000:
		if(reg[8] & 0x02) {
			nesSetRom8KBank(6, data);
		} else {
			nesSetRom8KBank(4, data);
		}
		break;
	case 0xA000:
		nesSetRom8KBank(5, data);
		break;

	case 0x9000:
		nesSetMirroring(static_cast<NesMirroring>(data & 3));
		break;

	case 0x9002:
	case 0x9080:
		reg[8] = data;
		break;

	case 0xB000:
		reg[0] = (reg[0] & 0xF0) | (data & 0x0F);
		nesSetVrom1KBank(0, reg[0] );
		break;
	case 0xB001:
		reg[0] = (reg[0] & 0x0F) | (data<< 4);
		nesSetVrom1KBank(0, reg[0] );
		break;

	case 0xB002:
		reg[1] = (reg[1] & 0xF0) | (data & 0x0F);
		nesSetVrom1KBank(1, reg[1] );
		break;
	case 0xB003:
		reg[1] = (reg[1] & 0x0F) | (data<< 4);
		nesSetVrom1KBank(1, reg[1] );
		break;

	case 0xC000:
		reg[2] = (reg[2] & 0xF0) | (data & 0x0F);
		nesSetVrom1KBank(2, reg[2] );
		break;
	case 0xC001:
		reg[2] = (reg[2] & 0x0F) | (data<< 4);
		nesSetVrom1KBank(2, reg[2] );
		break;

	case 0xC002:
		reg[3] = (reg[3] & 0xF0) | (data & 0x0F);
		nesSetVrom1KBank(3, reg[3] );
		break;
	case 0xC003:
		reg[3] = (reg[3] & 0x0F) | (data<< 4);
		nesSetVrom1KBank(3, reg[3] );
		break;

	case 0xD000:
		reg[4] = (reg[4] & 0xF0) | (data & 0x0F);
		nesSetVrom1KBank(4, reg[4] );
		break;
	case 0xD001:
		reg[4] = (reg[4] & 0x0F) | (data<< 4);
		nesSetVrom1KBank(4, reg[4] );
		break;

	case 0xD002:
		reg[5] = (reg[5] & 0xF0) | (data & 0x0F);
		nesSetVrom1KBank(5, reg[5] );
		break;
	case 0xD003:
		reg[5] = (reg[5] & 0x0F) | (data << 4);
		nesSetVrom1KBank(5, reg[5] );
		break;

	case 0xE000:
		reg[6] = (reg[6] & 0xF0) | (data & 0x0F);
		nesSetVrom1KBank(6, reg[6] );
		break;
	case 0xE001:
		reg[6] = (reg[6] & 0x0F) | (data << 4);
		nesSetVrom1KBank(6, reg[6] );
		break;

	case 0xE002:
		reg[7] = (reg[7] & 0xF0) | (data & 0x0F);
		nesSetVrom1KBank(7, reg[7] );
		break;
	case 0xE003:
		reg[7] = (reg[7] & 0x0F) | (data<< 4);
		nesSetVrom1KBank(7, reg[7] );
		break;

	case 0xF000:
		irq_latch = (irq_latch & 0xF0) | (data & 0x0F);
		nesMapperSetIrqSignalOut(false);
		break;
	case 0xF001:
		irq_latch = (irq_latch & 0x0F) | ((data & 0x0F) << 4);
		nesMapperSetIrqSignalOut(false);
		break;

	case 0xF003:
		irq_enable = (irq_enable & 0x01) * 3;
		irq_clock = 0;
		nesMapperSetIrqSignalOut(false);
		break;

	case 0xF002:
		irq_enable = data & 0x03;
		if( irq_enable & 0x02 ) {
			irq_counter = irq_latch;
			irq_clock = 0;
		}
		nesMapperSetIrqSignalOut(false);
		break;
	}
}

static void horizontalSync()
{
	if (irq_enable & 0x02) {
		if (irq_counter == 0xFF) {
			irq_counter = irq_latch;
			nesMapperSetIrqSignalOut(true);
		} else {
			irq_counter++;
		}
	}
}

void Mapper027::reset()
{
	NesMapper::reset();
	writeHigh = ::writeHigh;
	horizontalSync = ::horizontalSync;

	for (int i = 0; i < 8; i++)
		reg[i] = i;
	reg[8] = 0;

	irq_enable = 0;
	irq_counter = 0;
	irq_latch = 0;
	irq_clock = 0;

	nesSetRom8KBanks(0, 1, nesRomSize8KB-2, nesRomSize8KB-1);

	u32 crc = nesDiskCrc;
	if( crc == 0x47DCBCC4 ) {	// Gradius II(sample)
		nesEmuSetRenderMethod(NesEmu::PostRender);
	}
	if( crc == 0x468F21FC ) {	// Racer Mini 4 ku(sample)
		nesEmuSetRenderMethod(NesEmu::PostRender);
	}
}

void Mapper027::extSl()
{
	emsl.array("reg", reg, sizeof(reg));
	emsl.var("irq_enable", irq_enable);
	emsl.var("irq_counter", irq_counter);
	emsl.var("irq_latch", irq_latch);
	emsl.var("irq_clock", irq_clock);
}
