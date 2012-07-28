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

#include "mapper068.h"

static u8 reg[4];

static void updateBanks()
{
	if (reg[0]) {
		switch (reg[1]) {
		case 0:
			nesSetVrom1KBank( 8, reg[2]+0x80);
			nesSetVrom1KBank( 9, reg[3]+0x80);
			nesSetVrom1KBank(10, reg[2]+0x80);
			nesSetVrom1KBank(11, reg[3]+0x80);
			break;
		case 1:
			nesSetVrom1KBank( 8, reg[2]+0x80);
			nesSetVrom1KBank( 9, reg[2]+0x80);
			nesSetVrom1KBank(10, reg[3]+0x80);
			nesSetVrom1KBank(11, reg[3]+0x80);
			break;
		case 2:
			nesSetVrom1KBank( 8, reg[2]+0x80);
			nesSetVrom1KBank( 9, reg[2]+0x80);
			nesSetVrom1KBank(10, reg[2]+0x80);
			nesSetVrom1KBank(11, reg[2]+0x80);
			break;
		case 3:
			nesSetVrom1KBank( 8, reg[3]+0x80);
			nesSetVrom1KBank( 9, reg[3]+0x80);
			nesSetVrom1KBank(10, reg[3]+0x80);
			nesSetVrom1KBank(11, reg[3]+0x80);
			break;
		}
	} else {
		nesSetMirroring(static_cast<NesMirroring>(reg[1] & 0x03));
	}
}

static void writeHigh(u16 addr, u8 data)
{
	switch (addr & 0xF000) {
	case 0x8000:
		nesSetVrom2KBank(0, data);
		break;
	case 0x9000:
		nesSetVrom2KBank(2, data);
		break;
	case 0xA000:
		nesSetVrom2KBank(4, data);
		break;
	case 0xB000:
		nesSetVrom2KBank(6, data);
		break;

	case 0xC000:
		reg[2] = data;
		updateBanks();
		break;
	case 0xD000:
		reg[3] = data;
		updateBanks();
		break;
	case 0xE000:
		reg[0] = (data & 0x10)>>4;
		reg[1] = data & 0x03;
		updateBanks();
		break;

	case 0xF000:
		nesSetRom16KBank(4, data);
		break;
	}
}

void Mapper068::reset()
{
	NesMapper::reset();
	writeHigh = ::writeHigh;

	reg[0] = reg[1] = reg[2] = reg[3] = 0;

	nesSetRom8KBanks(0, 1, nesRomSize8KB-2, nesRomSize8KB-1);
}

void Mapper068::extSl()
{
	emsl.array("reg", reg, sizeof(reg));
}
