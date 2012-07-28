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

#include "inputpaddle.h"
#include "ppu.h"

NesInputPaddle nesInputPaddle;

void NesInputPaddle::reset()
{
	m_regs[0] = m_regs[1] = 0;
	m_reg1Cache = 0;
	m_button = 0;
	m_pos = 0;
}

void NesInputPaddle::write(u16 addr, u8 data)
{
	Q_UNUSED(data)
	if (addr == 0) {
		m_regs[0] = 0x00;
		if (m_button)
			m_regs[0] |= 0x02;
		u8 px = 0xFF - (u8)(0x60+m_pos);
		m_regs[1] = 0x00;
		for (int i = 0; i < 8; i++)
			m_regs[1] |= (px&(1<<i)) ? (0x80>>i) : 0;
	}
}

u8 NesInputPaddle::read(u16 addr)
{
	if (addr == 0) {
		return m_regs[0];
	} else {
		u8 data = (m_regs[1]&1) << 1;
		m_regs[1] >>= 1;
		return data;
	}
}

void NesInputPaddle::sync(const EmuInput *hostInput)
{
	int x = hostInput->touch.x();
	int y = hostInput->touch.y();
	if (y != -1) {
		m_pos = qBound(32, x, 223) - 32;
		m_button = (y < (NesPpu::VisibleScreenHeight/2));
	} else {
		m_button = false;
	}
}
