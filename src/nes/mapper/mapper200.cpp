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

#include "mapper200.h"

static void writeHigh(u16 addr, u8 data)
{
	Q_UNUSED(data)

	nesSetRom16KBank(4, addr & 0x07);
	nesSetRom16KBank(6, addr & 0x07);
	nesSetVrom8KBank(addr & 0x07);

	if (addr & 0x01)
		nesSetMirroring(VerticalMirroring);
	else
		nesSetMirroring(HorizontalMirroring);
}

void Mapper200::reset()
{
	NesMapper::reset();
	writeHigh = ::writeHigh;

	nesSetRom16KBank(4, 0);
	nesSetRom16KBank(6, 0);
	if (nesVromSize1KB)
		nesSetVrom8KBank(0);
}
