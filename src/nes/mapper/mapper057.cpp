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

#include "mapper057.h"

static u8 reg;

static void writeHigh(u16 addr, u8 data)
{
	switch (addr) {
	case 0x8000:
	case 0x8001:
	case 0x8002:
	case 0x8003:
		if (data & 0x40)
			nesSetVrom8KBank((data&0x03) + ((reg&0x10)>>1) + (reg&0x07));
		break;
	case 0x8800:
		reg = data;
		if (data & 0x80) {
			nesSetRom32KBank(((data & 0x40) >> 6) + 2);
		} else {
			nesSetRom16KBank(4, (data & 0x60) >> 5);
			nesSetRom16KBank(6, (data & 0x60) >> 5);
		}
		nesSetVrom8KBank((data&0x07) + ((data&0x10)>>1));
		nesSetMirroring(static_cast<NesMirroring>((data & 0x08) >> 3));
		break;
	}
}

void Mapper057::reset()
{
	NesMapper::reset();
	writeHigh = ::writeHigh;

	nesSetRom8KBanks(0, 1, 0, 1);
	nesSetVrom8KBank(0);

	reg = 0;
}

void Mapper057::extSl()
{
	emsl.var("reg", reg);
}
