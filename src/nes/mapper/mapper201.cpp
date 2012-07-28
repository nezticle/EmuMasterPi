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

#include "mapper201.h"

static void writeHigh(u16 addr, u8 data)
{
	Q_UNUSED(data)

	u8 bank = addr & 0x03;
	if (!(addr & 0x08))
		bank = 0;
	nesSetRom32KBank(bank);
	nesSetVrom8KBank(bank);
}

void Mapper201::reset()
{
	NesMapper::reset();
	writeHigh = ::writeHigh;

	nesSetRom16KBank(4, 0);
	nesSetRom16KBank(6, 0);
	if (nesVromSize1KB)
		nesSetVrom8KBank(0);
}
