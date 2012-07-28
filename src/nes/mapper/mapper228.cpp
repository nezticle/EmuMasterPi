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

#include "mapper228.h"

static void writeHigh(u16 addr, u8 data)
{
	u8 prg = (addr&0x0780) >> 7;

	switch ((addr&0x1800) >> 11) {
	case 1:
		prg |= 0x10;
		break;
	case 3:
		prg |= 0x20;
		break;
	}

	if (addr & 0x0020) {
		prg <<= 1;
		if (addr & 0x0040)
			prg++;
		nesSetRom8KBank(4, prg*4+0);
		nesSetRom8KBank(5, prg*4+1);
		nesSetRom8KBank(6, prg*4+0);
		nesSetRom8KBank(7, prg*4+1);
	} else {
		nesSetRom8KBank(4, prg*4+0);
		nesSetRom8KBank(5, prg*4+1);
		nesSetRom8KBank(6, prg*4+2);
		nesSetRom8KBank(7, prg*4+3);
	}

	nesSetVrom8KBank(((addr&0x000F)<<2)|(data&0x03));

	if (addr & 0x2000)
		nesSetMirroring(HorizontalMirroring);
	else
		nesSetMirroring(VerticalMirroring);
}

void Mapper228::reset()
{
	NesMapper::reset();
	writeHigh = ::writeHigh;

	nesSetRom32KBank(0);
	nesSetVrom8KBank(0);
}
