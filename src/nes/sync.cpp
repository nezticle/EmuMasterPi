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

#include "sync.h"
#include "sync_p.h"
#include "ppu.h"
#include "nes.h"
#include "mapper.h"
#include "input.h"
#include "inputzapper.h"
#include "apu.h"
#include <QtCore/QSemaphore>
#if defined(ENABLE_DEBUGGING)
#include "debug.h"
#endif

/*
	This file contains asm generator, which creates a frame rendering loop.
	The loop is driven by cpu emulation and synchronizes all other subsystems,
	such as ppu, apu, etc. It takes care about cycles, timing and synchronization
	with thread that requests frames by NesEmu::emulateFrame(). Frame rendering
	loop will be called synchronization loop later as well.
 */

static QSemaphore *producerSem;
static QSemaphore *consumerSem;

static NesSyncData syncData __attribute__((aligned(4)));
static NesSyncCpuRunner *syncCpuRunner = 0;
static AnonymousMemMapping *memMapping = 0;

/*! A stub to update zapper hit from asm code. */
static void updateZapperStub()
{
	nesInputZapper.setScanlineHit(nesPpuScanline == nesInputZapper.pos().y());
}

/*!
	A stub needed by asm generated frame renderer to synchronize with main
	thread (producer-consumer) and to init frame rendering. Function
	returns an event which should be handled by frame rendering loop.
 */
static int beginStub()
{
	producerSem->acquire();
#if defined(ENABLE_DEBUGGING)
	nesDebugPostInit();
#endif

	nesInputSyncWithHost(nesEmu.input());
	nesApuBeginFrame();

	int ev = syncData.cpuEvent;
	syncData.cpuEvent = NesCpuBase::SaveStateEvent;
	return ev;
}

/*!
	A stub needed by asm generated frame renderer to finalize frame and to
	signal main thread that a frame was generated (producer-consumer).
*/
static void endStub()
{
	nesApuProcessFrame();

	consumerSem->release();
}

/*! \class NesSyncCpuRunner
	The NesSyncCpuRunner class is a thread where cpu emulation runs within.
 */

/*!
	Creates a NesSyncCpuRunner object, takes frame generator sync function
	\a nesSync as an argument.
 */
NesSyncCpuRunner::NesSyncCpuRunner(NesSync *nesSync) :
	m_nesSync(nesSync)
{
	start();
}

/*! Cpu emulation loop is running here. */
void NesSyncCpuRunner::run()
{
	nesCpu->run(m_nesSync);
}

/*!
	Initializes synchronization loop: generates asm code and starts a thread
	with cpu emulation.
 */
bool nesSyncInit(QString *error)
{
	producerSem = new QSemaphore();
	consumerSem = new QSemaphore();

	syncData.fillWithFuctions();

	// allocate a page for compiled code
	int memSize = 16*KB;
	memMapping = new AnonymousMemMapping(memSize);
	if (!memMapping->address()) {
		*error = EM_MSG_MMAP_FAILED;
		return false;
	}

	NesSyncCompiler compiler;
	NesSync *nesSync = compiler.recompile();
	Q_ASSERT(nesSync != 0);

	syncCpuRunner = new NesSyncCpuRunner(nesSync);
	return true;
}

/*! Shutdowns synchronization loop and terminates cpu emulation. */
void nesSyncShutdown()
{
	syncData.cpuEvent = NesCpuBase::ExitEvent;
	producerSem->release();
	syncCpuRunner->wait();
	delete syncCpuRunner;
	delete memMapping;
}

/*!
	Resets state of synchronization loop and informs cpu emulation that
	it should reload the state from base class.
*/
void nesSyncReset()
{
	syncData.cpuCycleCounter = 0;
	syncData.baseCycleCounter = 0;
	// load reset state in cpu emulation
	syncData.cpuEvent = NesCpuBase::LoadStateEvent;
}

/*! Requests a frame to be produced by synchronization loop. */
void nesSyncFrame(bool drawEnabled)
{
	// zapper is rarely used, but once used it forces drawing
	if (nesInput.extraDevice() == NesInput::Zapper)
		drawEnabled = true;

	syncData.drawEnabled = drawEnabled;
	producerSem->release();
	consumerSem->acquire();
}

/*! This function is used when a data is written to APU for correct timing. */
u64 nesSyncCpuCycles()
{
	return syncData.cpuCycleCounter + nesCpu->ticks();
}

void nesSyncSl()
{
	emsl.begin("sync");
	emsl.var("cpuCycleCounter", syncData.cpuCycleCounter);
	emsl.var("baseCycleCounter", syncData.baseCycleCounter);
	emsl.end();
	// load state in cpu emulation
	if (!emsl.save)
		syncData.cpuEvent = NesCpuBase::LoadStateEvent;
}

NesSync *NesSyncCompiler::recompile()
{
	initRenderType();
	// create asm compiler
	m_masm = new MacroAssembler(memMapping->address(),
								memMapping->size());

	m_dataBase = r10;
	m_additionalCpuCycles = r9;
	m_regList = m_dataBase.bit() | m_additionalCpuCycles.bit();
#if defined(FRAME_POINTER_FOR_GDB)
	m_regList |= fp.bit();
#else
	m_regList |= fp.bit(); // save mInternalFlagsCopy
#endif

	// compile the code
	__ bind(&m_processScanlineLabel);
	mPpuProcessScanlineFunction();

	__ bind(&m_nesSyncLabel);
	mEntryPoint();

	Label begin;
	__ bind(&begin);
	syncData.nextPc = reinterpret_cast<u32>(memMapping->address()) + begin.pos();

	mBegin();
	mEmulateFrame();
	mEnd();

	__ b(&begin);

	// finish code generation
	m_masm->flush();
	Cpu::flushICache(memMapping->address(), m_masm->pcOffset());
	memMapping->changeRights(true, false, true);
	delete m_masm;

	// return address pointing to the generated nesSync() function
	return reinterpret_cast<NesSync *>((u8*)memMapping->address() +
									   m_nesSyncLabel.pos());
}

void NesSyncCompiler::mCallCFunction(int offsetInData)
{
#if defined(CAN_USE_ARMV7_INSTRUCTIONS)
	__ mov(ip, Operand(*(u32 *)((u8 *)&syncData + offsetInData)));
	__ blx(ip);
#else
	__ ldr(ip, MemOperand(m_dataBase, offsetInData));
	__ blx(ip);
#endif
}

void NesSyncCompiler::mEntryPoint()
{
	// r0 = additionalCpuCycles - the amount of cycles that CPU emulation
	//      executed minus requested cycles to execute

	__ stm(db_w, sp, m_regList | lr.bit());
#if defined(FRAME_POINTER_FOR_GDB)
	__ add(fp, sp, Operand(3*4)); // point to lr - omit r9,r10,fp
#endif
	// one step before we passed an amount of cycles that CPU should run,
	// but it's almost impossible to clock CPU by exact cycles, it will
	// often run more, and we must track these additional cycles
	// save additional CPU cycles, they will be used later in mClock
	__ mov(m_additionalCpuCycles, r0);
	// jump to the point the frame generation was before
	__ mov(m_dataBase, Operand(reinterpret_cast<int>(&syncData)));
	__ ldr(pc, MemOperand(m_dataBase, offsetof(NesSyncData,nextPc)));
}

void NesSyncCompiler::mLeaveToCpu()
{
	__ add(r3, pc, Operand(Assembler::kInstrSize));
	__ str(r3, MemOperand(m_dataBase, offsetof(NesSyncData,nextPc)));
	// return to cpu emulation
	__ ldm(ia_w, sp, m_regList | pc.bit());
	// nesSyncRecData.nextPc points here now
}

void NesSyncCompiler::mBegin()
{
	mCallCFunction(offsetof(NesSyncData,begin));
	Label noEvent;
	// r0 = event
	__ mov(r0, r0, SetCC);
	__ b(&noEvent, pl); // if >= 0 there is no event or SaveEvent which is done
						// always at the end of the synchronization loop
	mLeaveToCpu();
	__ bind(&noEvent);
}

void NesSyncCompiler::mEnd()
{
	// be sure that additionalCpuCycles are stored here because we are going
	// to save the current state
	mClock(0);
	__ mov(r0, Operand(NesCpuBase::SaveStateEvent));
	mLeaveToCpu();
	// save state here to the base class
	// r0 = 0, because mClock returns 0
	mCallCFunction(offsetof(NesSyncData,end));
}

extern "C" u64 __aeabi_uldivmod(u64 n, u64 d);

void NesSyncCompiler::mClock(int ppuCycles)
{
	/*
		Here the compiler will generate following function:

		syncData.baseCycleCounter += baseCycles;
		u64 cpuCyclesNow = syncData.baseCycleCounter / nesEmu.clockDividerForCpu();
		syncData.cpuCycleCounter += additionalCpuCycles;
		int newCpuCycles = cpuCyclesNow - syncData.cpuCycleCounter;
		if (newCpuCycles > 0) {
			syncData.cpuCycleCounter += newCpuCycles;
			return newCpuCycles;
		}
		return 0;

		If return value != 0 it will return to the cpu emulation also.
	*/
	int baseCycles = ppuCycles * nesEmu.clockDividerForPpu();
	Q_ASSERT(baseCycles >= 0);
	__ Ldrd(r0, r1, MemOperand(m_dataBase, offsetof(NesSyncData,baseCycleCounter)));
	__ add(r0, r0, Operand(baseCycles), SetCC);
	__ adc(r1, r1, Operand(0));
	__ Strd(r0, r1, MemOperand(m_dataBase, offsetof(NesSyncData,baseCycleCounter)));

	if (nesEmu.clockDividerForCpu() == 12
	#if !defined(CAN_USE_ARMV7_INSTRUCTIONS)
		|| nesEmu.clockDividerForCpu() == 16
	#endif
		) {
		__ mov(r2, Operand(nesEmu.clockDividerForCpu()));
		__ mov(r3, Operand(0));
		u8 *uldiv = reinterpret_cast<u8 *>(&__aeabi_uldivmod);
		__ mov(ip, Operand(reinterpret_cast<u32>(uldiv)));
		__ blx(ip);
#if defined(CAN_USE_ARMV7_INSTRUCTIONS)
	} else if (nesEmu.clockDividerForCpu() == 16) {
		__ bfi(r0, r1, 0, 4);
		__ mov(r0, Operand(r0, ROR, 4));
		__ mov(r1, Operand(r1, LSR, 4));
#endif
	} else {
		UNREACHABLE();
	}

	__ Ldrd(r2, r3, MemOperand(m_dataBase, offsetof(NesSyncData,cpuCycleCounter)));
	__ add(r2, r2, m_additionalCpuCycles, SetCC);
	__ adc(r3, r3, Operand(0));
	__ Strd(r2, r3, MemOperand(m_dataBase, offsetof(NesSyncData,cpuCycleCounter)));

	// clear additionalCpuCycles here because mClock can be executed multiple
	// times in single synchronization step
	__ mov(m_additionalCpuCycles, Operand(0));

	__ sub(r0, r0, r2);
	__ cmp(r0, Operand(0));
	__ mov(r0, Operand(0), LeaveCC, le);

	Label holdCpu;
	__ b(&holdCpu, le);

	__ add(r2, r2, r0, SetCC);
	__ adc(r3, r3, Operand(0));
	__ Strd(r2, r3, MemOperand(m_dataBase, offsetof(NesSyncData,cpuCycleCounter)));

	mLeaveToCpu();

	__ bind(&holdCpu);
}

void NesSyncCompiler::mClockLine()
{
	if (m_renderAll)
		mClock(NesPpu::ScanlineCycles);
	else
		mClock(NesPpu::HDrawCycles);
}

void NesSyncCompiler::mEmulateVisibleScanline(bool process)
{
	if (process) {
		if (m_renderPost)
			mClockLine();
		__ bl(&m_processScanlineLabel);
		if (m_renderPre)
			mClockLine();
	}

	mPpuProcessScanlineNext();

	bool tileAndHSync = (m_renderTile && nesMapper->hasHorizontalSync());

	if (tileAndHSync)
		mClock(NesPpu::FetchCycles*10);

	mMapperHorizontalSync();

	if (!m_renderAll) {
		if (tileAndHSync)
			mClock(NesPpu::FetchCycles*22);
		else
			mClock(NesPpu::FetchCycles*32);
	}

	mPpuProcessScanlineStart();

	if (!m_renderAll)
		mClock(NesPpu::FetchCycles*10 + NesPpu::ScanlineEndCycles);

	mUpdateZapper();
	mPpuNextScanline();
}

void NesSyncCompiler::mEmulateHiddenScanline(bool zapperHere)
{
	if (m_renderAll || !nesMapper->hasHorizontalSync()) {
		mClock(NesPpu::ScanlineCycles);
		mMapperHorizontalSync();
	} else {
		mClock(NesPpu::HDrawCycles);
		mMapperHorizontalSync();
		mClock(NesPpu::HBlankCycles);
	}
	if (zapperHere)
		mUpdateZapper();
	mPpuNextScanline();
}

void NesSyncCompiler::mEmulateFrame()
{
	mClockLine();
	mPpuProcessFrameStart();
	mEmulateVisibleScanline(false);

// do {
	Label processedLines;
	__ bind(&processedLines);

	mEmulateVisibleScanline(true);

	__ cmp(r0, Operand(NesPpu::VisibleScreenHeight));
	__ b(&processedLines, lt);
// } while (nesPpuScanline < NesPpu::VisibleScreenHeight)

	mMapperVerticalSync();
	mEmulateHiddenScanline(true);
	mPpuSetVBlank(true);

// do {
	Label hiddenLines;
	__ bind(&hiddenLines);

	mEmulateHiddenScanline(true);

	__ cmp(r0, Operand(nesPpu.scanlineCount()-1));
	__ b(&hiddenLines, lt);
// } while (nesPpuScanline < totalScanlines-1)

	mPpuSetVBlank(false);
	mEmulateHiddenScanline(false);
}

void NesSyncCompiler::mPpuProcessFrameStart()
{
	mCallCFunction(offsetof(NesSyncData,ppuProcessFrameStart));
}

void NesSyncCompiler::mPpuSetVBlank(bool on)
{
	__ mov(r0, Operand(static_cast<int>(on)));
	mCallCFunction(offsetof(NesSyncData,ppuSetVBlank));
}

void NesSyncCompiler::mPpuNextScanline()
{
	mCallCFunction(offsetof(NesSyncData,ppuNextScanline));
}

void NesSyncCompiler::mPpuProcessScanlineStart()
{
	mCallCFunction(offsetof(NesSyncData,ppuProcessScanlineStart));
}

void NesSyncCompiler::mPpuProcessScanlineNext()
{
	mCallCFunction(offsetof(NesSyncData,ppuProcessScanlineNext));
}

void NesSyncCompiler::mMapperVerticalSync()
{
	if (nesMapper->hasVerticalSync())
		mCallCFunction(offsetof(NesSyncData,mapperVerticalSync));
}

void NesSyncCompiler::mMapperHorizontalSync()
{
	if (nesMapper->hasHorizontalSync())
		mCallCFunction(offsetof(NesSyncData,mapperHorizontalSync));
}

void NesSyncCompiler::mUpdateZapper()
{
	if (nesInput.extraDevice() == NesInput::Zapper)
		mCallCFunction(offsetof(NesSyncData,updateZapper));
}

void NesSyncCompiler::initRenderType()
{
	m_renderPre = (nesEmuRenderMethod & 1);
	m_renderPost = !m_renderPre;
	m_renderAll = (nesEmuRenderMethod < NesEmu::PostRender);

	m_renderTile = (nesEmuRenderMethod == NesEmu::TileRender);
	if (m_renderTile) {
		m_renderPost = false;
		m_renderPre = false;
		m_renderAll = false;
	}
}

void NesSyncData::fillWithFuctions()
{
	begin					= &beginStub,
	end						= &endStub,
	ppuProcessFrameStart	= &nesPpuProcessFrameStart;
	ppuSetVBlank			= &nesPpuSetVBlank;
	ppuNextScanline			= &nesPpuNextScanline;
	ppuProcessScanlineStart	= &nesPpuProcessScanlineStart;
	ppuProcessScanlineNext	= &nesPpuProcessScanlineNext;
	mapperVerticalSync		= nesMapper->verticalSync;
	mapperHorizontalSync	= nesMapper->horizontalSync;
	updateZapper			= &updateZapperStub;

	ppuCheckSprite0			= &nesPpuCheckSprite0HitHere;
	ppuDummyScanline		= &nesPpuProcessDummyScanline;

	ppuDrawPrologue			= &nesPpuDrawPrologue;
	ppuDrawEpilogue			= &nesPpuDrawEpilogue;
	ppuDrawBackgroundTile	= &nesPpuDrawBackgroundTile;
	ppuDrawBackgroundLine	= &nesPpuDrawBackgroundLine;
}
