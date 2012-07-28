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

#ifndef NESAPU_H
#define NESAPU_H

#include <base/emu.h>

extern void nesApuInit();
extern void nesApuReset();
extern void nesApuSetOutputEnabled(bool on);

extern void nesApuWrite(u16 addr, u8 data);
extern   u8 nesApuRead(u16 addr);
extern void nesApuClock(int nCycles);

extern void nesApuBeginFrame();
extern void nesApuProcessFrame();
extern  int nesApuFillBuffer(char *stream, int size);

extern void nesApuSl();

class NesApuDcBlocker
{
public:
	void reset();
	int process(int sample);
private:
	int m_prev;
	int m_next;
	int m_acc;
};

#endif // NESAPU_H
