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

#include "apurectanglechannel.h"
#include "nes.h"
#include <qmath.h>

void NesApuRectangleChannel::reset()
{
	memset(this, 0, sizeof(NesApuRectangleChannel));
	for (int i = 0; i < 4; i++)
		write(i, 0);
	m_envelope.reset();
	m_lengthCounter.reset();

	for (int i = 0; i < 4; i++)
		syncWrite(i, 0);
	m_syncEnvelope.reset();
	m_syncLengthCounter.reset();
}

void NesApuRectangleChannel::setActive(bool on)
{
	m_lengthCounter.setEnabled(on);
}

void NesApuRectangleChannel::write(int addr, u8 data)
{
	Q_ASSERT(addr >= 0 && addr <= 3);
	m_regs[addr] = data;

	switch (addr) {
	case EnvelopeReg:
		m_envelope.write(data);
		m_duty = m_dutyLut[data>>6];
		break;
	case SweepReg:
		m_sweepEnable = data & 0x80;
		m_sweepIncrease = !(data & 0x08);
		m_sweepShift = data & 0x07;
		m_sweepRate = ((data&0x70) >> 4) + 1;

		m_frequencyLimit = m_frequencyLimitLut[m_sweepShift];
		break;
	case WaveLengthLow:
		m_frequency = (m_frequency & ~0xFF) | data;
		break;
	case WaveLengthHigh:
		m_frequency = ((data&0x07) << 8) | (m_frequency & 0xFF);
		m_envelope.resetClock();
		m_lengthCounter.write(data);
		m_adder = 0;
		break;
	}
}

void NesApuRectangleChannel::update(bool clock2nd, int complement)
{
	if (m_lengthCounter.count()) {
		if (!clock2nd) {
			if (!m_envelope.isLooping())
				m_lengthCounter.clock();

			clockSweep(complement);
		}
		m_envelope.clock();
	}
}

void NesApuRectangleChannel::clockSweep(int complement)
{
	if (m_sweepEnable && m_sweepShift) {
		if (m_sweepCounter)
			m_sweepCounter--;
		if (!m_sweepCounter) {
			m_sweepCounter = m_sweepRate;

			int shifted = m_frequency>>m_sweepShift;
			if (m_sweepIncrease)
				m_frequency += shifted;
			else
				m_frequency += complement - shifted;
		}
	}
}

int NesApuRectangleChannel::render(int cycleRate)
{
	if (!m_lengthCounter.count())
		return 0;
	if (m_frequency < 8)
		return 0;
	if (m_sweepIncrease && m_frequency > m_frequencyLimit)
		return 0;

	qreal sampleWeight = qMin(m_timer, cycleRate);
	qreal total = (m_adder < m_duty) ? sampleWeight : -sampleWeight;

	int freqFixed = IntToFixed(m_frequency+1);
	m_timer -= cycleRate;
	while (m_timer < 0) {
		m_timer += freqFixed;
		m_adder = (m_adder+1) & 0x0F;

		sampleWeight = freqFixed;
		if (m_timer > 0)
			sampleWeight -= m_timer;
		total += (m_adder < m_duty) ? sampleWeight : -sampleWeight;
	}
	return floor(total*qreal(m_envelope.output())/cycleRate + 0.5f);
}

void NesApuRectangleChannel::syncSetActive(bool on)
{
	m_syncLengthCounter.setEnabled(on);
}

void NesApuRectangleChannel::syncWrite(int addr, u8 data)
{
	Q_ASSERT(addr >= 0 && addr <= 3);
	switch (addr) {
	case EnvelopeReg:
		m_syncEnvelope.write(data);
		break;
	case SweepReg:
		break;
	case WaveLengthLow:
		break;
	case WaveLengthHigh:
		m_syncLengthCounter.write(data);
		break;
	}
}

void NesApuRectangleChannel::syncUpdate(bool clock2nd)
{
	if (!clock2nd) {
		if (m_syncLengthCounter.count()) {
			if (!m_syncEnvelope.isLooping())
				m_syncLengthCounter.clock();
		}
	}
}

void NesApuRectangleChannel::sl()
{
	emsl.array("regs", m_regs, sizeof(m_regs));

	if (!emsl.save) {
		for (int i = 0; i < 4; i++)
			write(i, m_regs[i]);
	}

	emsl.var("sweepCounter", m_sweepCounter);
	emsl.var("adder", m_adder);
	emsl.var("timer", m_timer);

	m_envelope.sl();
	m_lengthCounter.sl();

	if (!emsl.save) {
		m_syncEnvelope = m_envelope;
		m_syncLengthCounter = m_lengthCounter;
	}
}

const u8 NesApuRectangleChannel::m_dutyLut[4] =
{
	2, 4, 8, 12
};

const int NesApuRectangleChannel::m_frequencyLimitLut[8] =
{
	0x03FF, 0x0555, 0x0666, 0x071C, 0x0787, 0x07C1, 0x07E0, 0x07F0
};
