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

#ifndef NESCPUREC_MATH_H
#define NESCPUREC_MATH_H

#include "cpurec_p.h"

/*!
	the N2A03 has no BCD mode

	A = (A + DT + P.C)[7:0]
	P.Z = (A == 0)
	P.N = A[7]
	P.C = carry of sum (unsigned)
	P.V = carry of sum (signed)
 */
inline void NesCpuTranslator::mAdc()
{
	__ sub(mDT, mDT, Operand(0x100), LeaveCC, cs);
	__ adc(mA, mA, Operand(mDT, ROR, 8), SetCC);
}

inline void NesCpuTranslator::mSbc()
{
	__ eor(mDT, mDT, Operand(0xff));
	mAdc();
}

/*!
	WT = reg - DT
	P.Z = (WT[7:0] == 0)
	P.N = WT[7]
	P.C = (WT[8] == 0)
 */
inline void NesCpuTranslator::mCmpTemplate(Register reg)
{
	Q_ASSERT(reg == mA || reg == mX || reg == mY);
	__ rsb(mWT, mDT, Operand(reg, LSR, 24)); // do not update flags here - overflow flag
	__ eor(mWT, mWT, Operand(1 << 8));
	__ mov(mWT, Operand(mWT, LSL, 24), SetCC); // update P.C, P.N and P.Z
}

inline void NesCpuTranslator::mCmp() { mCmpTemplate(mA); }
inline void NesCpuTranslator::mCpx() { mCmpTemplate(mX); }
inline void NesCpuTranslator::mCpy() { mCmpTemplate(mY); }

//--------------------------- inc/dec instructions -----------------------------

/*!
	cumulates multiple inc instructions into one

	DT  = (DT + n)[7:0]
	P.Z = (DT == 0)
	P.N = DT[7]
 */
inline void NesCpuTranslator::mIncMultiple(int n)
{
	__ mov(mDT, Operand(mDT, LSL, 24));
	__ add(mDT, mDT, Operand((n&0xff) << 24)); // do not touch P.C
	__ mov(mDT, mDT, SetCC);
	__ mov(mDT, Operand(mDT, LSR, 24));
}

/*!
	cumulates multiple inx/iny instructions into one

	reg = (reg + n)[7:0]
	P.Z = (reg == 0)
	P.N = reg[7]
 */
inline void NesCpuTranslator::mIncRegMultiple(Register reg, int n)
{
	__ add(reg, reg, Operand((n&0xff) << 24));
	__ mov(reg, reg, SetCC);
}

inline void NesCpuTranslator::mInxMultiple(int n) { mIncRegMultiple(mX, n); }
inline void NesCpuTranslator::mInyMultiple(int n) { mIncRegMultiple(mY, n); }

inline void NesCpuTranslator::mInc() { mIncMultiple(1); }
inline void NesCpuTranslator::mInx() { mInxMultiple(1); }
inline void NesCpuTranslator::mIny() { mInyMultiple(1); }

/*!
	cumulates multiple dec instructions into one

	DT  = (DT - n)[7:0]
	P.Z = (DT == 0)
	P.N = DT[7]
 */
inline void NesCpuTranslator::mDecMultiple(int n)
{
	__ mov(mDT, Operand(mDT, LSL, 24));
	__ sub(mDT, mDT, Operand((n&0xff) << 24)); // do not touch P.C
	__ mov(mDT, mDT, SetCC);
	__ mov(mDT, Operand(mDT, LSR, 24));
}

/*!
	cumulates multiple dex/dey instructions into one

	reg = (reg - n)[7:0]
	P.Z = reg == 0
	P.N = reg[7]
 */
inline void NesCpuTranslator::mDecRegMultiple(Register reg, int n)
{
	__ sub(reg, reg, Operand((n&0xff) << 24)); // do not touch P.C
	__ mov(reg, reg, SetCC);
}

inline void NesCpuTranslator::mDexMultiple(int n) { mDecRegMultiple(mX, n); }
inline void NesCpuTranslator::mDeyMultiple(int n) { mDecRegMultiple(mY, n); }

inline void NesCpuTranslator::mDec() { mDecMultiple(1); }
inline void NesCpuTranslator::mDex() { mDexMultiple(1); }
inline void NesCpuTranslator::mDey() { mDeyMultiple(1); }

#endif // NESCPUREC_MATH_H
