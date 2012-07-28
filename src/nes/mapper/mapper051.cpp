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

#include "mapper051.h"

static u32 mode;
static u32 bank;

static void updateBanks()
{
	switch (mode) {
	case 0:
		nesSetMirroring(VerticalMirroring);
		nesSetRom8KBank(3, (bank|0x2c|3));
		nesSetRom8KBank(4, (bank|0x00|0));
		nesSetRom8KBank(5, (bank|0x00|1));
		nesSetRom8KBank(6, (bank|0x0c|2));
		nesSetRom8KBank(7, (bank|0x0c|3));
		break;
	case 1:
		nesSetMirroring(VerticalMirroring);
		nesSetRom8KBank(3, (bank|0x20|3));
		nesSetRom8KBank(4, (bank|0x00|0));
		nesSetRom8KBank(5, (bank|0x00|1));
		nesSetRom8KBank(6, (bank|0x00|2));
		nesSetRom8KBank(7, (bank|0x00|3));
		break;
	case 2:
		nesSetMirroring(VerticalMirroring);
		nesSetRom8KBank(3, (bank|0x2e|3));
		nesSetRom8KBank(4, (bank|0x02|0));
		nesSetRom8KBank(5, (bank|0x02|1));
		nesSetRom8KBank(6, (bank|0x0e|2));
		nesSetRom8KBank(7, (bank|0x0e|3));
		break;
	case 3:
		nesSetMirroring(HorizontalMirroring);
		nesSetRom8KBank(3, (bank|0x20|3));
		nesSetRom8KBank(4, (bank|0x00|0));
		nesSetRom8KBank(5, (bank|0x00|1));
		nesSetRom8KBank(6, (bank|0x00|2));
		nesSetRom8KBank(7, (bank|0x00|3));
		break;
	}
}

static void writeLow(u16 addr, u8 data)
{
	if (addr >= 0x6000) {
		mode = ((data & 0x10) >> 3) | ((data & 0x02) >> 1);
		updateBanks();
	}
}

static void writeHigh(u16 addr, u8 data) {
	bank = (data & 0x0f) << 2;
	if (0xC000 <= addr && addr <= 0xDFFF)
		mode = (mode & 0x01) | ((data & 0x10) >> 3);
	updateBanks();
}

void Mapper051::reset()
{
	NesMapper::reset();
	writeLow = ::writeLow;
	writeHigh = ::writeHigh;

	bank = 0;
	mode = 1;
	updateBanks();
	nesSetCram8KBank(0);
}

void Mapper051::extSl()
{
	emsl.var("mode", mode);
	emsl.var("bank", bank);
}
