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

#ifndef NESAPUNOISECHANNEL_H
#define NESAPUNOISECHANNEL_H

#include "apuchannel.h"

class NesApuNoiseChannel
{
public:
	enum Register {
		EnvelopeReg,
		UnusedReg,
		FrequencyReg,
		LengthCounterReg
	};

	enum Reg2 {
		Reg2Frequency	= 0x0F,
		Reg2RandomMode	= 0x80
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
	bool shift();

	u8 m_regs[4];
	uint m_bits;
	uint m_shifter;

	int m_timer;
	int m_frequency;
	int m_output;
	const int *m_currentFrequencyLut;

	NesApuEnvelope m_envelope;
	NesApuLengthCounter m_lengthCounter;

	NesApuEnvelope m_syncEnvelope;
	NesApuLengthCounter m_syncLengthCounter;

	static const int m_frequencyLut[2][16];
};

inline bool NesApuNoiseChannel::syncIsActive() const
{ return m_syncLengthCounter.count(); }

#endif // NESAPUNOISECHANNEL_H
