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

#include "inputzapper.h"
#include "ppu.h"

NesInputZapper nesInputZapper;

void NesInputZapper::reset()
{
	m_pos = QPoint(-1, -1);
	m_buttonPressed = false;
}

u8 NesInputZapper::read(u16 addr)
{
	u8 data = 0x00;
	if (addr == 1) {
		data |= 0x08;
		if (m_buttonPressed)
			data |= 0x10;
		if (m_scanlineHit) {
			QRgb pixel = nesPpuGetPixel(m_pos.x(), m_pos.y());
			int sensor = (qRed(pixel)*302 + qGreen(pixel)*592 + qBlue(pixel)*116);
			sensor /= 1024;
			if (sensor >= 0x40)
				data &= ~0x08;
		}
	}
	return data;
}

void NesInputZapper::sync(const EmuInput *hostInput)
{
	int x = hostInput->touch.x();
	int y = hostInput->touch.y();

	bool offScreen = false;
	offScreen |= (x < 0 || x >= NesPpu::VisibleScreenWidth);
	offScreen |= (y < 0 || y >= NesPpu::VisibleScreenHeight);
	if (offScreen) {
		x = -1;
		y = -1;
	}
	m_buttonPressed = !offScreen;
	if (!offScreen)
		m_pos = QPoint(x, y);

	m_scanlineHit = false;
}
