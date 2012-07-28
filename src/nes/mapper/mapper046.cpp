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

#include "mapper046.h"

static u32 reg[4];

static void updateBanks()
{
	nesSetRom32KBank(reg[0]*2+reg[2]);
	nesSetVrom8KBank(reg[1]*8+reg[3]);
}

static void writeLow(u16 addr, u8 data)
{
	Q_UNUSED(addr)
	reg[0] = data & 0x0F;
	reg[1] = (data & 0xF0) >> 4;
	updateBanks();
}

static void writeHigh(u16 addr, u8 data)
{
	Q_UNUSED(addr)
	reg[2] = data & 0x01;
	reg[3] = (data & 0x70) >> 4;
	updateBanks();
}

void Mapper046::reset()
{
	NesMapper::reset();
	writeLow = ::writeLow;
	writeHigh = ::writeHigh;

	memset(reg, 0, 4);
	updateBanks();
	nesSetMirroring(VerticalMirroring);
}

void Mapper046::extSl()
{
	emsl.array("reg", reg, sizeof(reg));
}
