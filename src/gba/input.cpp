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

#include "input.h"
#include "mem.h"

static const int buttonsMapping[10] = {
	EmuPad::Button_A,
	EmuPad::Button_B,
	EmuPad::Button_Select,
	EmuPad::Button_Start,
	EmuPad::Button_Right,
	EmuPad::Button_Left,
	EmuPad::Button_Up,
	EmuPad::Button_Down,
	EmuPad::Button_R1,
	EmuPad::Button_L1
};

void gbaInputUpdate(const EmuInput *input)
{
	int keys = input->pad[0].buttons();
	int gbaKeys = 0x3ff;
	for (int i = 0; i < 10; i++) {
		if (keys & buttonsMapping[i])
			gbaKeys &= ~(1 << i);
	}
	io_registers[REG_P1] = gbaKeys;
}
