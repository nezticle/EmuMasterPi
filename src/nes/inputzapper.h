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

#ifndef NESINPUTZAPPER_H
#define NESINPUTZAPPER_H

#include "input.h"

class NesInputZapper : public NesInputExtraDevice
{
public:
	void reset();
	u8 read(u16 addr);
	void setScanlineHit(bool on);
	bool wasScanlineHit() const;
	QPoint pos() const;
	void sync(const EmuInput *hostInput);
private:
	bool m_scanlineHit;
	bool m_buttonPressed;
	QPoint m_pos;
};

inline void NesInputZapper::setScanlineHit(bool on)
{ m_scanlineHit = on; }
inline bool NesInputZapper::wasScanlineHit() const
{ return m_scanlineHit; }
inline QPoint NesInputZapper::pos() const
{ return m_pos; }

extern NesInputZapper nesInputZapper;

#endif // NESINPUTZAPPER_H
