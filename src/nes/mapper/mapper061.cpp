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

#include "mapper061.h"

static void writeHigh(u16 addr, u8 data)
{
	Q_UNUSED(data)
	switch (addr & 0x30) {
	case 0x00:
	case 0x30:
		nesSetRom32KBank(addr & 0x0F);
		break;
	case 0x10:
	case 0x20:
		uint bank = ((addr&0x0F)<<1) | ((addr&0x20)>>4);
		nesSetRom16KBank(4, bank);
		nesSetRom16KBank(6, bank);
		break;
	}
	nesSetMirroring(static_cast<NesMirroring>((addr & 0x80) >> 7));
}

void Mapper061::reset()
{
	NesMapper::reset();
	writeHigh = ::writeHigh;

	nesSetRom8KBanks(0, 1, nesRomSize8KB-2, nesRomSize8KB-1);
}
