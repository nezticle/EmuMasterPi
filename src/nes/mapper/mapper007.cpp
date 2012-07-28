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

#include "mapper007.h"
#include "disk.h"

static u8 patch;

static void writeHigh(u16 addr, u8 data)
{
	Q_UNUSED(addr)
	nesSetRom32KBank(data & 0x07);
	if (!patch) {
		if (data & 0x10)
			nesSetMirroring(SingleHigh);
		else
			nesSetMirroring(SingleLow);
	}
}

void Mapper007::reset()
{
	NesMapper::reset();
	writeHigh = ::writeHigh;

	patch = 0;
	nesSetRom32KBank(0);
	nesSetMirroring(SingleLow);

	u32 crc = nesDiskCrc;
	if( crc == 0x3c9fe649 ) {	// WWF Wrestlemania Challenge(U)
		nesSetMirroring(VerticalMirroring);
		patch = 1;
	}
	if( crc == 0x09874777 ) {	// Marble Madness(U)
		nesEmuSetRenderMethod(NesEmu::TileRender);
	}

	if( crc == 0x279710dc		// Battletoads (U)
	 || crc == 0xceb65b06 ) {	// Battletoads Double Dragon (U)
		nesEmuSetRenderMethod(NesEmu::PreAllRender);
		memset(nesWram, 0, sizeof(nesWram));
	}
}
