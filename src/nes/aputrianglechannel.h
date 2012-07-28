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

#ifndef NESAPUTRIANGLECHANNEL_H
#define NESAPUTRIANGLECHANNEL_H

#include "apuchannel.h"

class NesApuTriangleChannel
{
public:
	enum Register {
		ControlReg,
		UnusedReg,
		WaveLengthLow,
		WaveLengthHigh
	};

	enum Reg0 {
		Reg0LinearCounterLoad	= 0x7F,
		Reg0LinearCounterStart	= 0x80
	};

	void reset();

	void setActive(bool on);
	void write(int addr, u8 data);
	void update(bool clock2nd);
	int render(int cycleRate);

	void syncSetActive(bool on);
	bool syncIsActive() const;
	void syncWrite(int addr, u8 data);
	void syncUpdate(bool clock2nd);

	void sl();
private:
	u8 m_regs[4];
	bool m_counterReload;
	int m_linearCounter;

	int m_timer;
	int m_frequency;
	int m_output;
	int m_adder;

	NesApuLengthCounter m_lengthCounter;

	u8 m_syncRegs[4];
	bool m_syncCounterReload;
	int m_syncLinearCounter;
	NesApuLengthCounter m_syncLengthCounter;
};

inline bool NesApuTriangleChannel::syncIsActive() const
{ return m_syncLengthCounter.count(); }

#endif // NESAPUTRIANGLECHANNEL_H
