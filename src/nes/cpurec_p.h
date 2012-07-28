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

#ifndef NESCPUREC_P_H
#define NESCPUREC_P_H

#include "cpurec.h"
#include "mapper.h"
#include <arm/macroassembler.h>

class NesMapper;

using namespace Arm;

static const Register mDT(r0);
static const Register mWT(r1);
static const Register mET(r2);
static const Register mEA(r3);
static const Register mA(r4);
static const Register mX(r5);
static const Register mY(r6);
static const Register mS(r7);
static const Register mFlags(r8);
static const Register mCycles(r9);
static const Register mDataBase(r10);
#if !defined(FRAME_POINTER_FOR_GDB)
static const Register mInternalFlagsCopy(fp);
#endif

#define __ m_masm->

class NesCpuRecData
{
public:
	enum Alert {
		AlertOn		= 0x00, // this values are needed to do pc trick when
		AlertOff	= 0x02  // checking for an alert
	};

	enum Reg { PC, P, A, X, Y, S };
	// temporary regs needed for interchange with registers from the base class
	// NesCpuBase on saving/loading state
	u32 regs[6];

	u8 *ram;
	u8 *stack;
	u8 **banks;

	u32 interruptSignals;
	u32 interrupts;
	// cycles returned from nesSync
	s32 startCycles;
	// amount of additional cycles after intrerrupt, used also to stole
	// cycles from cpu when dma occurs
	s32 additionalCycles;
	// a mirror of mCycles register (updated only on demand, when it's really
	// needed, e.g. when writing to APU-40xx address)
	s32 currentCycles;
	// indicates if write function triggered some interrupt or dma access
	u32 alert;

	void *(*translate)(u16 instrPointer, u8 *caller);

	NesSync *nesSync;
	void (*cpuWrite)(u16 addr, u8 data);
	  u8 (*cpuRead)(u16 addr);
	void (*cpuWrite40xx)(u16 addr, u8 data);
	  u8 (*cpuRead40xx)(u16 addr);
	void (*cpuWriteLow)(u16 addr, u8 data);
	  u8 (*cpuReadLow)(u16 addr);
	void (*cpuWriteHigh)(u16 addr, u8 data);

	void (*ppuWriteReg)(u16 addr, u8 data);
	  u8 (*ppuReadReg)(u16 addr);

	void (*apuClock)(int cycles);
	void (*mapperClock)(int cycles);

	void (*processCheats)();

#if defined(ENABLE_DEBUGGING)
	void (*debugStep)(u16 pc);
#endif

	union {
		void (*entryPoint)();
		intptr_t entryPointPtr;
	};

#if defined(FRAME_POINTER_FOR_GDB)
	u32 internalFlagsCopy;
#endif
};

class NesCpuTranslator
{
public:
	static const int BufferSize = 64 * KB * 32 * Assembler::kInstrSize;
	static const int BlockSize = BufferSize / NesNumOfCpuPages;

	bool init();
	void shutdown();

	u16 currentPc() const { return m_recPc; }
	u8 fetchPc8();
	u16 fetchPc16();

	void saveAbsAddr(u16 addr) { m_absAddr = addr; }
	u16 loadAbsAddr() const { return m_absAddr; }

	void saveZpgAddr(u8 addr) { m_zpgAddr = addr; }
	u8 loadZpgAddr() const { return m_zpgAddr; }

	void *process(u16 instrPointer, u8 *caller);
	void fixCallerInstruction(u16 instrPointer, u8 *caller);

	void clearPage(int pageIndex);

	void saveTranslationBoundary(int currentPageIndex,
								 int numBytesUsedFromNext);
	void checkTranslationBoundary(int pageIndex);

	void mTo16Bit(Register reg);
	void mAddCycles(int n, Condition cond = al);
	void mStoreCurrentCycles();
	void mFetchAdditionalCycles();
	void mTranslateCaller();
	void mCallCFunction(int funcOffset);
	void mLoadLabelAddress(Register offset, Register dst);

	void mSaveInternalFlags();
	void mRestoreInternalFlags();

	void mCheckSync();
	void mHandleEvent();
	void mHwInterrupt();
	void mHandleInterrupts();
	void mSync();

	void mCheckAlert();
	void mAlertHandler();
	void mClearAlert();

	void mEntryPoint();
	void mExitPoint();

#if defined(ENABLE_DEBUGGING)
	void mDebugStep();
#endif

	bool mTryOptimize();
	bool mOptimAslAcc();
	bool mOptimLsrAcc();
	bool mOptimBitAbs();
	bool mOptimBitAbsBxx(Condition cond);
	bool mOptimJmpAbs();
	bool mOptimLdaAbs();
	bool mOptimLdaAbsBpl();
	bool mOptimLdaAbsAndImm();
	bool mOptimLdaAbsAndImmBneBeq(Condition cond);
	bool mOptimIncReg(u8 op, Register reg);
	bool mOptimDecReg(u8 op, Register reg);
	bool mOptimDecRegBne(Register reg);
	bool mOptimCmpZpg();
	bool mOptimCmpZpgBxx(Condition cond);
	bool mOptimLdaZpg();
	bool mOptimLdaZpgBxx(Condition cond);
	bool mOptimNop(u8 op);
	bool mOptimNopJmpAbs(int nopCycles, int nopCount);
	void mSaturateCycles(int modValue);
	Condition bxxToCondition(u8 op) const;

	void mSingleInstruction();
	void mNop() {}
	// logical
	void mAnd();
	void mOra();
	void mEor();
	void mBit();
	// math
	void mAdc();
	void mSbc();
	void mCmpTemplate(Register reg);
	void mCmp();
	void mCpx();
	void mCpy();
	// inc/dec
	void mIncMultiple(int n);
	void mIncRegMultiple(Register reg, int n);
	void mInxMultiple(int n);
	void mInyMultiple(int n);
	void mInc();
	void mInx();
	void mIny();
	void mDecMultiple(int n);
	void mDecRegMultiple(Register reg, int n);
	void mDexMultiple(int n);
	void mDeyMultiple(int n);
	void mDec();
	void mDex();
	void mDey();
	// shift
	void mAsl();
	void mAslAcc();
	void mLsr();
	void mLsrAcc();
	void mRol();
	void mRolAcc();
	void mRor();
	void mRorAcc();
	// transfer
	void mLdTemplate(Register reg);
	void mLda();
	void mLdx();
	void mLdy();
	void mLdImmTemplate(Register reg);
	void mLdaImm();
	void mLdxImm();
	void mLdyImm();
	void mStTemplate(Register reg);
	void mSta();
	void mStx();
	void mSty();
	void mTransferTemplate(Register dst, Register src);
	void mTax();
	void mTxa();
	void mTay();
	void mTya();
	void mTsx();
	void mTxs();
	// flags
	void mSetInternalFlag(int flag, bool on);
	void mSetExternalFlag(int flag, bool on);
	void mClc();
	void mSec();
	void mClv();
	void mCld();
	void mSed();
	void mCli();
	void mSei();
	void mPackFlags(Register reg);
	void mUnpackFlags(Register reg, Register tmp);
	// stack access
	void mPush8(Register val);
	void mPop8(Register val);
	void mPush16(Register val);
	void mPop16(Register val);
	// stack instructions
	void mPha();
	void mPla();
	void mPhp();
	void mPlp();
	// flow
	void mJump(Register to);
	void mJump(u16 to);
	void mJmpAbs();
	void mJmpInd();
	void mJsr();
	void mRti();
	void mRts();
	void mReadInterruprVector(u16 vector, Register dst, Register tmp);
	void mBrk();
	bool mCheckBoundaryConst(u16 ET, u16 EA, bool addCyclesHere = true);
	void mBxx(Condition cond);
	void mBcc();
	void mBcs();
	void mBne();
	void mBeq();
	void mBpl();
	void mBmi();
	void mBvc();
	void mBvs();
	// memory
	void mRead8(Register addr, Register dst, RegList preserve);
	void mWrite8(Register addr, Register src, RegList preserve);
	void mRead8CallCFunc(u16 addr,
						 Register dst,
						 RegList preserve,
						 int functionOffset);
	void mWrite8CallCFunc(u16 addr,
						  Register src,
						  RegList preserve,
						  int functionOffset,
						  bool cheatsProcessing = false);
	void mReadRam8(u16 addr, Register dst);
	void mReadRam8(Register addr, Register dst);
	void mWriteRam8(u16 addr, Register src);
	void mWriteRam8(Register addr, Register src);
	void mReadZp8(u8 addr, Register dst);
	void mReadZp8(Register addr, Register dst);
	void mWriteZp8(u8 addr, Register src);
	void mWriteZp8(Register addr, Register src);
	void mReadZp16(u8 addr, Register dst);
	void mReadZp16(Register addr, Register dst, Register scratch);
	void mReadDirect8(u16 addr, Register dst);
	void mReadDirect8(Register addr, Register dst);
	void mRead8(u16 addr, Register dst, RegList preserve);
	void mWrite8(u16 addr, Register src, RegList preserve);
	// addressing
	void mFinishRW();
	void mCrossRead();
	void mCheckBoundary();
	void mImmR();
	void mZpgW();
	void mZpgR();
	void mZpgRW();
	void mZpiW(Register reg);
	void mZpiR(Register reg);
	void mZpxW();
	void mZpyW();
	void mZpxR();
	void mZpyR();
	void mZpxRW();
	void mZpyRW();
	void mAbsW();
	void mAbsR();
	void mAbsRW();
	void mAbiW(Register reg);
	void mAbiR(Register reg);
	void mAbiRW(Register reg);
	void mAbxW();
	void mAbyW();
	void mAbxR();
	void mAbyR();
	void mAbxRW();
	void mAbyRW();
	void mIdxW();
	void mIdxR();
	void mIdxRW();
	void mIdyW();
	void mIdyR();
	void mIdyRW();
	void mStoreZpg();
	void mStoreZpx();
	void mStoreZpy();
	void mStoreMem();
	void mStoreAbs();
	void mStoreAbx();
	void mStoreAby();
	void mStoreIdx();
	void mStoreIdy();
	// undocummented instructions
	void mAac();
	void mXaa();
	void mArr();
	void mAsr();
	void mDcp();
	void mIsb();
	void mLax();
	void mRla();
	void mRra();
	void mSlo();
	void mSre();
	void mLar();
	void mXas();
	void mAxs();
	void mAax();
	void mAtx();
	void mSiaTemplate(Register reg);
	void mSxa();
	void mSya();
	void mSha();
	void mDop();
	void mTop();
	void mKil();
private:
	MacroAssembler *m_masm;
	u16 m_recPc;
	u16 m_pad1;
	u8 *m_codeBuffer;
	int m_pageTranslationOffset[NesNumOfCpuPages];
	int m_pageUsedMask;
	bool m_checkAlertAfterInstruction;
	// labels of each instruction in 6502 address space
	Label *m_labels;

	Label m_syncLabel;
	Label m_syncWithoutInterruptHandling;
	Label m_checkInterruptsForNextInstruction;
	Label m_alertHandlerLabel;
	Label m_translateCallerLabel;
#if defined(ENABLE_DEBUGGING)
	Label m_debugStepLabel;
#endif

	u16 m_absAddr;
	u8 m_zpgAddr;
	u8 pad2;

	int m_numOfDataUsedFromNextPage[8];
	u8 m_dataUsedFromNextPage[8][16];

	u16 m_lastRecompilationInRam;
	u16 pad3;
};

#endif // NESCPUREC_P_H
