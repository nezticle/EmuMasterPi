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

#ifndef NESPPUSPRITE_H
#define NESPPUSPRITE_H

#include <base/emu.h>

class NesPpuSprite {
public:
	enum AttributeBit {
		FlipVertically		= 0x80,
		FlipHorizontally	= 0x40,
		BehindBackground	= 0x20,
		HighPaletteBitsMask	= 0x03
	};
	int x() const;
	int y() const;
	u8 tileIndex() const;
	u8 paletteHighBits() const;
	bool flipHorizontally() const;
	bool flipVertically() const;
	bool isBehindBackground() const;
private:
	u8 m_y;
	u8 m_tileIndex;
	u8 m_attributes;
	u8 m_x;

	friend class Nes2C0XPpu;
} Q_PACKED;

inline int NesPpuSprite::x() const
{ return m_x; }
inline int NesPpuSprite::y() const
{ return m_y + 1; }
inline u8 NesPpuSprite::tileIndex() const
{ return m_tileIndex; }
inline u8 NesPpuSprite::paletteHighBits() const
{ return (m_attributes & HighPaletteBitsMask) << 2; }
inline bool NesPpuSprite::flipHorizontally() const
{ return m_attributes & FlipHorizontally; }
inline bool NesPpuSprite::flipVertically() const
{ return m_attributes & FlipVertically; }
inline bool NesPpuSprite::isBehindBackground() const
{ return m_attributes & BehindBackground; }

#endif // NESPPUSPRITE_H
