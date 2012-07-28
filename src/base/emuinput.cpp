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

#include "emuinput.h"

/*!
	\class EmuInput
	EmuInput is some sort of proxy between host and emulation. All input devices
	from host write to, while emulation reads from it.
 */

void EmuKeyb::enqueue(int key)
{
	int i = 0;
	for (; i < 4; i++) {
		if (!m_keys[i])
			break;
	}
	if (i < 4)
		m_keys[i] = key;
}

int EmuKeyb::dequeue()
{
	int *keyb = m_keys;
	int key = keyb[0];
	keyb[0] = keyb[1];
	keyb[1] = keyb[2];
	keyb[2] = keyb[3];
	keyb[3] = 0;
	return key;
}
