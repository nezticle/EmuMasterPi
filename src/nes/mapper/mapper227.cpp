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

#include "mapper227.h"

static void writeHigh(u16 addr, u8 data)
{
	Q_UNUSED(data)

	u8 bank = ((addr&0x0100)>>4) | ((addr&0x0078)>>3);

	if (addr & 0x0001) {
		nesSetRom32KBank(bank);
	} else {
		if (addr & 0x0004) {
			nesSetRom8KBank(4, bank*4+2);
			nesSetRom8KBank(5, bank*4+3);
			nesSetRom8KBank(6, bank*4+2);
			nesSetRom8KBank(7, bank*4+3);
		} else {
			nesSetRom8KBank(4, bank*4+0);
			nesSetRom8KBank(5, bank*4+1);
			nesSetRom8KBank(6, bank*4+0);
			nesSetRom8KBank(7, bank*4+1);
		}
	}

	if (!(addr & 0x0080)) {
		if (addr & 0x0200) {
			nesSetRom8KBank(6, (bank&0x1C)*4+14);
			nesSetRom8KBank(7, (bank&0x1C)*4+15);
		} else {
			nesSetRom8KBank(6, (bank&0x1C)*4+0);
			nesSetRom8KBank(7, (bank&0x1C)*4+1);
		}
	}
	if (addr & 0x0002)
		nesSetMirroring(HorizontalMirroring);
	else
		nesSetMirroring(VerticalMirroring);
}

void Mapper227::reset()
{
	NesMapper::reset();
	writeHigh = ::writeHigh;

	nesSetRom8KBanks(0, 1, 0, 1);
}
