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

#include "apunoisechannel.h"
#include "nes.h"

void NesApuNoiseChannel::reset()
{
	memset(this, 0, sizeof(NesApuNoiseChannel));
	m_currentFrequencyLut = m_frequencyLut[nesSystemType];
	m_bits = 0x4000;

	for (int i = 0; i < 4; i++)
		write(i, 0);
	m_lengthCounter.reset();
	m_envelope.reset();

	for (int i = 0; i < 4; i++)
		syncWrite(i, 0);
	m_syncLengthCounter.reset();
	m_syncEnvelope.reset();

	m_output = 0;
}

void NesApuNoiseChannel::setActive(bool on)
{
	m_lengthCounter.setEnabled(on);
}

void NesApuNoiseChannel::write(int addr, u8 data)
{
	Q_ASSERT(addr >= 0 && addr <= 3);
	m_regs[addr] = data;
	switch (addr) {
	case EnvelopeReg:
		m_envelope.write(data);
		break;
	case UnusedReg:
		break;
	case FrequencyReg:
		m_frequency = m_currentFrequencyLut[data & Reg2Frequency];
		m_shifter = (data&Reg2RandomMode) ? 6 : 1;
		break;
	case LengthCounterReg:
		m_envelope.resetClock();
		m_lengthCounter.write(data);
		break;
	}
}

void NesApuNoiseChannel::update(bool clock2nd)
{
	if (m_lengthCounter.count()) {
		if (!clock2nd && !m_envelope.isLooping())
			m_lengthCounter.clock();
		m_envelope.clock();
	}
}

bool NesApuNoiseChannel::shift()
{
	bool ret = (m_bits^1) & 1;
	m_bits = (m_bits>>1) | (((m_bits^(m_bits>>m_shifter))&1) << 14);
	return ret;
}

int NesApuNoiseChannel::render(int cycleRate)
{
	if (!m_lengthCounter.count())
		return 0;

	m_timer -= cycleRate;
	if (m_timer >= 0)
		return m_output;

	int freqFixed = IntToFixed(m_frequency);
	int sum = 0;
	int n = 0;
	while (m_timer < 0) {
		m_timer += freqFixed;
		if (shift())
			m_output = m_envelope.output();
		else
			m_output = -m_envelope.output();
		sum += m_output;
		n++;
	}
	return sum / n;
}

void NesApuNoiseChannel::syncSetActive(bool on)
{
	m_syncLengthCounter.setEnabled(on);
}

void NesApuNoiseChannel::syncWrite(int addr, u8 data)
{
	Q_ASSERT(addr >= 0 && addr <= 3);
	switch (addr) {
	case EnvelopeReg:
		m_syncEnvelope.write(data);
		break;
	case UnusedReg:
		break;
	case FrequencyReg:
		break;
	case LengthCounterReg:
		m_syncLengthCounter.write(data);
		break;
	}
}

void NesApuNoiseChannel::syncUpdate(bool clock2nd)
{
	if (!clock2nd) {
		if (m_syncLengthCounter.count()) {
			if (!m_syncEnvelope.isLooping())
				m_syncLengthCounter.clock();
		}
	}
}

void NesApuNoiseChannel::sl()
{
	emsl.array("regs", m_regs, sizeof(m_regs));

	if (!emsl.save) {
		for (int i = 0; i < 4; i++)
			write(i, m_regs[i]);
	}

	emsl.var("timer", m_timer);
	emsl.var("bits", m_bits);

	m_envelope.sl();
	m_lengthCounter.sl();

	if (!emsl.save) {
		m_syncEnvelope = m_envelope;
		m_syncLengthCounter = m_lengthCounter;
	}
}

const int NesApuNoiseChannel::m_frequencyLut[2][16] =
{
	{
		0x004, 0x008, 0x010, 0x020,
		0x040, 0x060, 0x080, 0x0A0,
		0x0CA, 0x0FE, 0x17C, 0x1FC,
		0x2FA, 0x3F8, 0x7F2, 0xFE4
	},
	{
		0x004, 0x007, 0x00E, 0x01E,
		0x03C, 0x058, 0x076, 0x094,
		0x0BC, 0x0EC, 0x162, 0x1D8,
		0x2C4, 0x3B0, 0x762, 0xEC2
	}
};
