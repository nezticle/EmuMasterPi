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

#include "mapper040.h"

static u8 irq_enable;
static int irq_line;


static void writeHigh(u16 address, u8 data)
{
	switch (address & 0xe000) {
	case 0x8000:
		irq_enable = 0;
		nesMapperSetIrqSignalOut(false);
		break;
	case 0xa000:
		irq_enable = 0xff;
		irq_line = 37;
		nesMapperSetIrqSignalOut(false);
		break;
	case 0xc000:
		break;
	case 0xe000:
		nesSetRom8KBank(6, data&0x07);
		break;
	}
}

static void horizontalSync()
{
	if (irq_enable) {
		if (--irq_line <= 0)
			nesMapperSetIrqSignalOut(true);
	}
}

void Mapper040::reset()
{
	NesMapper::reset();
	writeHigh = ::writeHigh;
	horizontalSync = ::horizontalSync;

	nesSetRom8KBank(3, 6);
	nesSetRom8KBanks(4, 5, 0, 7);
	if (nesVromSize1KB)
		nesSetVrom8KBank(0);

	irq_enable = 0;
	irq_line = 0;
}

void Mapper040::extSl()
{
	emsl.var("irq_enable", irq_enable);
	emsl.var("irq_line", irq_line);
}
