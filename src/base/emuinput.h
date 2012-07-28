/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef EMUINPUT_H
#define EMUINPUT_H

#include "base_global.h"

class BASE_EXPORT EmuPad
{
public:
	enum Button {
		Button_Right	= (1 <<  0),
		Button_Down		= (1 <<  1),
		Button_Up		= (1 <<  2),
		Button_Left		= (1 <<  3),

		Button_A		= (1 <<  4),
		Button_B		= (1 <<  5),
		Button_X		= (1 <<  6),
		Button_Y		= (1 <<  7),

		Button_L1		= (1 <<  8),
		Button_R1		= (1 <<  9),
		Button_L2		= (1 << 10),
		Button_R2		= (1 << 11),

		Button_Start	= (1 << 12),
        Button_Select	= (1 << 13),
        Button_Mode     = (1 << 14)
	};
	void setButtons(int buttons);
	int buttons() const;
private:
	int m_buttons;
};

inline void EmuPad::setButtons(int buttons)
{ m_buttons |= buttons; }
inline int EmuPad::buttons() const
{ return m_buttons; }

class BASE_EXPORT EmuKeyb
{
public:
	void enqueue(int key);
	int dequeue();
private:
	int m_keys[4];
};

class BASE_EXPORT EmuMouse
{
public:
	enum Buttons {
		Button_Left		= (1 << 0),
		Button_Right	= (1 << 1),
		Button_Middle	= (1 << 2)
	};

	void setButtons(int buttons);
	int buttons() const;

	void addRel(int x, int y);
	int xRel() const;
	int yRel() const;
private:
	int m_buttons;
	int m_x;
	int m_y;
	int m_pad;
};

inline void EmuMouse::setButtons(int buttons)
{ m_buttons |= buttons; }
inline int EmuMouse::buttons() const
{ return m_buttons; }

inline void EmuMouse::addRel(int x, int y)
{ m_x += x; m_y += y; }
inline int EmuMouse::xRel() const
{ return m_x; }
inline int EmuMouse::yRel() const
{ return m_y; }

class BASE_EXPORT EmuTouch
{
public:
	void setPos(int x, int y);
	int x() const;
	int y() const;
private:
	int m_x;
	int m_y;
};

// +-1 needed when there is no device attached
inline void EmuTouch::setPos(int x, int y)
{ m_x = x+1; m_y = y+1; }
inline int EmuTouch::x() const
{ return m_x-1; }
inline int EmuTouch::y() const
{ return m_y-1; }

class BASE_EXPORT EmuInput
{
public:
	EmuPad pad[2];
	EmuKeyb keyb;
	EmuMouse mouse[2];
	EmuTouch touch;
};

#endif // EMUINPUT_H
