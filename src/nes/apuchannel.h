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

#ifndef NESAPUCHANNEL_H
#define NESAPUCHANNEL_H

#include <base/emu.h>

static inline int IntToFixed(int val)
{ return val << 16; }

class NesApuLengthCounter
{
public:
	void reset();
	void setEnabled(bool on);
	bool isEnabled() const;
	void write(u8 data);
	void clock();
	u8 count() const;
	void sl();
private:
	u8 m_count;
	u8 m_enabled;

	static const u8 m_lut[32];
};

inline void NesApuLengthCounter::reset()
{ m_count = m_enabled = 0x00; }
inline void NesApuLengthCounter::setEnabled(bool on)
{ m_enabled = on ? 0xFF : 0x00; m_count &= m_enabled; }
inline bool NesApuLengthCounter::isEnabled() const
{ return m_enabled; }
inline void NesApuLengthCounter::write(u8 data)
{ m_count = (m_lut[data>>3]*2) & m_enabled; }
inline void NesApuLengthCounter::clock()
{ if (m_count) m_count--; }
inline u8 NesApuLengthCounter::count() const
{ return m_count; }

class NesApuEnvelope
{
public:
	void reset();
	void resetClock();
	u8 output() const;
	void write(u8 data);
	void clock();
	bool isLooping() const;
	void sl();
private:
	void updateOutput();

	u8 m_regs[2];
	u8 m_output;
	u8 m_count;
	bool m_reset;
};

inline void NesApuEnvelope::resetClock()
{ m_reset = true; }
inline u8 NesApuEnvelope::output() const
{ return m_output; }
inline void NesApuEnvelope::write(u8 data)
{ m_regs[1] = data; updateOutput(); }
inline bool NesApuEnvelope::isLooping() const
{ return m_regs[1] & 0x20; }
inline void NesApuEnvelope::updateOutput()
{ m_output = m_regs[(m_regs[1]>>4) & 1] & 0xF; }

#endif // NESAPUCHANNEL_H
