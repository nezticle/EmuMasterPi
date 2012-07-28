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

#include "mapper002.h"
#include "disk.h"

static u8 patch;
static bool hasBattery;

static void writeLow(u16 addr, u8 data)
{
	if (!hasBattery) {
		if (addr >= 0x5000 && patch == 1)
			nesSetRom16KBank(4, data);
	} else {
		nesDefaultCpuWriteLow(addr, data);
	}
}

static void writeHigh(u16 addr, u8 data)
{
	Q_UNUSED(addr)
	if (patch != 2)
		nesSetRom16KBank(4, data);
	else
		nesSetRom16KBank(4, data >> 4);
}

void Mapper002::reset()
{
	NesMapper::reset();
	writeLow = :: writeLow;
	writeHigh = :: writeHigh;

	nesSetRom8KBanks(0, 1, nesRomSize8KB-2, nesRomSize8KB-1);
	patch = 0;
	hasBattery = nesDiskHasBatteryBackedRam();

	u32 crc = nesDiskCrc;
	if( crc == 0x8c3d54e8		// Ikari(J)
	 || crc == 0x655efeed		// Ikari Warriors(U)
	 || crc == 0x538218b2 ) {	// Ikari Warriors(E)
		patch = 1;
	}
	if( crc == 0xb20c1030 ) {	// Shanghai(J)(original)
		patch = 2;
	}
}
