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

#include "cpurec.h"
#include "cpurec_p.h"
#include "ppu.h"
#include "apu.h"
#include "cheats.h"
#if defined(ENABLE_DEBUGGING)
#include "debug.h"
#endif
#include <base/memutils.h>

#include "cpurec_addressing.h"
#include "cpurec_flags.h"
#include "cpurec_flow.h"
#include "cpurec_logical.h"
#include "cpurec_math.h"
#include "cpurec_memory.h"
#include "cpurec_shift.h"
#include "cpurec_stack.h"
#include "cpurec_transfer.h"
#include "cpurec_undocummented.h"
#include "cpurec_optimizations.h"

NesCpuRecompiler nesCpuRecompiler;
static NesCpuTranslator translator Q_DECL_ALIGN(4);
static NesCpuRecData cpuRecData Q_DECL_ALIGN(4);
static Label recLabels[0x10000] Q_DECL_ALIGN(16);
static AnonymousMemMapping *memMapping = 0;

/*!
	A stub needed by asm code to call process() member of the translator object.
	\a instrPointer contains value of the 6502.PC register
	\a caller is a pointer to the branch instruction which initiated the
		translation
 */
static void *translateStub(u16 instrPointer, u8 *caller)
{
	return translator.process(instrPointer, caller);
}

/*!
	Initializes translator.
 */
bool NesCpuTranslator::init()
{
	m_labels = recLabels;
	memMapping = new AnonymousMemMapping(BufferSize);
	if (!memMapping->address())
		return false;
	m_codeBuffer = (u8 *)memMapping->address();
	m_masm = new MacroAssembler(m_codeBuffer, BufferSize);

	m_checkAlertAfterInstruction = false;

	// compile some needed stubs
	mEntryPoint();
	mTranslateCaller();
	mSync();
	mAlertHandler();
#if defined(ENABLE_DEBUGGING)
	mDebugStep();
#endif

	// mark pages "used before" here to force page clearing
	m_pageUsedMask = (1<<NesNumOfCpuPages)-1;
	for (int i = 0; i < NesNumOfCpuPages; i++)
		clearPage(i);

	m_lastRecompilationInRam = 0;
	return true;
}

/*!
	Frees resources used by the translator.
 */
void NesCpuTranslator::shutdown()
{
	delete m_masm;
	delete memMapping;
}

/*!
	Reads a byte from 6502 address space pointed by 6502.PC register.
	Increments 6502.PC and returns the byte as result.
 */
inline u8 NesCpuTranslator::fetchPc8()
{
	u8 data = nesCpuReadDirect(m_recPc);
	m_recPc++;
	return data;
}

/*!
	Reads a word from 6502 address space pointed by 6502.PC register.
	Increments 6502.PC and returns the word as result.
 */
inline u16 NesCpuTranslator::fetchPc16()
{
	u8 lo = fetchPc8();
	u8 hi = fetchPc8();
	return lo | (hi<<8);
}

/*!
	Translates 6502 instructions starting from \a instrPointer.
	\a instrPointer contains value of the 6502.PC register
	\a caller is a pointer to the branch instruction which initiated the
		translation
 */
inline void *NesCpuTranslator::process(u16 instrPointer, u8 *caller)
{
	int count = 128;
	if (instrPointer < 0x4000) {
		count = 16;
		memset32(m_labels + m_lastRecompilationInRam,
				 -m_translateCallerLabel.pos()-1,
				 count * 4);
		m_lastRecompilationInRam = instrPointer;
	}
	// check if already translated
	if (m_labels[instrPointer] == m_translateCallerLabel) {
		Q_ASSERT(!m_checkAlertAfterInstruction);
		m_recPc = instrPointer;

		int page = nesCpuPageByAddr(instrPointer);
		int recompiledStart = m_pageTranslationOffset[page];
		m_masm->setPcOffset(recompiledStart);

		// recompile here
		while (m_labels[m_recPc] == m_translateCallerLabel &&
			   nesCpuPageByAddr(m_recPc) == page &&
			   count > 0) {

			m_labels[m_recPc].unuse();
			__ bind(&m_labels[m_recPc]);

#if defined(ENABLE_DEBUGGING)
			__ mov(r0, Operand(currentPc()));
			__ bl(&m_debugStepLabel);
#endif

#if !defined(DISABLE_RECOMPILER_OPTIMIZATIONS)
			if (!mTryOptimize())
#endif
				mSingleInstruction();

			if (m_checkAlertAfterInstruction) {
				mCheckAlert();
				m_checkAlertAfterInstruction = false;
			}

			count--;
		}
		// in the end jump to next instruction label
		u16 endRecPc = m_recPc;
		m_recPc = instrPointer;
		mJump(endRecPc);

		// flush const pool or any other pending data in the assembler
		m_masm->flush();
		// save translation end pointer for this page
		int recompiledEnd = m_masm->pcOffset();
		m_pageTranslationOffset[page] = recompiledEnd;
		Q_ASSERT(page == 0 || recompiledEnd < (page+1) * BlockSize);
		// flush instruction cache
		int recompiledSize = recompiledEnd - recompiledStart;
		Cpu::flushICache(m_codeBuffer + recompiledStart, recompiledSize);
		// mark translation page used
		m_pageUsedMask |= 1 << page;

		// handle section boundary because 6502 has variable length of
		// instructions (m_recPc == 0 on KIL instruction so omit that value)
		int pageNow = nesCpuPageByAddr(m_recPc);
		if (m_recPc && pageNow != page)
			saveTranslationBoundary(page, m_recPc & NesCpuBankMask);
	}
	fixCallerInstruction(instrPointer, caller);
	return m_codeBuffer + m_labels[instrPointer].pos();
}

/*!
	Fixes branch instruction which caused the translation.
 */
inline void NesCpuTranslator::fixCallerInstruction(u16 instrPointer, u8 *caller)
{
	Instr callerInstr = *(Instr *)caller;
	// fix only bl call (blx is from other sections)
	if ((callerInstr & (15 << 24)) == (B27|B25|B24)) {
		int callerOffset = caller - m_codeBuffer;
		Q_ASSERT(m_labels[instrPointer].isBound());
		m_masm->putTargetAt(callerOffset, m_labels[instrPointer].pos());
		Cpu::flushICache(caller, 4);
	}
}

/*!
	Clears translation block used for the specified page \a pageIndex.
 */
void NesCpuTranslator::clearPage(int pageIndex)
{
	Q_ASSERT(pageIndex >= 0 && pageIndex < 8);
	Q_ASSERT(m_translateCallerLabel.isBound());

	if (m_pageUsedMask & (1<<pageIndex)) {
		m_pageUsedMask &= ~(1<<pageIndex);
		// in first page mSync and others stubs will be compiled
		// we can assume there should be no execution in 0x0000-0x3fff
		// so we can move impossible translation of first page to the space
		// of second page
		if (pageIndex == 0) {
			m_pageTranslationOffset[0] = 1 * BlockSize;
			m_pageTranslationOffset[1] = 1 * BlockSize;
		} else {
			m_pageTranslationOffset[pageIndex] = pageIndex * BlockSize;
		}

		int offset = pageIndex * NesCpuBankSize;
		// page 0 and 1 will be cleared only once at start, so no need
		// to call it for both
		memset32(m_labels + offset,
				 -m_translateCallerLabel.pos()-1,
				 NesCpuBankSize);

		m_numOfDataUsedFromNextPage[pageIndex] = 0;
		checkTranslationBoundary(pageIndex);
	}
}

void NesCpuTranslator::saveTranslationBoundary(int currentPageIndex,
											   int numBytesUsedFromNext)
{
	int nextPageIndex = (currentPageIndex+1) & 7;
	m_numOfDataUsedFromNextPage[currentPageIndex] = numBytesUsedFromNext;
	for (int i = 0; i < numBytesUsedFromNext; i++) {
		u8 data = nesCpuReadDirect((nextPageIndex << 13) + i);
		m_dataUsedFromNextPage[currentPageIndex][i] = data;
	}
}

void NesCpuTranslator::checkTranslationBoundary(int pageIndex)
{
	int previousPageIndex = (pageIndex-1) & 7;
	int numUsedFromNextPage = m_numOfDataUsedFromNextPage[previousPageIndex];
	if (numUsedFromNextPage) {
		bool same = true;
		for (int i = 0; i < numUsedFromNextPage; i++) {
			if (nesCpuReadDirect((pageIndex << 13) + i) !=
				m_dataUsedFromNextPage[previousPageIndex][i]) {
				same = false;
				break;
			}
		}
		m_numOfDataUsedFromNextPage[previousPageIndex] = 0;
		if (!same)
			clearPage(previousPageIndex);
	}
}

/*!
	Increments cycle counter \a n times.
 */
inline void NesCpuTranslator::mAddCycles(int n, Condition cond)
{
	__ add(mCycles, mCycles, Operand(n), LeaveCC, cond);
}

/*!
	Stores mCycles to currentCycles member of cpuRecData, exact cycles are
	needed for APU writes.
 */
inline void NesCpuTranslator::mStoreCurrentCycles()
{
	__ str(mCycles, MemOperand(mDataBase,
							   offsetof(NesCpuRecData,currentCycles)));
}

/*!
	Adds cycles from cpuRecData.additionalCycles to mCycles and sets
	cpuRecData.additionalCycles to 0. additionalCycles member holds
	postponed cycles in case of interrupt pending or DMA transfer.
 */
void NesCpuTranslator::mFetchAdditionalCycles()
{
	__ ldr(ip, MemOperand(mDataBase, offsetof(NesCpuRecData,additionalCycles)));
	__ add(mCycles, mCycles, Operand(ip), SetCC);
	__ mov(ip, Operand(0));
	__ str(ip, MemOperand(mDataBase, offsetof(NesCpuRecData,additionalCycles)));
}

/*!
	Converts 32 bit value in register \a reg to 16 bit value.
 */
inline void NesCpuTranslator::mTo16Bit(Register reg)
{
#if defined(CAN_USE_ARMV7_INSTRUCTIONS)
	__ bfc(reg, 16, 16);
#else
	__ mov(reg, Operand(reg, LSL, 16));
	__ mov(reg, Operand(reg, LSR, 16));
#endif
}

/*!
	Loads C function address based on the given \a offset and calls
	this function.
 */
inline void NesCpuTranslator::mCallCFunction(int funcOffset)
{
	__ ldr(ip, MemOperand(mDataBase, funcOffset));
	__ blx(ip);
}

/*!
	Compiles a stub needed to call C++ translator.process() from asm code.
 */
inline void NesCpuTranslator::mTranslateCaller()
{
	// r0 - PC address
	// lr - caller address - lr will not be used to return to the caller,
	//		it's used to fix branch instruction
	__ bind(&m_translateCallerLabel);
	mSaveInternalFlags();

	// adjust lr to point to branch instruction and move it to r1 as an argument
	// for translateStub() function
	__ sub(r1, lr, Operand(4));
	mCallCFunction(offsetof(NesCpuRecData,translate));

	mRestoreInternalFlags();
	// translate function returns address of recompiled code
	// just jump to it
	__ mov(pc, r0);
}

/*!
	Compiles a code that loads address pointed by a label at the given \a offset
	in 6502 address space to the \a dst. The label can be bound to the
	recompiled code or to mTranslateCaller if translation haven't occured for
	the given \a offset.
 */
inline void NesCpuTranslator::mLoadLabelAddress(Register offset, Register dst)
{
	Q_ASSERT(offset != ip && dst != ip);

	__ mov(dst, Operand(intptr_t(m_codeBuffer)));
	__ mov(ip, Operand(intptr_t(m_labels)));
	__ ldr(ip, MemOperand(ip, offset, LSL, 2));
	// dst = buffer - pos - 1
	__ sub(dst, dst, Operand(ip));
	__ bic(dst, dst, Operand(3));
}

void NesCpuTranslator::mSaveInternalFlags()
{
#if !defined(FRAME_POINTER_FOR_GDB)
	__ mrs(mInternalFlagsCopy, CPSR);
#else
	__ mrs(ip, CPSR);
	__ str(ip, MemOperand(mDataBase, offsetof(NesCpuRecData,internalFlagsCopy)));
#endif
}

void NesCpuTranslator::mRestoreInternalFlags()
{
#if !defined(FRAME_POINTER_FOR_GDB)
	__ msr(CPSR_f, Operand(mInternalFlagsCopy));
#else
	__ ldr(ip, MemOperand(mDataBase, offsetof(NesCpuRecData,internalFlagsCopy)));
	__ msr(CPSR_f, Operand(ip));
#endif
}

inline void NesCpuTranslator::mCheckSync()
{
	// r0 - 6502.PC address of the next instruction

	// a clever trick to jump over the synchronization call without using flags
	__ mov(ip, Operand(mCycles, LSR, 31));
	__ add(pc, pc, Operand(ip, LSL, 2));
	// some pad needed - write current pc
	__ dd(currentPc());
	// sync if needed
	__ bl(&m_syncLabel);
	// continue otherwise
}

void NesCpuTranslator::mHandleEvent()
{
	Label saveState;
	Label loadState;
	Label exit;

	// arguments: r1 = event

	// reg list for saving state
	// r0 contains 6502.PC address
	// r2 contains 6502.P flags
	RegList regList =	r0.bit() | r2.bit() | mA.bit() |
						mX.bit() | mY.bit() | mS.bit();

	// regs member of NesCpuRecData should be placed at the start of the object
	Q_ASSERT(offsetof(NesCpuRecData,regs) == 0);
	__ cmp(r1, Operand(NesCpuBase::SaveStateEvent));
	__ b(&saveState, eq);
	__ cmp(r1, Operand(NesCpuBase::LoadStateEvent));
	__ b(&loadState, eq);

// exitEvent:
	mExitPoint();

// loadState:
	__ bind(&loadState);
	__ ldm(ia, mDataBase, regList);
	mUnpackFlags(r2, r1);
	mSaveInternalFlags();
	mLoadLabelAddress(r0, lr);
	// mCycles is zeroed later
	__ b(&exit);

// saveState:
	__ bind(&saveState);
	mRestoreInternalFlags();
	mPackFlags(r2);
	__ stm(ia, mDataBase, regList);

	// saveState occurs on frame end, ticks() will be used on new frame
	// so set it to zero
	__ mov(ip, Operand(0));
	__ str(ip, MemOperand(mDataBase, offsetof(NesCpuRecData,currentCycles)));

// exit:
	__ bind(&exit);
}

/*!
	PUSH16(PC)
	PUSH8((P&~B) | U)
	P |= I
	PC = READ16(vector)
	ADDCYC(7)
 */
void NesCpuTranslator::mHwInterrupt()
{
	// r0 - 6502.PC address of interrupted instruction
	// r1 - vector addresss - selects (nmi, irq, reset) which interrupt will
	//        be executed
	// as a result lr register is loaded with the address of code

	mPush16(r0);
	mRestoreInternalFlags();
	mPackFlags(r0);
	__ bic(r0, r0, Operand(NesCpuBase::Break));
	__ orr(r0, r0, Operand(NesCpuBase::Unused));
	mPush8(r0);
	mSei();
	mAddCycles(7);

	// new address will be loaded, r0 must point to PC address as translation
	//   can occur for the given interrupt
	mReadDirect8(r1, r0);
	__ add(r1, r1, Operand(1));
	mReadDirect8(r1, r2);
	__ orr(r0, r0, Operand(r2, LSL, 8));
	// load lr with label pos
	mLoadLabelAddress(r0, lr);
}

void NesCpuTranslator::mHandleInterrupts()
{
	// code at the bottom does following job:
	//
	// if (interrupts & mNmiInterrupt) {
	//     hwInterrupt(NmiVectorAddress);
	//     interrupts &= ~mNmiInterrupt;
	// } else if (interrupts & mIrqInterrupt) {
	//     if (!(mFlags & mIrqDisable)) {
	//         hwInterrupt(IrqVectorAddress);
	//     }
	// }

	// r0 - 6502.PC address of the next instruction

	Label exitInterruptCheck;
	Label irqPending;
	Label doInterrupt;

	__ ldr(r1, MemOperand(mDataBase, offsetof(NesCpuRecData,interrupts)));
	__ mov(r1, r1, SetCC);
	__ b(&exitInterruptCheck, eq);

	__ tst(r1, Operand(NesCpuBase::NmiInterrupt));
	__ b(&irqPending, eq);

	// NMI is handled here
	// clear NMI flag
	__ bic(r1, r1, Operand(NesCpuBase::NmiInterrupt));
	__ str(r1, MemOperand(mDataBase, offsetof(NesCpuRecData,interrupts)));
	__ mov(r1, Operand(NesCpuBase::NmiVectorAddress));
	__ b(&doInterrupt);

// irqPending:
	// IRQ is handled here
	// check if P.I is cleared
	__ bind(&irqPending);
	__ tst(mFlags, Operand(NesCpuBase::IrqDisable));
	__ b(&exitInterruptCheck, ne);
	__ mov(r1, Operand(NesCpuBase::IrqVectorAddress));

// doInterrupt:
	__ bind(&doInterrupt);
	// interrupt occured - handle it
	mHwInterrupt();

// exitInterruptCheck:
	__ bind(&exitInterruptCheck);
}

void NesCpuTranslator::mSync()
{
	// r0 - 6502.PC address of the next instruction

	__ bind(&m_syncLabel);
	Label exitSync;
	Label handleEvent;
	mSaveInternalFlags();
	// IRQ can be cleared, so we must fetch additional cycles every time
	mFetchAdditionalCycles();
	mHandleInterrupts();

	// IRQ may be pending and P.I can be set, in this case we may not call
	// nesSync if mCycles < 0
	__ mov(mCycles, mCycles, SetCC);
	__ b(&m_checkInterruptsForNextInstruction, mi);

	__ bind(&m_syncWithoutInterruptHandling);

	int preserved = r0.bit() | lr.bit();
#if defined(FRAME_POINTER_FOR_GDB)
	preserved |= fp.bit();
#endif
	__ stm(db_w, sp, preserved);
#if defined(FRAME_POINTER_FOR_GDB)
	__ add(fp, sp, Operand(2*4));
#endif

	Label dontSyncWithApuAndClock;
	__ ldr(ip, MemOperand(mDataBase, offsetof(NesCpuRecData,startCycles)));
	__ add(r0, mCycles, ip, SetCC);
	__ b(&dontSyncWithApuAndClock, eq);
	mCallCFunction(offsetof(NesCpuRecData,apuClock));
	if (nesMapper->hasClock()) {
		__ ldr(ip, MemOperand(mDataBase, offsetof(NesCpuRecData,startCycles)));
		__ add(r0, mCycles, ip, SetCC);
		mCallCFunction(offsetof(NesCpuRecData,mapperClock));
	}
	__ bind(&dontSyncWithApuAndClock);

	__ mov(r0, mCycles);
	mCallCFunction(offsetof(NesCpuRecData,nesSync));
	__ str(r0, MemOperand(mDataBase, offsetof(NesCpuRecData,startCycles)));
	__ rsb(mCycles, r0, Operand(0), SetCC);
	__ ldm(ia_w, sp, preserved);

	// if r0 >= 0 then it means we should handle an event
	__ b(&handleEvent, pl);

	// interrupts handling is a little tricky:
	//   - once interrupt occurs save cycles in the memory
	//   - force mSync to be called next time by setting mCycles to zero
	//   - handle interrupt in the new mSync call
	__ bind(&m_checkInterruptsForNextInstruction);
	mClearAlert();
	__ ldr(ip, MemOperand(mDataBase, offsetof(NesCpuRecData,interrupts)));
	__ mov(ip, ip, SetCC);
	__ b(&exitSync, eq);
	// interrupt is pending here
	__ str(mCycles, MemOperand(mDataBase,
							   offsetof(NesCpuRecData,additionalCycles)));
	__ mov(mCycles, Operand(0));
	__ b(&exitSync);

// handleEvent:
	__ bind(&handleEvent);
	__ rsb(r1, mCycles, Operand(0));
	__ mov(mCycles, Operand(0));
	__ str(mCycles, MemOperand(mDataBase, offsetof(NesCpuRecData,startCycles)));
	mHandleEvent();
	__ b(&m_syncWithoutInterruptHandling);

// exitSync:
	__ bind(&exitSync);
	mRestoreInternalFlags();
	// r0 must be loaded here with 6502.PC address of the next instruction
	__ mov(pc, lr);
}

inline void NesCpuTranslator::mCheckAlert()
{
	// check if write caused an alert
	__ ldr(ip, MemOperand(mDataBase, offsetof(NesCpuRecData,alert)));
	__ add(pc, pc, Operand(ip, LSL, 2));
	// some pad needed - write current 6502.PC
	__ dd(currentPc());
	// save state can happen in alert handler - r0 must contain 6502.PC
	__ mov(r0, Operand(currentPc()));
	// handle an alert if needed
	__ bl(&m_alertHandlerLabel);
	// continue otherwise
}

void NesCpuTranslator::mAlertHandler()
{
	__ bind(&m_alertHandlerLabel);
	mSaveInternalFlags();
	// reload lr in case alert has been signaled from mapping change
	mLoadLabelAddress(r0, lr);
	// do not clear alert here, will be cleared in mSync()
	// mClearAlert();
	// add additionalCycles in case DMA transfer occur
	__ b(&m_syncLabel);
	mFetchAdditionalCycles();
	// now we have to options:
	// 1. mCycles >= 0: call nesSync()
	// 2. mCycles <  0: handle interrupts on next instruction
	__ b(&m_syncWithoutInterruptHandling, pl);
	__ b(&m_checkInterruptsForNextInstruction);
}

void NesCpuTranslator::mClearAlert()
{
	__ mov(ip, Operand(NesCpuRecData::AlertOff));
	__ str(ip, MemOperand(mDataBase, offsetof(NesCpuRecData,alert)));
}

void NesCpuTranslator::mEntryPoint()
{
	cpuRecData.entryPointPtr = intptr_t(m_codeBuffer + m_masm->pcOffset());
	// save caller registers and return address

#if defined(FRAME_POINTER_FOR_GDB)
	__ stm(db_w, sp, kCalleeSaved | fp.bit() | lr.bit());
	__ add(fp, sp, Operand(kNumCalleeSaved*4));
#else
	__ stm(db_w, sp, kCalleeSaved | lr.bit());
#endif
	// load base of the data structure needed by the recompiler
	__ mov(mDataBase, Operand(intptr_t(&cpuRecData)));
	// make sure we pass 0 on first nesSync() call
	__ mov(mCycles, Operand(0));
	// jump to sync() which will call nesSync()
	__ b(&m_syncLabel);
}

void NesCpuTranslator::mExitPoint()
{
	// restore registers and return to caller
#if defined(FRAME_POINTER_FOR_GDB)
	__ ldm(ia_w, sp, kCalleeSaved | fp.bit() | pc.bit());
#else
	__ ldm(ia_w, sp, kCalleeSaved | pc.bit());
#endif
}

#if defined(ENABLE_DEBUGGING)
void NesCpuTranslator::mDebugStep()
{
	// regList from mHandleEvent
	RegList regList =	r0.bit() | r2.bit() | mA.bit() |
						mX.bit() | mY.bit() | mS.bit();

	__ bind(&m_debugStepLabel);
	__ push(lr);
//	mStoreCurrentCycles();
	mSaveInternalFlags();
	mPackFlags(r2);
	__ stm(ia, mDataBase, regList);
	mCallCFunction(offsetof(NesCpuRecData,debugStep));
	mRestoreInternalFlags();
	__ pop(pc);
}
#endif

bool NesCpuRecompiler::init(QString *error)
{
	cpuRecData.ram = nesRam;
	cpuRecData.stack = nesRam + StackBase;
	cpuRecData.banks = nesCpuBanks;

	cpuRecData.translate = &translateStub;

	cpuRecData.cpuWrite = &nesCpuWrite;
	cpuRecData.cpuRead = &nesCpuRead;
	cpuRecData.cpuWrite40xx = &nesCpuWrite40xx;
	cpuRecData.cpuRead40xx = &nesCpuRead40xx;
	cpuRecData.cpuWriteLow = nesMapper->writeLow;
	cpuRecData.cpuReadLow = nesMapper->readLow;
	cpuRecData.cpuWriteHigh = nesMapper->writeHigh;

	cpuRecData.ppuWriteReg = &nesPpuWriteReg;
	cpuRecData.ppuReadReg = &nesPpuReadReg;

	cpuRecData.apuClock = &nesApuClock;
	cpuRecData.mapperClock = nesMapper->clock;

	cpuRecData.processCheats = &nesCheatsProcess;

#if defined(ENABLE_DEBUGGING)
	cpuRecData.debugStep = &nesDebugCpuOp;
#endif

	bool ok = translator.init();
	if (!ok)
		*error = "Could not initialize CPU recompiler";
	return ok;
}

void NesCpuRecompiler::shutdown()
{
	translator.shutdown();
}

void NesCpuRecompiler::run(NesSync *nesSync)
{
	cpuRecData.nesSync = nesSync;
	// run() is called before reset(), so we need clear some members in order
	// to bypass interupt checking
	cpuRecData.interrupts = 0;
	// make sure we pass 0 on first nesSync() call
	cpuRecData.startCycles = 0;
	cpuRecData.additionalCycles = 0;
	cpuRecData.currentCycles = 0;
	(*cpuRecData.entryPoint)();
}

void NesCpuRecompiler::reset()
{
	NesCpuBase::reset();
	loadStateFromBase();
}

s32 NesCpuRecompiler::ticks() const
{
	return cpuRecData.currentCycles + cpuRecData.additionalCycles;
}

void NesCpuRecompiler::setSignal(InterruptSignal sig, bool on)
{
#if defined(ENABLE_DEBUGGING)
	if (on) {
		if (sig == NmiSignal)
			nesDebugNmi();
		else
			nesDebugIrq();
	}
#endif
	int oldSignals = cpuRecData.interruptSignals;

	if (on)
		cpuRecData.interruptSignals |=  sig;
	else
		cpuRecData.interruptSignals &= ~sig;

	int changes = oldSignals ^ cpuRecData.interruptSignals;
	if (changes) {
		if (changes & IrqSignalMask) {
			// IRQ is triggered on high level
			setInterrupt(IrqInterrupt, cpuRecData.interruptSignals & IrqSignalMask);
		} else if (changes & NmiSignal) {
			// NMI is triggered on rising edge
			if (cpuRecData.interruptSignals & NmiSignal)
				setInterrupt(NmiInterrupt, true);
		} else {
			UNREACHABLE();
		}
	}
}

void NesCpuRecompiler::dma()
{
	cpuRecData.additionalCycles += NesDmaCycles;
	cpuRecData.alert = NesCpuRecData::AlertOn;
}

void NesCpuRecompiler::clearPage(int pageIndex)
{
	translator.clearPage(pageIndex);
	cpuRecData.alert = NesCpuRecData::AlertOn;
}

#if defined(ENABLE_DEBUGGING)
void NesCpuRecompiler::storeRegistersToBase()
{
	saveStateToBase();
}
#endif

void NesCpuRecompiler::setInterrupt(Interrupt interrupt, bool on)
{
	if (on) {
		cpuRecData.interrupts |=  interrupt;
		cpuRecData.alert = NesCpuRecData::AlertOn;
	} else {
		cpuRecData.interrupts &= ~interrupt;
	}
}

void NesCpuRecompiler::sl()
{
	if (emsl.save)
		saveStateToBase();
	NesCpuBase::sl();
	if (!emsl.save)
		loadStateFromBase();
}

void NesCpuRecompiler::saveStateToBase()
{
	m_stateRegs.pc = cpuRecData.regs[NesCpuRecData::PC];
	m_stateRegs.p = cpuRecData.regs[NesCpuRecData::P];
	m_stateRegs.a = cpuRecData.regs[NesCpuRecData::A] >> 24;
	m_stateRegs.x = cpuRecData.regs[NesCpuRecData::X] >> 24;
	m_stateRegs.y = cpuRecData.regs[NesCpuRecData::Y] >> 24;
	m_stateRegs.s = cpuRecData.regs[NesCpuRecData::S] >> 24;
	m_stateSignals = cpuRecData.interruptSignals;
	m_stateInterrupts = cpuRecData.interrupts;
}

void NesCpuRecompiler::loadStateFromBase()
{
	cpuRecData.regs[NesCpuRecData::PC] = m_stateRegs.pc;
	cpuRecData.regs[NesCpuRecData::P] = m_stateRegs.p;
	cpuRecData.regs[NesCpuRecData::A] = m_stateRegs.a << 24;
	cpuRecData.regs[NesCpuRecData::X] = m_stateRegs.x << 24;
	cpuRecData.regs[NesCpuRecData::Y] = m_stateRegs.y << 24;
	cpuRecData.regs[NesCpuRecData::S] = m_stateRegs.s << 24;
	cpuRecData.interruptSignals = m_stateSignals;
	cpuRecData.interrupts = m_stateInterrupts;
	Q_ASSERT(cpuRecData.additionalCycles == 0);
	cpuRecData.currentCycles = 0;
}

#define OP___(ci,op) case ci: op(); break
#define OP_R_(ci,op,addr) case ci: m##addr##R(); op(); break
#define OP__W(ci,op,addr) case ci: m##addr##W(); op(); mStore##addr(); break
#define OP_RW(ci,op,addr) case ci: m##addr##RW(); op(); mStore##addr(); break

void NesCpuTranslator::mSingleInstruction()
{
	u8 op = fetchPc8();
	mAddCycles(NesCpuBase::cyclesTable[op]);

	switch (op) {
	OP___(0x00, mBrk     );
	OP_R_(0x01, mOra, Idx);
	OP___(0x02, mKil     );
	OP_RW(0x03, mSlo, Idx);
	OP___(0x04, mDop     );
	OP_R_(0x05, mOra, Zpg);
	OP_RW(0x06, mAsl, Zpg);
	OP_RW(0x07, mSlo, Zpg);
	OP___(0x08, mPhp     );
	OP_R_(0x09, mOra, Imm);
	OP___(0x0a, mAslAcc  );
	OP_R_(0x0b, mAac, Imm);
	OP_R_(0x0c, mNop, Abs);
	OP_R_(0x0d, mOra, Abs);
	OP_RW(0x0e, mAsl, Abs);
	OP_RW(0x0f, mSlo, Abs);
	OP___(0x10, mBpl     );
	OP_R_(0x11, mOra, Idy);
	OP___(0x12, mKil     );
	OP_RW(0x13, mSlo, Idy);
	OP___(0x14, mDop     );
	OP_R_(0x15, mOra, Zpx);
	OP_RW(0x16, mAsl, Zpx);
	OP_RW(0x17, mSlo, Zpx);
	OP___(0x18, mClc     );
	OP_R_(0x19, mOra, Aby);
	OP___(0x1a, mNop     );
	OP_RW(0x1b, mSlo, Aby);
	OP_R_(0x1c, mNop, Abx);
	OP_R_(0x1d, mOra, Abx);
	OP_RW(0x1e, mAsl, Abx);
	OP_RW(0x1f, mSlo, Abx);
	OP___(0x20, mJsr     );
	OP_R_(0x21, mAnd, Idx);
	OP___(0x22, mKil     );
	OP_RW(0x23, mRla, Idx);
	OP_R_(0x24, mBit, Zpg);
	OP_R_(0x25, mAnd, Zpg);
	OP_RW(0x26, mRol, Zpg);
	OP_RW(0x27, mRla, Zpg);
	OP___(0x28, mPlp     );
	OP_R_(0x29, mAnd, Imm);
	OP___(0x2a, mRolAcc  );
	OP_R_(0x2b, mAac, Imm);
	OP_R_(0x2c, mBit, Abs);
	OP_R_(0x2d, mAnd, Abs);
	OP_RW(0x2e, mRol, Abs);
	OP_RW(0x2f, mRla, Abs);
	OP___(0x30, mBmi     );
	OP_R_(0x31, mAnd, Idy);
	OP___(0x32, mKil     );
	OP_RW(0x33, mRla, Idy);
	OP___(0x34, mDop     );
	OP_R_(0x35, mAnd, Zpx);
	OP_RW(0x36, mRol, Zpx);
	OP_RW(0x37, mRla, Zpx);
	OP___(0x38, mSec     );
	OP_R_(0x39, mAnd, Aby);
	OP___(0x3a, mNop     );
	OP_RW(0x3b, mRla, Aby);
	OP_R_(0x3c, mNop, Abx);
	OP_R_(0x3d, mAnd, Abx);
	OP_RW(0x3e, mRol, Abx);
	OP_RW(0x3f, mRla, Abx);
	OP___(0x40, mRti     );
	OP_R_(0x41, mEor, Idx);
	OP___(0x42, mKil     );
	OP_RW(0x43, mSre, Idx);
	OP___(0x44, mDop     );
	OP_R_(0x45, mEor, Zpg);
	OP_RW(0x46, mLsr, Zpg);
	OP_RW(0x47, mSre, Zpg);
	OP___(0x48, mPha     );
	OP_R_(0x49, mEor, Imm);
	OP___(0x4a, mLsrAcc  );
	OP_R_(0x4b, mAsr, Imm);
	OP___(0x4c, mJmpAbs  );
	OP_R_(0x4d, mEor, Abs);
	OP_RW(0x4e, mLsr, Abs);
	OP_RW(0x4f, mSre, Abs);
	OP___(0x50, mBvc     );
	OP_R_(0x51, mEor, Idy);
	OP___(0x52, mKil     );
	OP_RW(0x53, mSre, Idy);
	OP___(0x54, mDop     );
	OP_R_(0x55, mEor, Zpx);
	OP_RW(0x56, mLsr, Zpx);
	OP_RW(0x57, mSre, Zpx);
	OP___(0x58, mCli     );
	OP_R_(0x59, mEor, Aby);
	OP___(0x5a, mNop     );
	OP_RW(0x5b, mSre, Aby);
	OP_R_(0x5c, mNop, Abx);
	OP_R_(0x5d, mEor, Abx);
	OP_RW(0x5e, mLsr, Abx);
	OP_RW(0x5f, mSre, Abx);
	OP___(0x60, mRts     );
	OP_R_(0x61, mAdc, Idx);
	OP___(0x62, mKil     );
	OP_RW(0x63, mRra, Idx);
	OP___(0x64, mDop     );
	OP_R_(0x65, mAdc, Zpg);
	OP_RW(0x66, mRor, Zpg);
	OP_RW(0x67, mRra, Zpg);
	OP___(0x68, mPla     );
	OP_R_(0x69, mAdc, Imm);
	OP___(0x6a, mRorAcc  );
	OP_R_(0x6b, mArr, Imm);
	OP___(0x6c, mJmpInd );
	OP_R_(0x6d, mAdc, Abs);
	OP_RW(0x6e, mRor, Abs);
	OP_RW(0x6f, mRra, Abs);
	OP___(0x70, mBvs     );
	OP_R_(0x71, mAdc, Idy);
	OP___(0x72, mKil     );
	OP_RW(0x73, mRra, Idy);
	OP___(0x74, mDop     );
	OP_R_(0x75, mAdc, Zpx);
	OP_RW(0x76, mRor, Zpx);
	OP_RW(0x77, mRra, Zpx);
	OP___(0x78, mSei     );
	OP_R_(0x79, mAdc, Aby);
	OP___(0x7a, mNop     );
	OP_RW(0x7b, mRra, Aby);
	OP_R_(0x7c, mNop, Abx);
	OP_R_(0x7d, mAdc, Abx);
	OP_RW(0x7e, mRor, Abx);
	OP_RW(0x7f, mRra, Abx);
	OP___(0x80, mDop     );
	OP__W(0x81, mSta, Idx);
	OP___(0x82, mDop     );
	OP__W(0x83, mAax, Idx);
	OP__W(0x84, mSty, Zpg);
	OP__W(0x85, mSta, Zpg);
	OP__W(0x86, mStx, Zpg);
	OP__W(0x87, mAax, Zpg);
	OP___(0x88, mDey     );
	OP___(0x89, mDop     );
	OP___(0x8a, mTxa     );
	OP_R_(0x8b, mXaa, Imm);
	OP__W(0x8c, mSty, Abs);
	OP__W(0x8d, mSta, Abs);
	OP__W(0x8e, mStx, Abs);
	OP__W(0x8f, mAax, Abs);
	OP___(0x90, mBcc     );
	OP__W(0x91, mSta, Idy);
	OP___(0x92, mKil     );
	OP__W(0x93, mSha, Idy);
	OP__W(0x94, mSty, Zpx);
	OP__W(0x95, mSta, Zpx);
	OP__W(0x96, mStx, Zpy);
	OP__W(0x97, mAax, Zpy);
	OP___(0x98, mTya     );
	OP__W(0x99, mSta, Aby);
	OP___(0x9a, mTxs     );
	OP__W(0x9b, mXas, Aby);
	OP__W(0x9c, mSya, Abx);
	OP__W(0x9d, mSta, Abx);
	OP__W(0x9e, mSxa, Aby);
	OP__W(0x9f, mSha, Aby);
	OP___(0xa0, mLdyImm  );
	OP_R_(0xa1, mLda, Idx);
	OP___(0xa2, mLdxImm  );
	OP_R_(0xa3, mLax, Idx);
	OP_R_(0xa4, mLdy, Zpg);
	OP_R_(0xa5, mLda, Zpg);
	OP_R_(0xa6, mLdx, Zpg);
	OP_R_(0xa7, mLax, Zpg);
	OP___(0xa8, mTay     );
	OP___(0xa9, mLdaImm  );
	OP___(0xaa, mTax     );
	OP_R_(0xab, mAtx, Imm);
	OP_R_(0xac, mLdy, Abs);
	OP_R_(0xad, mLda, Abs);
	OP_R_(0xae, mLdx, Abs);
	OP_R_(0xaf, mLax, Abs);
	OP___(0xb0, mBcs     );
	OP_R_(0xb1, mLda, Idy);
	OP___(0xb2, mKil     );
	OP_R_(0xb3, mLax, Idy);
	OP_R_(0xb4, mLdy, Zpx);
	OP_R_(0xb5, mLda, Zpx);
	OP_R_(0xb6, mLdx, Zpy);
	OP_R_(0xb7, mLax, Zpy);
	OP___(0xb8, mClv     );
	OP_R_(0xb9, mLda, Aby);
	OP___(0xba, mTsx     );
	OP_R_(0xbb, mLar, Aby);
	OP_R_(0xbc, mLdy, Abx);
	OP_R_(0xbd, mLda, Abx);
	OP_R_(0xbe, mLdx, Aby);
	OP_R_(0xbf, mLax, Aby);
	OP_R_(0xc0, mCpy, Imm);
	OP_R_(0xc1, mCmp, Idx);
	OP___(0xc2, mDop     );
	OP_RW(0xc3, mDcp, Idx);
	OP_R_(0xc4, mCpy, Zpg);
	OP_R_(0xc5, mCmp, Zpg);
	OP_RW(0xc6, mDec, Zpg);
	OP_RW(0xc7, mDcp, Zpg);
	OP___(0xc8, mIny     );
	OP_R_(0xc9, mCmp, Imm);
	OP___(0xca, mDex     );
	OP_R_(0xcb, mAxs, Imm);
	OP_R_(0xcc, mCpy, Abs);
	OP_R_(0xcd, mCmp, Abs);
	OP_RW(0xce, mDec, Abs);
	OP_RW(0xcf, mDcp, Abs);
	OP___(0xd0, mBne     );
	OP_R_(0xd1, mCmp, Idy);
	OP___(0xd2, mKil     );
	OP_RW(0xd3, mDcp, Idy);
	OP___(0xd4, mDop     );
	OP_R_(0xd5, mCmp, Zpx);
	OP_RW(0xd6, mDec, Zpx);
	OP_RW(0xd7, mDcp, Zpx);
	OP___(0xd8, mCld     );
	OP_R_(0xd9, mCmp, Aby);
	OP___(0xda, mNop     );
	OP_RW(0xdb, mDcp, Aby);
	OP_R_(0xdc, mNop, Abx);
	OP_R_(0xdd, mCmp, Abx);
	OP_RW(0xde, mDec, Abx);
	OP_RW(0xdf, mDcp, Abx);
	OP_R_(0xe0, mCpx, Imm);
	OP_R_(0xe1, mSbc, Idx);
	OP___(0xe2, mDop     );
	OP_RW(0xe3, mIsb, Idx);
	OP_R_(0xe4, mCpx, Zpg);
	OP_R_(0xe5, mSbc, Zpg);
	OP_RW(0xe6, mInc, Zpg);
	OP_RW(0xe7, mIsb, Zpg);
	OP___(0xe8, mInx     );
	OP_R_(0xe9, mSbc, Imm);
	OP___(0xea, mNop     );
	OP_R_(0xeb, mSbc, Imm);
	OP_R_(0xec, mCpx, Abs);
	OP_R_(0xed, mSbc, Abs);
	OP_RW(0xee, mInc, Abs);
	OP_RW(0xef, mIsb, Abs);
	OP___(0xf0, mBeq     );
	OP_R_(0xf1, mSbc, Idy);
	OP___(0xf2, mKil     );
	OP_RW(0xf3, mIsb, Idy);
	OP___(0xf4, mDop     );
	OP_R_(0xf5, mSbc, Zpx);
	OP_RW(0xf6, mInc, Zpx);
	OP_RW(0xf7, mIsb, Zpx);
	OP___(0xf8, mSed     );
	OP_R_(0xf9, mSbc, Aby);
	OP___(0xfa, mNop     );
	OP_RW(0xfb, mIsb, Aby);
	OP_R_(0xfc, mNop, Abx);
	OP_R_(0xfd, mSbc, Abx);
	OP_RW(0xfe, mInc, Abx);
	OP_RW(0xff, mIsb, Abx);
	}
}
