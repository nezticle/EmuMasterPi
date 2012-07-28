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

#ifndef NESCPUREC_UNDOCUMMENTED_H
#define NESCPUREC_UNDOCUMMENTED_H

#include "cpurec_p.h"

/*!
	AND
	P.C = A[7]
 */
inline void NesCpuTranslator::mAac()
{
	__ and_(mA, mA, Operand(mDT, LSL, 24));
	__ mov(mWT, Operand(mA, LSL, 1), SetCC); // update P.C
	__ mov(mA, mA, SetCC); // update P.N and P.Z
}

/*!
	A = (A|0xee) & X & DT
	P.Z = (A == 0)
	P.N = A[7]
 */
inline void NesCpuTranslator::mXaa()
{
	__ orr(mA, mA, Operand(0xee << 24));
	__ and_(mA, mA, Operand(mDT, LSL, 24));
	__ and_(mA, mA, Operand(mX), SetCC);
}

/*!
	AND
	A = P.C:A[7:1]
	P.C = (A >> 6) & 1
	P.V = ((A >> 6) ^ (A >> 5)) & 1
	P.Z = (A == 0)
	P.N = A[7]
 */
inline void NesCpuTranslator::mArr()
{
	__ mov(mA, Operand(mA, LSR, 25));
	__ and_(mA, mA, Operand(mDT, LSR, 1));
	__ orr(mA, mA, Operand(0x80), LeaveCC, cs);
	__ mov(mWT, Operand(mA, LSR, 6+1), SetCC); // update P.C
	__ mrs(mWT, CPSR);
	__ eor(mDT, mA, Operand(mA, LSR, 1));
	__ tst(mDT, Operand(0x20));
	__ orr(mWT, mWT, Operand(kVConditionFlagBit), LeaveCC, ne); // update P.V
	__ bic(mWT, mWT, Operand(kVConditionFlagBit), LeaveCC, eq);
	__ msr(CPSR_f, Operand(mWT));
	__ mov(mA, Operand(mA, LSL, 24));
	__ mov(mA, mA, SetCC); // update P.N and P.Z
}

inline void NesCpuTranslator::mAsr() { mAnd(); mLsrAcc(); }
inline void NesCpuTranslator::mDcp() { mDec(); mCmp(); }
inline void NesCpuTranslator::mIsb() { mInc(); mSbc(); }
inline void NesCpuTranslator::mLax() { mLda(); mLdx(); }
inline void NesCpuTranslator::mRla() { mRol(); mAnd(); }
inline void NesCpuTranslator::mRra() { mRor(); mAdc(); }
inline void NesCpuTranslator::mSlo() { mAsl(); mOra(); }
inline void NesCpuTranslator::mSre() { mLsr(); mEor(); }

/*!
	S = S & DT
	X = S
	A = S
	P.Z = (A == 0)
	P.N = A[7]
 */
inline void NesCpuTranslator::mLar()
{
	__ and_(mS, mS, Operand(mDT, LSL, 24));
	__ mov(mX, mS);
	__ mov(mA, mS, SetCC);
}

/*!
	S = A & X
	DT = S & ((EA>>8)+1)
 */
inline void NesCpuTranslator::mXas()
{
	__ and_(mS, mA, Operand(mX));
	__ mov(mDT, Operand(mEA, LSR, 8));
	__ add(mDT, mDT, Operand(1));
	__ and_(mDT, mDT, Operand(mS, LSR, 24));
}

/*!
	WT = (A&X) - DT
	P.C = (WT[8] == 0)
	X = WT[7:0]
	P.Z = (X == 0)
	P.N = X[7]
 */
inline void NesCpuTranslator::mAxs()
{
	__ and_(mWT, mA, Operand(mX));
	__ rsb(mWT, mDT, Operand(mWT, LSR, 24));
	__ eor(mWT, mWT, Operand(1 << 8));
	__ mov(mX, Operand(mWT, LSL, 24), SetCC);
}

/*!
	DT = A & X;
 */
inline void NesCpuTranslator::mAax()
{
	__ and_(mDT, mA, Operand(mX));
	__ mov(mDT, Operand(mDT, LSR, 24));
}

/*!
	A = A | 0xee
	AND
	X = A
 */
inline void NesCpuTranslator::mAtx()
{
	__ orr(mA, mA, Operand(0xee << 24));
	mAnd();
	__ mov(mX, mA);
}

/*!
	DT = reg & ((EA>>8)+1)
 */
inline void NesCpuTranslator::mSiaTemplate(Register reg)
{
	Q_ASSERT(reg != mDT);
	__ mov(mDT, Operand(mEA, LSR, 8));
	__ add(mDT, mDT, Operand(1));
	__ and_(mDT, mDT, Operand(reg, LSR, 24));
}

inline void NesCpuTranslator::mSxa() { mSiaTemplate(mX); }
inline void NesCpuTranslator::mSya() { mSiaTemplate(mY); }

inline void NesCpuTranslator::mSha()
{
	__ and_(mWT, mA, Operand(mX));
	mSiaTemplate(mWT);
}

inline void NesCpuTranslator::mDop() { fetchPc8(); }
inline void NesCpuTranslator::mTop() { fetchPc16(); }

inline void NesCpuTranslator::mKil()
{
	// once KIL is executed the emulation system is jammed and can be only restarted
	Label loop;
	__ bind(&loop);
	mSei();
	__ mov(mCycles, Operand(0xff));
	// clear interrupts
	__ mov(ip, Operand(0));
	__ str(ip, MemOperand(mDataBase, offsetof(NesCpuRecData,interrupts)));
	__ mov(r0, Operand(m_recPc-1));
	mCheckSync();
	__ b(&loop);
	// halt recompilation
	m_recPc = 0;
}

#endif // NESCPUREC_UNDOCUMMENTED_H
