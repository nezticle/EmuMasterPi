#include "apudmchannel.h"
#include "apu.h"
#include "mapper.h"
#include <base/emu.h>

void NesApuDMChannel::reset()
{
	memset(this, 0, sizeof(NesApuDMChannel));
	m_currentFrequencyLut = m_frequencyLut[nesSystemType];
	for (int i = 0; i < 4; i++)
		write(i, 0);
	for (int i = 0; i < 4; i++)
		syncWrite(i, 0);
}

void NesApuDMChannel::setActive(bool on)
{
	if (on) {
		if (!m_dmaLength) {
			restart();
			m_timer = 0;
		}
	} else {
		m_dmaLength = 0;
	}
}

void NesApuDMChannel::write(int addr, u8 data)
{
	Q_ASSERT(addr >= 0 && addr <= 3);
	m_regs[addr] = data;

	switch (addr) {
	case ControlReg:
		m_frequency = m_currentFrequencyLut[data & Reg0Frequency];
		m_looping = data & Reg0Loop;
		break;
	case DacReg:
		m_dac = data & 0x7F;
		break;
	case AddressReg:
		m_startAddress = 0xC000 | (data << 6);
		break;
	case LengthReg:
		m_initialDmaLength = ((data<<4)+1) << 3;
		break;
	}
}

int NesApuDMChannel::render(int cycleRate)
{
	if (m_dmaLength) {
		m_timer -= cycleRate;

		int freqFixed = IntToFixed(m_frequency);
		while (m_timer < 0) {
			m_timer += freqFixed;

			if (!clockDma())
				break;
			// only alter DAC value if the sample buffer has data
			clockDac();
		}
	}
	return m_dac - 0x40;
}

void NesApuDMChannel::clockDac()
{	
	// DAC += (BUFFER&1) ? +2 : -2
	u8 shift = (m_dmaLength&7)^7;
	u8 bit = (m_buffer & (1<<shift)) >> shift;
	u8 next = m_dac + (bit<<2) - 2;
	if (next <= 0x7F)
		m_dac = next;
}

bool NesApuDMChannel::clockDma()
{
	if (!(m_dmaLength&7)) {
		m_buffer = nesCpuRead(m_address);
		m_address = 0x8000 | (m_address+1);
	}

	m_dmaLength--;
	if (!m_dmaLength) {
		if (m_looping)
			restart();
		else
			return false;
	}
	return true;
}

void NesApuDMChannel::restart()
{
	m_address = m_startAddress;
	m_dmaLength = m_initialDmaLength;
}

void NesApuDMChannel::syncSetActive(bool on)
{
	if (on) {
		if (!m_syncDmaLength) {
			m_syncDmaLength = m_syncInitialDmaLength;
			m_syncCycles = 0;
		}
	} else {
		m_syncDmaLength = 0;
		// write to 0x4015 clears irq
		m_syncIrq = false;
	}
}

void NesApuDMChannel::syncWrite(int addr, u8 data)
{
	Q_ASSERT(addr >= 0 && addr <= 3);
	switch (addr) {
	case ControlReg:
		m_syncCyclesDelta = m_currentFrequencyLut[data & Reg0Frequency];
		m_syncCyclesDelta *= 8;
		m_syncLooping = data & Reg0Loop;
		m_syncIrqEnable = data & Reg0IrqEnable;
		if (!m_syncIrqEnable)
			m_syncIrq = false;
		break;
	case DacReg:
		break;
	case AddressReg:
		break;
	case LengthReg:
		m_syncInitialDmaLength = (data<<4) + 1;
		break;
	}
}

void NesApuDMChannel::syncUpdate(int cycles)
{
	if (m_syncDmaLength) {
		m_syncCycles -= cycles;
		while (m_syncCycles < 0) {
			m_syncCycles += m_syncCyclesDelta;
			if (m_syncDmaLength) {
				m_syncDmaLength--;
				if (m_syncDmaLength < 2) {
					if (m_syncLooping) {
						m_syncDmaLength = m_syncInitialDmaLength;
					} else {
						m_syncDmaLength = 0;
						m_syncIrq |= m_syncIrqEnable;
						break;
					}
				}
			}
		}
	}
}

void NesApuDMChannel::sl()
{
	emsl.array("regs", m_regs, sizeof(m_regs));

	if (!emsl.save) {
		for (int i = 0; i < 4; i++) {
			write(i, m_regs[i]);
			syncWrite(i, m_regs[i]);
		}
	}

	emsl.var("dmaLength", m_dmaLength);
	emsl.var("address", m_address);
	emsl.var("irq", m_syncIrq);
	emsl.var("timer", m_timer);
	emsl.var("buffer", m_buffer);
	emsl.var("dac", m_dac);
	emsl.var("syncCycles", m_syncCycles);

	if (!emsl.save)
		m_syncDmaLength = m_dmaLength;
}

const int NesApuDMChannel::m_frequencyLut[2][16] =
{
	{
		0x1AC, 0x17C, 0x154, 0x140,
		0x11E, 0x0FE, 0x0E2, 0x0D6,
		0x0BE, 0x0A0, 0x08E, 0x080,
		0x06A, 0x054, 0x048, 0x036
	},
	{
		0x18E, 0x162, 0x13C, 0x12A,
		0x114, 0x0EC, 0x0D2, 0x0C6,
		0x0B0, 0x094, 0x084, 0x076,
		0x062, 0x04E, 0x042, 0x032
	}
};
