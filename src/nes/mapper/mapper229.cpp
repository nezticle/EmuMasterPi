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

#include "mapper229.h"

static void writeHigh(u16 addr, u8 data)
{
	Q_UNUSED(data)

	if (addr & 0x001E) {
		u8 prg = addr & 0x001F;

		nesSetRom8KBank(4, prg*2+0);
		nesSetRom8KBank(5, prg*2+1);
		nesSetRom8KBank(6, prg*2+0);
		nesSetRom8KBank(7, prg*2+1);

		nesSetVrom8KBank(addr & 0x0FFF);
	} else {
		nesSetRom32KBank(0);
		nesSetVrom8KBank(0);
	}
	if (addr & 0x0020)
		nesSetMirroring(HorizontalMirroring);
	else
		nesSetMirroring(VerticalMirroring);
}

void Mapper229::reset()
{
	NesMapper::reset();
	writeHigh = ::writeHigh;

	nesSetRom32KBank(0);
	nesSetVrom8KBank(0);
}
