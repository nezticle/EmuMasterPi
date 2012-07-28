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

#ifndef NESINPUTPADDLE_H
#define NESINPUTPADDLE_H

#include "input.h"

class NesInputPaddle : public NesInputExtraDevice
{
public:
	void reset();
	void write(u16 addr, u8 data);
	  u8 read(u16 addr);
	void sync(const EmuInput *hostInput);
private:
	u8 m_regs[2];
	u8 m_reg1Cache;
	u8 m_button;
	int m_pos;
};

extern NesInputPaddle nesInputPaddle;

#endif // NESINPUTPADDLE_H
