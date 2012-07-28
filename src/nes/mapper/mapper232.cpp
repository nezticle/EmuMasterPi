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

#include "mapper232.h"

static u8 reg[2];

static void writeHigh(u16 addr, u8 data) {
	if (addr <= 0x9fff)
		reg[0] = (data & 0x18)>>1;
	else
		reg[1] = data & 0x03;

	nesSetRom8KBank(4, (reg[0]|reg[1])*2+0);
	nesSetRom8KBank(5, (reg[0]|reg[1])*2+1);
	nesSetRom8KBank(6, (reg[0]|0x03)*2+0);
	nesSetRom8KBank(7, (reg[0]|0x03)*2+1);
}

static void writeLow(u16 addr, u8 data)
{
	if (addr >= 0x6000)
		writeHigh(addr, data);
}

void Mapper232::reset()
{
	NesMapper::reset();
	writeLow = ::writeLow;
	writeHigh = ::writeHigh;

	nesSetRom8KBanks(0, 1, nesRomSize8KB-2, nesRomSize8KB-1);

	reg[0] = 0x0c;
	reg[1] = 0x00;
}

void Mapper232::extSl()
{
	emsl.array("reg", reg, sizeof(reg));
}
