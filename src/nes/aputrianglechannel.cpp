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

#include "aputrianglechannel.h"
#include <base/emu.h>

void NesApuTriangleChannel::reset()
{
	memset(this, 0, sizeof(NesApuTriangleChannel));
	for (int i = 0; i < 4; i++)
		write(i, 0);
	m_lengthCounter.reset();

	for (int i = 0; i < 4; i++)
		syncWrite(i, 0);
	m_syncLengthCounter.reset();
}

void NesApuTriangleChannel::setActive(bool on)
{
	m_lengthCounter.setEnabled(on);
}

void NesApuTriangleChannel::write(int addr, u8 data)
{
	Q_ASSERT(addr >= 0 && addr <= 3);
	m_regs[addr] = data;

	switch (addr) {
	case ControlReg:
		break;
	case UnusedReg:
		break;
	case WaveLengthLow:
		m_frequency = (m_frequency & ~0xFF) | data;
		break;
	case WaveLengthHigh:
		m_frequency = ((data&0x07) << 8) | (m_frequency & 0xFF);
		m_lengthCounter.write(data);
		m_counterReload = true;
		break;
	}
}

void NesApuTriangleChannel::update(bool clock2nd)
{
	if (!m_lengthCounter.isEnabled())
		return;

	bool startCounter = !(m_regs[ControlReg] & Reg0LinearCounterStart);
	if (!clock2nd && startCounter)
		m_lengthCounter.clock();

	if (m_counterReload)
		m_linearCounter = m_regs[0] & 0x7F;
	else if (m_linearCounter)
		m_linearCounter--;

	if (startCounter && m_linearCounter)
		m_counterReload = false;
}

int NesApuTriangleChannel::render(int cycleRate)
{
	if (!m_lengthCounter.count() || !m_linearCounter)
		return m_output;
	if (m_frequency < 8)
		return m_output;

	m_timer -= cycleRate;
	if (m_timer >= 0)
		return m_output;

	int freqFixed = IntToFixed(m_frequency);
	int sum = 0;
	int n = 0;
	while (m_timer < 0) {
		m_timer += freqFixed;
		m_adder = (m_adder+1) & 0x1F;

		if (m_adder < 0x10)
			m_output = (m_adder&0x0F) << 1;
		else
			m_output = (0x0F-(m_adder&0x0F)) << 1;

		sum += m_output;
		n++;
	}
	return sum / n;
}

void NesApuTriangleChannel::syncSetActive(bool on)
{
	m_syncLengthCounter.setEnabled(on);
}

void NesApuTriangleChannel::syncWrite(int addr, u8 data)
{
	Q_ASSERT(addr >= 0 && addr <= 3);
	m_syncRegs[addr] = data;

	switch (addr) {
	case ControlReg:
		break;
	case UnusedReg:
		break;
	case WaveLengthLow:
		break;
	case WaveLengthHigh:
		m_syncLengthCounter.write(data);
		m_syncCounterReload = true;
		break;
	}
}

void NesApuTriangleChannel::syncUpdate(bool clock2nd)
{
	if (!m_syncLengthCounter.isEnabled())
		return;

	bool startCounter = !(m_syncRegs[ControlReg] & Reg0LinearCounterStart);
	if (!clock2nd && startCounter)
		m_syncLengthCounter.clock();

	if (m_syncCounterReload)
		m_syncLinearCounter = m_syncRegs[0] & 0x7F;
	else if (m_syncLinearCounter)
		m_syncLinearCounter--;

	if (startCounter && m_syncLinearCounter)
		m_syncCounterReload = false;
}

void NesApuTriangleChannel::sl()
{
	emsl.array("regs", m_regs, sizeof(m_regs));

	if (!emsl.save) {
		for (int i = 0; i < 4; i++) {
			write(i, m_regs[i]);
			syncWrite(i, m_regs[i]);
		}
	}

	emsl.var("counterReload", m_counterReload);
	emsl.var("linearCounter", m_linearCounter);
	emsl.var("timer", m_timer);
	emsl.var("adder", m_adder);

	m_lengthCounter.sl();

	if (!emsl.save) {
		m_syncLengthCounter = m_lengthCounter;
		m_syncCounterReload = m_counterReload;
		m_syncLinearCounter = m_linearCounter;
	}
}
