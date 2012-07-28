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

#include "mapper233.h"

static void writeHigh(u16 addr, u8 data)
{
	Q_UNUSED(addr)

	if (data & 0x20) {
		nesSetRom8KBank(4, (data&0x1F)*2+0);
		nesSetRom8KBank(5, (data&0x1F)*2+1);
		nesSetRom8KBank(6, (data&0x1F)*2+0);
		nesSetRom8KBank(7, (data&0x1F)*2+1);
	} else {
		u8 bank = (data&0x1E)>>1;

		nesSetRom8KBank(4, bank*4+0);
		nesSetRom8KBank(5, bank*4+1);
		nesSetRom8KBank(6, bank*4+2);
		nesSetRom8KBank(7, bank*4+3);
	}

	if ((data&0xC0) == 0x00)
		nesSetMirroring(0, 0, 0, 1);
	else if ((data&0xC0) == 0x40)
		nesSetMirroring(VerticalMirroring);
	else if ((data&0xC0) == 0x80)
		nesSetMirroring(HorizontalMirroring);
	else
		nesSetMirroring(SingleHigh);
}

void Mapper233::reset()
{
	NesMapper::reset();
	writeHigh = ::writeHigh;

	nesSetRom32KBank(0);
}
