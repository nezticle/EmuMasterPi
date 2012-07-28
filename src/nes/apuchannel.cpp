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

#include "apuchannel.h"
#include <base/emu.h>

void NesApuLengthCounter::sl()
{
	emsl.var("lengthCounter.count", m_count);
	emsl.var("lengthCounter.enabled", m_enabled);
}

const u8 NesApuLengthCounter::m_lut[32] =
{
	5, 127,   10,   1,   19,   2,   40,   3,
   80,   4,   30,   5,    7,   6,   13,   7,
	6,   8,   12,   9,   24,  10,   48,  11,
   96,  12,   36,  13,    8,  14,   16,  15
};

void NesApuEnvelope::reset()
{
	memset(this, 0, sizeof(NesApuEnvelope));
	m_regs[1] = 0x10;
}

void NesApuEnvelope::clock()
{
	if (!m_reset) {
		if (m_count) {
			m_count--;
			return;
		}
		if (m_regs[0] | (m_regs[1] & 0x20))
			m_regs[0] = (m_regs[0]-1) & 0xF;
	} else {
		m_reset = false;
		m_regs[0] = 0xF;
	}
	m_count = m_regs[1] & 0x0F;
	updateOutput();
}

void NesApuEnvelope::sl()
{
	emsl.var("envelope.regs[0]", m_regs[0]);
	emsl.var("envelope.regs[1]", m_regs[1]);
	emsl.var("envelope.output", m_output);
	emsl.var("envelope.count", m_count);
	emsl.var("envelope.reset", m_reset);
}
