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

#include "mapper222.h"

static void writeHigh(u16 addr, u8 data)
{
	switch (addr & 0xF003) {
	case 0x8000:
		nesSetRom8KBank(4, data);
		break;
	case 0xA000:
		nesSetRom8KBank(5, data);
		break;
	case 0xB000:
		nesSetVrom1KBank(0, data);
		break;
	case 0xB002:
		nesSetVrom1KBank(1, data);
		break;
	case 0xC000:
		nesSetVrom1KBank(2, data);
		break;
	case 0xC002:
		nesSetVrom1KBank(3, data);
		break;
	case 0xD000:
		nesSetVrom1KBank(4, data);
		break;
	case 0xD002:
		nesSetVrom1KBank(5, data);
		break;
	case 0xE000:
		nesSetVrom1KBank(6, data);
		break;
	case 0xE002:
		nesSetVrom1KBank(7, data);
		break;
	}
}

void Mapper222::reset()
{
	NesMapper::reset();
	writeHigh = ::writeHigh;

	nesSetRom8KBanks(0, 1, nesRomSize8KB-2, nesRomSize8KB-1);
	if (nesVromSize1KB)
		nesSetVrom8KBank(0);
	nesSetMirroring(VerticalMirroring);
}
