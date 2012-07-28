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

#include "mapper202.h"

static void writeSub(u16 addr, u8 data)
{
	Q_UNUSED(data)

	int bank = (addr>>1) & 0x07;
	nesSetRom16KBank(4, bank);
	if ((addr & 0x0C) == 0x0C)
		nesSetRom16KBank(6, bank+1);
	else
		nesSetRom16KBank(6, bank);
	nesSetVrom8KBank(bank);

	if (addr & 0x01)
		nesSetMirroring(HorizontalMirroring);
	else
		nesSetMirroring(VerticalMirroring);
}

static void writeEx(u16 addr, u8 data)
{
	if (addr >= 0x4020)
		writeSub(addr, data);
}

static void writeLow(u16 addr, u8 data)
{
	writeSub(addr, data);
}

static void writeHigh(u16 addr, u8 data)
{
	writeSub(addr, data);
}

void Mapper202::reset()
{
	NesMapper::reset();
	writeEx = ::writeEx;
	writeLow = ::writeLow;
	writeHigh = ::writeHigh;

	nesSetRom16KBank(4, 6);
	nesSetRom16KBank(6, 7);
	if (nesVromSize1KB)
		nesSetVrom8KBank(0);
}
