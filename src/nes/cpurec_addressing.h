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

#ifndef NESCPUREC_ADDRESSING_H
#define NESCPUREC_ADDRESSING_H

#include "cpurec_p.h"

inline void NesCpuTranslator::mFinishRW()
{
	mRead8(mEA, mDT, mEA.bit());
	mWrite8(mEA, mDT, mEA.bit()|mDT.bit());
}

/*!
	READ8((ET&0xff00) | (EA&0x00ff))
 */
inline void NesCpuTranslator::mCrossRead()
{
	__ and_(mWT, mET, Operand(0xff00));
	__ and_(ip, mEA, Operand(0x00ff));
	__ orr(mWT, mWT, ip);
	mRead8(mWT, no_reg, mEA.bit());
}

/*!
	if ((ET^EA) & 0x100) {
		READ8(EA-0x100)
		CYCLES = CYCLES + 1
	}
 */
inline void NesCpuTranslator::mCheckBoundary()
{
	__ mrs(mWT, CPSR);
	__ eor(mDT, mEA, Operand(mET));
	__ tst(mDT, Operand(0x100));
	Label noBoundary;
	__ b(&noBoundary, eq);
	__ sub(mDT, mDT, Operand(0x100));
	mRead8(mDT, no_reg, mEA.bit()|mWT.bit());
	mAddCycles(1);
	__ bind(&noBoundary);
	__ msr(CPSR_f, Operand(mWT));
}

//--------------------------- immediate addressing -----------------------------

/*!
	DT = [PC++][7:0]
 */
inline void NesCpuTranslator::mImmR()
{
	u8 data = fetchPc8();
	__ mov(mDT, Operand(data));
}

//--------------------------- zero page addressing -----------------------------

/*!
	EA = [PC++][7:0]
 */
inline void NesCpuTranslator::mZpgW()
{
	u8 addr = fetchPc8();
	saveZpgAddr(addr);
	__ mov(mEA, Operand(addr));
}

/*!
	EA = [PC++][7:0]
	DT = [EA]
 */
inline void NesCpuTranslator::mZpgR()
{
	u8 addr = fetchPc8();
	saveZpgAddr(addr);
	mReadZp8(addr, mDT);
	__ mov(mEA, Operand(addr));
}

inline void NesCpuTranslator::mZpgRW()
{
	mZpgR();
	// write not needed here
}

//----------------------- zero page indexed addressing -------------------------

/*!
	EA = ([PC++][7:0] + reg)[7:0]
 */
inline void NesCpuTranslator::mZpiW(Register reg)
{
	Q_ASSERT(reg == mX || reg == mY);
	u8 addr = fetchPc8();
	__ add(mEA, reg, Operand(addr << 24)); // will be cut down to 8 bit
	__ mov(mEA, Operand(mEA, LSR, 24));
}

/*!
	EA = ([PC++][7:0] + reg)[7:0]
	DT = [EA]
 */
inline void NesCpuTranslator::mZpiR(Register reg)
{
	Q_ASSERT(reg == mX || reg == mY);
	mZpiW(reg);
	mReadZp8(mEA, mDT);
}

inline void NesCpuTranslator::mZpxW() { mZpiW(mX); }
inline void NesCpuTranslator::mZpyW() { mZpiW(mY); }

inline void NesCpuTranslator::mZpxR() { mZpiR(mX); }
inline void NesCpuTranslator::mZpyR() { mZpiR(mY); }

inline void NesCpuTranslator::mZpxRW() { mZpiR(mX); }
inline void NesCpuTranslator::mZpyRW() { mZpiR(mY); }

//--------------------------- absolute addressing ------------------------------

/*!
	EA = [PC][15:0]
	PC = PC + 2
 */
inline void NesCpuTranslator::mAbsW()
{
	u16 addr = fetchPc16();
	saveAbsAddr(addr);
	__ mov(mEA, Operand(addr));
}

/*!
	EA = [PC][15:0]
	PC = PC + 2
	DT = [EA]
 */
inline void NesCpuTranslator::mAbsR()
{
	u16 addr = fetchPc16();
	mRead8(addr, mDT, 0);
	__ mov(mEA, Operand(addr));
}

inline void NesCpuTranslator::mAbsRW()
{
	u16 addr = fetchPc16();
	saveAbsAddr(addr);
	// instead of mFinishRW() generate quicker alternative based on constant address
	mRead8(addr, mDT, 0);
	if (addr >= 0x2000)
		mWrite8(addr, mDT, mDT.bit());
	__ mov(mEA, Operand(addr));
}

//----------------------- absolute indexed addressing --------------------------

/*!
	ET = [PC][15:0]
	EA = (ET + reg)[15:0]
	cross page read: READ8((EA&0x00ff)|(ET&0xff00))
 */
inline void NesCpuTranslator::mAbiW(Register reg)
{
	Q_ASSERT(reg == mX || reg == mY);
	u16 addr = fetchPc16();
	__ mov(mET, Operand(addr));
	__ add(mEA, mET, Operand(reg, LSR, 24));
	mTo16Bit(mEA);
	mCrossRead();
}

/*!
	ET = [PC][15:0]
	EA = (ET + reg)[15:0]
	CHECK_BOUNDARY
	DT = [EA]
 */
inline void NesCpuTranslator::mAbiR(Register reg)
{
	Q_ASSERT(reg == mX || reg == mY);
	u16 addr = fetchPc16();
	__ mov(mET, Operand(addr));
	__ add(mEA, mET, Operand(reg, LSR, 24));
	mTo16Bit(mEA);
	mCheckBoundary();
	mRead8(mEA, mDT, mEA.bit());
}

inline void NesCpuTranslator::mAbiRW(Register reg)
{
	mAbiW(reg);
	mFinishRW();
}

inline void NesCpuTranslator::mAbxW() { mAbiW(mX); }
inline void NesCpuTranslator::mAbyW() { mAbiW(mY); }

inline void NesCpuTranslator::mAbxR() { mAbiR(mX); }
inline void NesCpuTranslator::mAbyR() { mAbiR(mY); }

inline void NesCpuTranslator::mAbxRW() { mAbiRW(mX); }
inline void NesCpuTranslator::mAbyRW() { mAbiRW(mY); }

//----------------------- indexed indirect addressing --------------------------

/*!
	ET = ([PC++][7:0] + X)[7:0]
	EA = READ_ZP16(ET)
 */
inline void NesCpuTranslator::mIdxW()
{
	u8 addr = fetchPc8();
	__ add(mET, mX, Operand(addr << 24));
	__ mov(mET, Operand(mET, LSR, 24));
	mReadZp16(mET, mEA, mWT);
}

inline void NesCpuTranslator::mIdxR()
{
	mIdxW();
	mRead8(mEA, mDT, mEA.bit());
}

inline void NesCpuTranslator::mIdxRW()
{
	mIdxW();
	mFinishRW();
}

/*!
	DT = [PC++][7:0]
	ET = READ_ZP16(DT)
	EA = (ET + Y)[15:0]
	cross page read: READ8((EA&0x00ff) | (ET&0xff00))
 */
inline void NesCpuTranslator::mIdyW()
{
	u8 addr = fetchPc8();
	mReadZp16(addr, mET);
	__ add(mEA, mET, Operand(mY, LSR, 24));
	mTo16Bit(mEA);
	mCrossRead();
}

/*!
	DT = [PC++][7:0]
	ET = READ_ZP16(DT)
	EA = (ET + Y)[15:0]
	CHECK_BOUNDARY
	DT = READ8(EA)
 */
inline void NesCpuTranslator::mIdyR()
{
	u8 addr = fetchPc8();
	mReadZp16(addr, mET);
	__ add(mEA, mET, Operand(mY, LSR, 24));
	mTo16Bit(mEA);
	mCheckBoundary();
	mRead8(mEA, mDT, mEA.bit());
}

inline void NesCpuTranslator::mIdyRW()
{
	mIdyW();
	mFinishRW();
}

//----------------------- store for W and RW -----------------------------------

inline void NesCpuTranslator::mStoreZpg() { mWriteZp8(loadZpgAddr(), mDT); }
inline void NesCpuTranslator::mStoreZpx() { mWriteZp8(mEA, mDT); }
inline void NesCpuTranslator::mStoreZpy() { mWriteZp8(mEA, mDT); }

inline void NesCpuTranslator::mStoreMem() { mWrite8(mEA, mDT, 0); }
inline void NesCpuTranslator::mStoreAbs() { mWrite8(loadAbsAddr(), mDT, 0); }
inline void NesCpuTranslator::mStoreAbx() { mStoreMem(); }
inline void NesCpuTranslator::mStoreAby() { mStoreMem(); }
inline void NesCpuTranslator::mStoreIdx() { mStoreMem(); }
inline void NesCpuTranslator::mStoreIdy() { mStoreMem(); }

#endif // NESCPUREC_ADDRESSING_H
