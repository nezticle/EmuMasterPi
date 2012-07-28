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

#include "apu.h"
#include "ppu.h"
#include "apurectanglechannel.h"
#include "aputrianglechannel.h"
#include "apunoisechannel.h"
#include "apudmchannel.h"
#include "nes.h"
#include "cpubase.h"
#include <base/audioringbuffer.h>

class ApuWrite
{
public:
	u64 time;
	u16 addr;
	u8 data;
	u8 pad;
	u32 pad2;
};

#define SAMPLE_RATE 44100.0f

static NesApuRectangleChannel	r1ch;
static NesApuRectangleChannel	r2ch;
static NesApuTriangleChannel	trch;
static NesApuNoiseChannel		nsch;
static NesApuDMChannel			dmch;

// cycle rate in fixed representation
static int cycleRate;

static bool irqSignal;

static int frameIrqControl;
static bool frameIrq;

static int frameCycle;
static int frameCount;

static QList<ApuWrite> writeQueue;
static qreal elapsedTime;
static qreal cyclesPerSample;
static NesApuDcBlocker dcBlocker;

static AudioRingBuffer<8192> buffer;
static bool outputEnabled;

static inline void enqueueWrite(u16 addr, u8 data);

void nesApuInit()
{
	qreal cpuClock = nesEmu.baseClock() / static_cast<qreal>(nesEmu.clockDividerForCpu());
	qreal ppuFrameCycles = NesPpu::ScanlineCycles * nesPpu.scanlineCount();
	qreal baseFrameCycles = ppuFrameCycles * static_cast<qreal>(nesEmu.clockDividerForPpu());
	qreal cpuCyclesPerSec = baseFrameCycles * nesEmu.systemFrameRate() /
			static_cast<qreal>(nesEmu.clockDividerForCpu());
	cyclesPerSample = cpuCyclesPerSec / SAMPLE_RATE;
	cycleRate = cpuClock * 65536.0f / SAMPLE_RATE;
	outputEnabled = true;
}

void nesApuReset()
{
	r1ch.reset();
	r2ch.reset();
	trch.reset();
	nsch.reset();
	dmch.reset();

	irqSignal = false;

	frameIrqControl = 0xC0;
	frameIrq = false;

	frameCycle = 0;
	frameCount = 0;

	writeQueue.clear();
	nesApuWrite(0x15, 0x00);

	buffer.reset();
	dcBlocker.reset();
}

void nesApuSetOutputEnabled(bool on)
{
	if (outputEnabled != on) {
		outputEnabled = on;
		buffer.reset();
	}
}

static void updateIrqSignal()
{
	bool on = (frameIrq || dmch.irq());
	if (on != irqSignal) {
		irqSignal = on;
		nesCpu->setSignal(NesCpuBase::ApuIrqSignal, on);
	}
}

static void syncClockCounters(bool clock2nd)
{
	r1ch.syncUpdate(clock2nd);
	r2ch.syncUpdate(clock2nd);
	trch.syncUpdate(clock2nd);
	nsch.syncUpdate(clock2nd);

	enqueueWrite(0x18, clock2nd);
}

static void updateFrame()
{
	if (!frameCount) {
		if (!(frameIrqControl&0xC0))
			frameIrq = true;
	}
	if (frameCount == 3) {
		if (frameIrqControl & 0x80)
			frameCycle += 14915;
	}
	syncClockCounters(frameCount & 1);
	frameCount = (frameCount + 1) & 3;
	updateIrqSignal();
}

void nesApuClock(int nCycles)
{
	Q_ASSERT(nCycles > 0);
	frameCycle -= nCycles * 2;
	if (frameCycle <= 0) {
		frameCycle += 14915;
		updateFrame();
	}
	dmch.syncUpdate(nCycles);
	updateIrqSignal();
}

static void syncWrite17(u8 data)
{
	frameIrqControl = data;
	frameIrq = false;
	updateIrqSignal();

	frameCycle = 0;
	frameCount = 0;
	if (data & 0x80)
		updateFrame();
	frameCount = 1;
	frameCycle = 14915;
}

void nesApuWrite(u16 addr, u8 data)
{
	Q_ASSERT(addr <= 0x17);
	switch (addr) {
	case 0x00: case 0x01: case 0x02: case 0x03:
		r1ch.syncWrite(addr&3, data);
		break;
	case 0x04: case 0x05: case 0x06: case 0x07:
		r2ch.syncWrite(addr&3, data);
		break;
	case 0x08: case 0x09: case 0x0A: case 0x0B:
		trch.syncWrite(addr&3, data);
		break;
	case 0x0C: case 0x0D: case 0x0E: case 0x0F:
		nsch.syncWrite(addr&3, data);
		break;
	case 0x10: case 0x11: case 0x12: case 0x13:
		dmch.syncWrite(addr&3, data);
		if (addr == 0x10)
			updateIrqSignal();
		break;
	case 0x15:
		r1ch.syncSetActive(data & 0x01);
		r2ch.syncSetActive(data & 0x02);
		trch.syncSetActive(data & 0x04);
		nsch.syncSetActive(data & 0x08);
		dmch.syncSetActive(data & 0x10);
		updateIrqSignal();
		break;
	case 0x17:
		syncWrite17(data);
		break;
	default:
		break;
	}
	enqueueWrite(addr, data);
}

u8 nesApuRead(u16 addr)
{
	Q_ASSERT(addr <= 0x17);
	u8 data = addr >> 8;
	if (addr == 0x15) {
		data = 0;
		data |= (r1ch.syncIsActive() ? 0x01 : 0);
		data |= (r2ch.syncIsActive() ? 0x02 : 0);
		data |= (trch.syncIsActive() ? 0x04 : 0);
		data |= (nsch.syncIsActive() ? 0x08 : 0);
		data |= (dmch.syncIsActive() ? 0x10 : 0);
		data |= (frameIrq			 ? 0x40 : 0);
		data |= (dmch.irq()			 ? 0x80 : 0);
		frameIrq = false;
		dmch.clearIrq();
		updateIrqSignal();
	} else if (addr == 0x17) {
		if (frameIrq)
			data = 0;
		else
			data |= 0x40;
	}
	return data;
}

static void clockCounters(bool clock2nd)
{
	r1ch.update(clock2nd, -1);
	r2ch.update(clock2nd,  0);
	trch.update(clock2nd);
	nsch.update(clock2nd);
}

static void queuedWrite(u16 addr, u8 data)
{
	Q_ASSERT(addr <= 0x18);
	switch (addr) {
	case 0x00: case 0x01: case 0x02: case 0x03:
		r1ch.write(addr&3, data);
		break;
	case 0x04: case 0x05: case 0x06: case 0x07:
		r2ch.write(addr&3, data);
		break;
	case 0x08: case 0x09: case 0x0A: case 0x0B:
		trch.write(addr&3, data);
		break;
	case 0x0C: case 0x0D: case 0x0E: case 0x0F:
		nsch.write(addr&3, data);
		break;
	case 0x10: case 0x11: case 0x12: case 0x13:
		dmch.write(addr&3, data);
		break;
	case 0x15:
		r1ch.setActive(data & 0x01);
		r2ch.setActive(data & 0x02);
		trch.setActive(data & 0x04);
		nsch.setActive(data & 0x08);
		dmch.setActive(data & 0x10);
		break;
	case 0x17:
		break;
	case 0x18: // special handling for clock update
		clockCounters(data);
		break;
	default:
		break;
	}
}

static inline void enqueueWrite(u16 addr, u8 data)
{
	ApuWrite item;
	item.time = nesSyncCpuCycles();
	item.addr = addr;
	item.data = data;
	writeQueue.append(item);
}

static inline bool processWrite(u64 time)
{
	if (writeQueue.isEmpty())
		return false;
	if (writeQueue.first().time > time)
		return false;
	queuedWrite(writeQueue.first().addr, writeQueue.first().data);
	writeQueue.removeFirst();
	return true;
}

void nesApuBeginFrame()
{
	elapsedTime = nesSyncCpuCycles();
}

void nesApuProcessFrame()
{
    int n = SAMPLE_RATE / nesEmu.frameRate();
    n *= 2;

	for (int i = 0; i < n; i++) {
		bool pending;
		u64 cycles = elapsedTime;
		do {
			pending = processWrite(cycles);
		} while (pending);

		int output = 0;
		output += r1ch.render(cycleRate);
		output += r2ch.render(cycleRate);
		output += trch.render(cycleRate);
		output += nsch.render(cycleRate);
		output += dmch.render(cycleRate);

		output <<= 8;
		output = dcBlocker.process(output);

		s16 sample = qBound(-0x8000, output, 0x7FFF);
		buffer.writeSample(sample, sample);

		elapsedTime += cyclesPerSample;
	}
}

int nesApuFillBuffer(char *stream, int size)
{
	return buffer.fillBuffer(stream, size);
}

#define SL_CHANNEL(name) \
	emsl.begin("apu." #name); \
	name.sl(); \
	emsl.end()

void nesApuSl()
{
	emsl.begin("apu");
	emsl.var("irq", irqSignal);
	emsl.var("frameIrqControl", frameIrqControl);
	emsl.var("frameIrq", frameIrq);
	emsl.var("frameCycle", frameCycle);
	emsl.var("frameCount", frameCount);
	emsl.end();

	SL_CHANNEL(r1ch);
	SL_CHANNEL(r2ch);
	SL_CHANNEL(trch);
	SL_CHANNEL(nsch);
	SL_CHANNEL(dmch);

	if (!emsl.save) {
		writeQueue.clear();
		buffer.reset();
		dcBlocker.reset();
	}
}

//--------------------------------- DC Blocker ---------------------------------

void NesApuDcBlocker::reset()
{
	m_prev = 0;
	m_next = 0;
	m_acc = 0;
}

int NesApuDcBlocker::process(int sample)
{
	m_acc -= m_prev;
	m_prev = sample << 15;
	m_acc += m_prev - m_next*3;
	m_next = m_acc >> 15;
	return m_next;
}
