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

#include "mapper050.h"
#include "ppu.h"

static u8 irq_enable;

static void writeEx(u16 addr, u8 data)
{
	if ((addr & 0xE060) == 0x4020) {
		if (addr & 0x0100) {
			irq_enable = data & 0x01;
			nesMapperSetIrqSignalOut(false);
		} else {
			nesSetRom8KBank(6, (data&0x08)|((data&0x01)<<2)|((data&0x06)>>1));
		}
	}
}

static void writeLow(u16 addr, u8 data)
{
	if ((addr & 0xE060) == 0x4020) {
		if (addr & 0x0100) {
			irq_enable = data & 0x01;
			nesMapperSetIrqSignalOut(false);
		} else {
			nesSetRom8KBank(6, (data&0x08)|((data&0x01)<<2)|((data&0x06)>>1));
		}
	}
}

static void horizontalSync()
{
	if (irq_enable) {
		if (nesPpuScanline == 21)
			nesMapperSetIrqSignalOut(true);
	}
}

void Mapper050::reset()
{
	NesMapper::reset();
	writeEx = ::writeEx;
	writeLow = ::writeLow;
	horizontalSync = ::horizontalSync;

	irq_enable = 0;
	nesSetRom8KBank(3, 15);
	nesSetRom8KBank(4, 8);
	nesSetRom8KBank(5, 9);
	nesSetRom8KBank(6, 0);
	nesSetRom8KBank(7, 11);
	if (nesVromSize1KB)
		nesSetVrom8KBank(0);
}

void Mapper050::extSl()
{
	emsl.var("irq_enable", irq_enable);
}
