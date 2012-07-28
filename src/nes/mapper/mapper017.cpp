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

#include "mapper017.h"

static u8 irqEnable;
static int irqCounter;
static int irqLatch;

static void writeLow(u16 addr, u8 data)
{
	switch (addr) {
	case 0x42fE:
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
		irqLatch = (irqCounter&0x00ff) | (data<<8);
		irqCounter = irqLatch;
		irqEnable = 0xFF;
		break;
	case 0x4504:
	case 0x4505:
	case 0x4506:
	case 0x4507:
		nesSetRom8KBank(addr&0x07, data);
		break;

	case 0x4510:
	case 0x4511:
	case 0x4512:
	case 0x4513:
	case 0x4514:
	case 0x4515:
	case 0x4516:
	case 0x4517:
		nesSetVrom1KBank(addr&0x07, data);
		break;

	default:
		nesDefaultCpuWriteLow(addr, data);
		break;
	}
}

static void writeHigh(u16 addr, u8 data)
{
	Q_UNUSED(addr)
	nesSetRom16KBank(4, (data & 0x3c) >> 2);
	nesSetCram8KBank(data & 0x03);
}

static void horizontalSync()
{
	if (irqEnable) {
		if (irqCounter >= 0xffff-113) {
			nesMapperSetIrqSignalOut(true);
			irqCounter &= 0xffff;
		} else {
			irqCounter += 133;
		}
	}
}

void Mapper017::reset()
{
	NesMapper::reset();
	writeLow = ::writeLow;
	writeHigh = ::writeHigh;
	horizontalSync = ::horizontalSync;

	nesSetRom8KBanks(0, 1, nesRomSize8KB-2, nesRomSize8KB-1);
	if (nesVromSize1KB)
		nesSetVrom8KBank(0);
	irqEnable = 0;
	irqCounter = 0;
}

void Mapper017::extSl()
{
	emsl.var("irqEnable", irqEnable);
	emsl.var("irqCounter", irqCounter);
	emsl.var("irqLatch", irqLatch);
}
