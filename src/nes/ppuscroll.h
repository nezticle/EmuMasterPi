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

#ifndef NESPPUSCROLL_H
#define NESPPUSCROLL_H

#include <base/emu.h>

class NesPpuScroll
{
public:
	enum {
		X_TILE    = 0x001F,
		Y_TILE    = 0x03E0,
		Y_FINE    = 0x7000,
		LOW       = 0x00FF,
		HIGH      = 0xFF00,
		NAME      = 0x0C00,
		NAME_LOW  = 0x0400,
		NAME_HIGH = 0x0800
	};

	void clockX();
	void resetX();
	void clockY();
	uint yFine();

	uint address;
	uint toggle;
	uint latch;
	uint xFine;
};

inline void NesPpuScroll::clockX()
{
	if ((address & X_TILE) != X_TILE)
		address++;
	else
		address ^= (X_TILE|NAME_LOW);
}

inline void NesPpuScroll::resetX()
{
	address = (address & ((X_TILE|NAME_LOW) ^ 0x7FFF)) | (latch & (X_TILE|NAME_LOW));
}

inline void NesPpuScroll::clockY()
{
	if ((address & Y_FINE) != Y_FINE) {
		address += 1 << 12;
	} else switch (address & Y_TILE) {
		default:         address = (address & (Y_FINE ^ 0x7FFF)) + (1 << 5); break;
		case (29 << 5): address ^= NAME_HIGH;
		case (31 << 5): address &= (Y_FINE|Y_TILE) ^ 0x7FFF; break;
	}
}

inline uint NesPpuScroll::yFine()
{
	return address >> 12;
}

#endif // NESPPUSCROLL_H
