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

#include "mapper236.h"

static u8 bank;
static u8 mode;

static void writeHigh(u16 addr, u8 data)
{
	Q_UNUSED(data)

	if (addr >= 0x8000 && addr <= 0xBFFF) {
		bank = ((addr&0x03)<<4)|(bank&0x07);
	} else {
		bank = (addr&0x07)|(bank&0x30);
		mode = addr & 0x30;
	}

	if (addr & 0x20)
		nesSetMirroring(HorizontalMirroring);
	else
		nesSetMirroring(VerticalMirroring);

	switch (mode) {
	case 0x00:
		bank |= 0x08;
		nesSetRom8KBank(4, bank*2+0);
		nesSetRom8KBank(5, bank*2+1);
		nesSetRom8KBank(6, (bank|0x07)*2+0);
		nesSetRom8KBank(7, (bank|0x07)*2+1);
		break;
	case 0x10:
		bank |= 0x37;
		nesSetRom8KBank(4, bank*2+0);
		nesSetRom8KBank(5, bank*2+1);
		nesSetRom8KBank(6, (bank|0x07)*2+0);
		nesSetRom8KBank(7, (bank|0x07)*2+1);
		break;
	case 0x20:
		bank |= 0x08;
		nesSetRom8KBank(4, (bank&0xFE)*2+0);
		nesSetRom8KBank(5, (bank&0xFE)*2+1);
		nesSetRom8KBank(6, (bank&0xFE)*2+2);
		nesSetRom8KBank(7, (bank&0xFE)*2+3);
		break;
	case 0x30:
		bank |= 0x08;
		nesSetRom8KBank(4, bank*2+0);
		nesSetRom8KBank(5, bank*2+1);
		nesSetRom8KBank(6, bank*2+0);
		nesSetRom8KBank(7, bank*2+1);
		break;
	}
}

void Mapper236::reset()
{
	NesMapper::reset();
	writeHigh = ::writeHigh;

	nesSetRom8KBanks(0, 1, nesRomSize8KB-2, nesRomSize8KB-1);
	bank = mode = 0;
}

void Mapper236::extSl()
{
	emsl.var("bank", bank);
	emsl.var("mode", mode);
}
