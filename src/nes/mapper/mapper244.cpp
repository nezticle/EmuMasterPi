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

#include "mapper244.h"
#include "disk.h"

static void writeHigh(u16 addr, u8 data)
{
	Q_UNUSED(data)
	if (addr >= 0x8065 && addr <= 0x80A4)
		nesSetRom32KBank((addr-0x8065)&0x3);
	if (addr >= 0x80A5 && addr <= 0x80E4)
		nesSetVrom8KBank((addr-0x80A5)&0x7);
}

void Mapper244::reset()
{
	NesMapper::reset();
	writeHigh = ::writeHigh;

	nesSetRom32KBank(0);
}
