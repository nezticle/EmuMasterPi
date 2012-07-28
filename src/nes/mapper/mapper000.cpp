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

#include "mapper000.h"
#include "disk.h"

void Mapper000::reset()
{
	NesMapper::reset();

	nesSetRom32KBank(0);

	u32 crc = nesDiskCrc;
	if (crc == 0x4e7db5af) {	// Circus Charlie(J)
		nesEmuSetRenderMethod(NesEmu::PostRender);
	}
	if (crc == 0x57970078) {	// F-1 Race(J)
		nesEmuSetRenderMethod(NesEmu::PostRender);
	}
	if (crc == 0xaf2bbcbc		// Mach Rider(JU)
	 || crc == 0x3acd4bf1) {	// Mach Rider(Alt)(JU)
		nesEmuSetRenderMethod(NesEmu::PostRender);
	}
}
