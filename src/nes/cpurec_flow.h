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

#ifndef NESCPUREC_FLOW_H
#define NESCPUREC_FLOW_H

#include "cpurec_p.h"

inline void NesCpuTranslator::mJump(Register to)
{
	Q_ASSERT(to != ip);

	if (to != r0)
		__ mov(r0, to);
	mCheckSync();
	// load label pos
	mLoadLabelAddress(r0, r1);
	// branch and save return for translate
	__ blx(r1);
}

inline void NesCpuTranslator::mJump(u16 to)
{
	__ mov(r0, Operand(to));
	// fast path: when address of the label is in the same section
	u16 from = currentPc();
	if (!((from>>13) ^ (to>>13))) {
		Label *label = &m_labels[to];
		mCheckSync();
		// branch and save return for translate
		__ bl(label);
		return;
	}
	// slow path:
	mJump(r0);
}

/*!
	PC = FETCH_PC16(PC)
 */
inline void NesCpuTranslator::mJmpAbs()
{
	u16 addr = fetchPc16();
	mJump(addr);
}

/*!
	u16 WT = READ16(PC)
	EA = READ8(WT)
	WT = ((WT+1)&0x00ff) | (WT&0xff00)
	PC = EA | (READ8(WT)<<8)
 */
inline void NesCpuTranslator::mJmpInd()
{
	u16 WT = fetchPc16();
	mRead8(WT, mEA, 0);
	WT = ((WT+1)&0x00ff) | (WT&0xff00);
	mRead8(WT, mDT, mEA.bit());
	__ orr(r0, mEA, Operand(mDT, LSL, 8));
	mJump(r0);
}

/*!
	EA = READ16(PC)
	PUSH16(PC+1)
	PC = EA
 */
inline void NesCpuTranslator::mJsr()
{
	__ mov(r0, Operand(u16(currentPc()+1)));
	mPush16(r0);
	mJmpAbs();
}

/*!
	P = POP8()
	PC = POP16()
 */
inline void NesCpuTranslator::mRti()
{
	mPlp();
	mPop16(r0);
	mJump(r0);
}

/*!
	PC = POP16() + 1
 */
inline void NesCpuTranslator::mRts()
{
	mPop16(r0);
	__ add(r0, r0, Operand(1));
	// there is no need to and with 0xffff because it will be definitely < 0xffff
	mJump(r0);
}

inline void NesCpuTranslator::mReadInterruprVector(u16 vector,
												   Register dst,
												   Register tmp)
{
	Q_ASSERT(dst != ip && tmp != ip && dst != tmp);
	mReadDirect8(vector  , dst);
	mReadDirect8(vector+1, tmp);
	__ orr(dst, dst, Operand(tmp, LSL, 8));
}

/*!
	PUSH16(PC+1);
	PUSH8(P|U|B);
	P |= I;
	PC = READ16(IrqVectorAddress);
 */
inline void NesCpuTranslator::mBrk()
{
	__ mov(r0, Operand(u16(currentPc()+1)));
	mPush16(r0);
	mPackFlags(mDT);
	__ orr(mDT, mDT, Operand(NesCpuBase::Unused|NesCpuBase::Break));
	mPush8(mDT);
	mSei();
	mReadInterruprVector(NesCpuBase::IrqVectorAddress, r0, r1);
	mJump(r0);
}

inline bool NesCpuTranslator::mCheckBoundaryConst(u16 ET, u16 EA, bool addCyclesHere)
{
	// (ET^EA) & 0x100
	bool boundary = ((ET^EA) >> 8) & 1;
	if (boundary) {
		mRead8(EA-0x100, no_reg, 0);
		if (addCyclesHere)
			mAddCycles(1);
	}
	return boundary;
}

/*!
	ET = PC
	EA = PC + (s8)DT
	PC = EA
	ADDCYC(1)
	CHECK_EA()
 */
inline void NesCpuTranslator::mBxx(Condition cond)
{
	Q_ASSERT(cond != al);
	Label omit;
	__ b(&omit, NegateCondition(cond));

	s8 offset = fetchPc8();
	u16 ET = currentPc();
	u16 EA = ET + offset;
	int addcyc = 1;
	if (mCheckBoundaryConst(ET, EA, false))
		addcyc++;
	mAddCycles(addcyc);
	mJump(EA);

	__ bind(&omit);
}

inline void NesCpuTranslator::mBcc() { mBxx(cc); }
inline void NesCpuTranslator::mBcs() { mBxx(cs); }
inline void NesCpuTranslator::mBne() { mBxx(ne); }
inline void NesCpuTranslator::mBeq() { mBxx(eq); }
inline void NesCpuTranslator::mBpl() { mBxx(pl); }
inline void NesCpuTranslator::mBmi() { mBxx(mi); }
inline void NesCpuTranslator::mBvc() { mBxx(vc); }
inline void NesCpuTranslator::mBvs() { mBxx(vs); }

#endif // NESCPUREC_FLOW_H
