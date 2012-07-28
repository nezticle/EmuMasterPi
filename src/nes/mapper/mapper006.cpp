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

#include "mapper006.h"

static u8 irqEnable;
static int irqCounter;

static void writeLow(u16 addr, u8 data) {
	switch (addr) {
	case 0x42fe:
		if (data & 0x10)
			nesSetMirroring(SingleHigh);
		else
			nesSetMirroring(SingleLow);
		break;
	case 0x42ff:
		if (data & 0x10)
			nesSetMirroring(HorizontalMirroring);
		else
			nesSetMirroring(VerticalMirroring);
		break;
	case 0x4501:
		irqEnable = 0;
		nesMapperSetIrqSignalOut(false);
		break;
	case 0x4502:
		irqCounter = (irqCounter&0xff00) | (data<<0);
		break;
	case 0x4503:
		irqCounter = (irqCounter&0x00ff) | (data<<8);
		irqEnable = 0xff;
		nesMapperSetIrqSignalOut(false);
		break;
	default:
		nesDefaultCpuWriteLow(addr, data);
		break;
	}
}

static void writeHigh(u16 address, u8 data)
{
	Q_UNUSED(address)
	nesSetRom16KBank(4, (data & 0x3c) >> 2);
	nesSetCram8KBank(data & 0x03);
}

static void horizontalSync()
{
	if (irqEnable) {
		irqCounter += 133;
		if (irqCounter >= 0xffff) {
			irqCounter = 0;
			nesMapperSetIrqSignalOut(true);
		}
	}
}

void Mapper006::reset()
{
	NesMapper::reset();
	writeLow = ::writeLow;
	writeHigh = ::writeHigh;
	horizontalSync = ::horizontalSync;

	nesSetRom8KBanks(0, 1, 14, 15);
	if (nesVromSize1KB)
		nesSetVrom8KBank(0);
	else
		nesSetCram8KBank(0);
	irqEnable = 0;
	irqCounter = 0;
}

void Mapper006::extSl()
{
	emsl.var("irqEnable", irqEnable);
	emsl.var("irqCounter", irqCounter);
}
