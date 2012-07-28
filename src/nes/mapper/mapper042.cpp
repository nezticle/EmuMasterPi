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

#include "mapper042.h"

static u8 irq_enable;
static u8 irq_counter;

static void writeHigh(u16 addr, u8 data)
{
	switch (addr & 0xE003) {
	case 0xE000:
		nesSetRom8KBank(3, data&0x0F);
		break;

	case 0xE001:
		nesSetMirroring(static_cast<NesMirroring>((data & 0x08) >> 3));
		break;

	case 0xE002:
		if (data&0x02) {
			irq_enable = 0xFF;
		} else {
			irq_enable = 0;
			irq_counter = 0;
		}
		nesMapperSetIrqSignalOut(false);
		break;
	}
}

static void horizontalSync()
{
	nesMapperSetIrqSignalOut(false);
	if (irq_enable) {
		if (irq_counter < 215) {
			irq_counter++;
		}
		if (irq_counter == 215) {
			irq_enable = 0;
			nesMapperSetIrqSignalOut(true);
		}
	}
}

void Mapper042::reset()
{
	NesMapper::reset();
	writeHigh = ::writeHigh;
	horizontalSync = ::horizontalSync;

	nesSetRom8KBank(3, 0);
	nesSetRom8KBanks(nesRomSize8KB-4, nesRomSize8KB-3, nesRomSize8KB-2, nesRomSize8KB-1);
	if (nesVromSize1KB)
		nesSetVrom8KBank(0);
	irq_enable = 0;
	irq_counter = 0;
}

void Mapper042::extSl()
{
	emsl.var("irq_enable", irq_enable);
	emsl.var("irq_counter", irq_counter);
}
