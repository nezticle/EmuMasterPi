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

#ifndef NESAPURECTANGLECHANNEL_H
#define NESAPURECTANGLECHANNEL_H

#include "apuchannel.h"

class NesApuRectangleChannel
{
public:
	enum Register {
		EnvelopeReg,
		SweepReg,
		WaveLengthLow,
		WaveLengthHigh
	};

	void reset();

	void setActive(bool on);
	void write(int addr, u8 data);
	void update(bool clock2nd, int complement);
	int render(int cycleRate);

	void syncSetActive(bool on);
	bool syncIsActive() const;
	void syncWrite(int addr, u8 data);
	void syncUpdate(bool clock2nd);

	void sl();
private:
	void clockSweep(int complement);

	u8 m_regs[4];
	u8 m_duty;
	u8 m_sweepEnable;
	u8 m_sweepIncrease;
	u8 m_sweepShift;
	u8 m_sweepRate;
	u8 m_sweepCounter;
	u8 m_adder;

	int m_timer;
	int m_frequency;
	int m_frequencyLimit;

	NesApuEnvelope m_envelope;
	NesApuLengthCounter m_lengthCounter;

	NesApuEnvelope m_syncEnvelope;
	NesApuLengthCounter m_syncLengthCounter;

	static const u8 m_dutyLut[4];
	static const int m_frequencyLimitLut[8];
};

inline bool NesApuRectangleChannel::syncIsActive() const
{ return m_syncLengthCounter.count(); }

#endif // NESAPURECTANGLECHANNEL_H
