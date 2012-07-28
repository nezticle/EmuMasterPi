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

#include "mapper079.h"

static void writeLow(u16 addr, u8 data)
{
	if (addr & 0x0100) {
		nesSetRom32KBank((data>>3) & 0x01);
		nesSetVrom8KBank(data & 0x07);
	}
}

void Mapper079::reset()
{
	NesMapper::reset();
	writeLow = ::writeLow;

	nesSetRom32KBank(0);
	if (nesVromSize1KB)
		nesSetVrom8KBank(0);
}
