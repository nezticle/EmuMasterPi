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

#include "mapper043.h"

static u8 irq_enable;
static int irq_counter;

static u8 readLow(u16 addr)
{
	if (0x5000 <= addr && addr < 0x6000)
		return	nesRom[0x2000*8+0x1000+(addr-0x5000)];
	return addr >> 8;
}

static void writeEx(u16 addr, u8 data)
{
	if ((addr&0xF0FF) == 0x4022) {
		switch (data&0x07) {
		case 0x00:
		case 0x02:
		case 0x03:
		case 0x04:
			nesSetRom8KBank(6, 4);
			break;
		case 0x01:
			nesSetRom8KBank(6, 3);
			break;
		case 0x05:
			nesSetRom8KBank(6, 7);
			break;
		case 0x06:
			nesSetRom8KBank(6, 5);
			break;
		case 0x07:
			nesSetRom8KBank(6, 6);
			break;
		}
	}
}

static void writeLow(u16 addr, u8 data)
{
	if ((addr&0xF0FF) == 0x4022)
		writeEx(addr, data);
}

static void writeHigh(u16 addr, u8 data)
{
	if (addr == 0x8122) {
		if (data & 0x03) {
			irq_enable = 1;
		} else {
			irq_counter = 0;
			irq_enable = 0;
		}
		nesMapperSetIrqSignalOut(false);
	}
}

static void horizontalSync()
{
	nesMapperSetIrqSignalOut(false);
	if (irq_enable) {
		irq_counter += 341;
		if (irq_counter >= 12288) {
			irq_counter = 0;
			nesMapperSetIrqSignalOut(true);
		}
	}
}

void Mapper043::reset()
{
	NesMapper::reset();
	readLow = ::readLow;
	writeEx = ::writeEx;
	writeLow = ::writeLow;
	writeHigh = ::writeHigh;
	horizontalSync = ::horizontalSync;

	nesSetRom8KBank(3, 2);
	nesSetRom8KBanks(1, 0, 4, 9);
	if (nesVromSize1KB)
		nesSetVrom8KBank(0);
	irq_enable = 0xFF;
	irq_counter = 0;
}

void Mapper043::extSl()
{
	emsl.var("irq_enable", irq_enable);
	emsl.var("irq_counter", irq_counter);
}
