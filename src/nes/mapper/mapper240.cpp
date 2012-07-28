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

#include "mapper240.h"

static void writeLow(u16 addr, u8 data)
{
	if (addr>=0x4020 && addr < 0x6000) {
		nesSetRom32KBank((data & 0xf0) >> 4);
		nesSetVrom8KBank(data & 0xf);
	}
}

void Mapper240::reset()
{
	NesMapper::reset();
	writeLow = ::writeLow;

	nesSetRom8KBanks(0, 1, nesRomSize8KB-2, nesRomSize8KB-1);
	if (nesVromSize1KB)
		nesSetVrom8KBank(0);
}
