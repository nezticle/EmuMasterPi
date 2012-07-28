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

#ifndef NESAPUDPCMCHANNEL_H
#define NESAPUDPCMCHANNEL_H

#include "apuchannel.h"

class NesApuDMChannel
{
public:
	enum Mode { Normal, Loop, Irq };

	enum Register {
		ControlReg,
		DacReg,
		AddressReg,
		LengthReg
	};

	enum Reg0 {
		Reg0Frequency	= 0x0F,
		Reg0Loop		= 0x40,
		Reg0IrqEnable	= 0x80
	};

	void reset();

	void setActive(bool on);
	bool isActive() const;
	void write(int addr, u8 data);
	int render(int cycleRate);

	void syncSetActive(bool on);
	bool syncIsActive() const;
	void syncWrite(int addr, u8 data);
	void syncUpdate(int cycles);

	bool irq() const;
	void clearIrq();

	void sl();
private:
	bool clockDma();
	void clockDac();
	void restart();

	u8 m_regs[4];
	u16 m_dmaLength;
	u16 m_initialDmaLength;
	u16 m_address;
	u16 m_startAddress;
	u8 m_buffer;
	u8 m_looping;
	u8 m_dac;
	u8 m_pad; // unused

	int m_timer;
	int m_frequency;

	bool m_syncIrqEnable;
	bool m_syncIrq;
	int m_syncCycles;
	int m_syncCyclesDelta;
	u16 m_syncDmaLength;
	u16 m_syncInitialDmaLength;
	u8 m_syncLooping;

	const int *m_currentFrequencyLut;

	static const int m_frequencyLut[2][16];
};

inline bool NesApuDMChannel::isActive() const
{ return m_dmaLength; }

inline bool NesApuDMChannel::syncIsActive() const
{ return m_syncDmaLength; }

inline bool NesApuDMChannel::irq() const
{ return m_syncIrq; }

inline void NesApuDMChannel::clearIrq()
{ m_syncIrq = false; }

#endif // NESAPUDPCMCHANNEL_H
