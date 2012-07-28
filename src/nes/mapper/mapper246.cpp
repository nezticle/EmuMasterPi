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

#include "mapper246.h"
#include "disk.h"

static void writeHigh(u16 addr, u8 data)
{
	if (addr >= 0x6000 && addr < 0x8000) {
		switch( addr) {
		case 0x6000:
			nesSetRom8KBank(4, data);
			break;
		case 0x6001:
			nesSetRom8KBank(5, data);
			break;
		case 0x6002:
			nesSetRom8KBank(6, data);
			break;
		case 0x6003:
			nesSetRom8KBank(7, data);
			break;
		case 0x6004:
			nesSetVrom2KBank(0, data);
			break;
		case 0x6005:
			nesSetVrom2KBank(2, data);
			break;
		case 0x6006:
			nesSetVrom2KBank(4, data);
			break;
		case 0x6007:
			nesSetVrom2KBank(6, data);
			break;
		default:
			nesCpuWriteDirect(addr, data);
			break;
		}
	}
}

void Mapper246::reset()
{
	NesMapper::reset();
	writeHigh = ::writeHigh;

	nesSetRom8KBanks(0, 1, nesRomSize8KB-2, nesRomSize8KB-1);
}
