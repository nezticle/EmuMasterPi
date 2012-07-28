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

#ifndef NESCPUREC_TRANSFER_H
#define NESCPUREC_TRANSFER_H

#include "cpurec_p.h"

//----------------------------- load instructions ------------------------------

/*!
	reg = DT
	P.Z = (reg == 0)
	P.N = reg[7]
 */
inline void NesCpuTranslator::mLdTemplate(Register reg)
{
	Q_ASSERT(reg == mA || reg == mX || reg == mY);
	__ mov(reg, Operand(mDT, LSL, 24));
	__ mov(reg, reg, SetCC);
}

inline void NesCpuTranslator::mLda() { mLdTemplate(mA); }
inline void NesCpuTranslator::mLdx() { mLdTemplate(mX); }
inline void NesCpuTranslator::mLdy() { mLdTemplate(mY); }

inline void NesCpuTranslator::mLdImmTemplate(Register reg)
{
	Q_ASSERT(reg == mA || reg == mX || reg == mY);
	u8 imm = fetchPc8();
	__ mov(reg, Operand(imm << 24));
	__ mov(reg, reg, SetCC);
}

inline void NesCpuTranslator::mLdaImm() { mLdImmTemplate(mA); }
inline void NesCpuTranslator::mLdxImm() { mLdImmTemplate(mX); }
inline void NesCpuTranslator::mLdyImm() { mLdImmTemplate(mY); }

//---------------------------- store instructions ------------------------------

/*!
	// DT = reg
 */
inline void NesCpuTranslator::mStTemplate(Register reg)
{
	Q_ASSERT(reg == mA || reg == mX || reg == mY);
	__ mov(mDT, Operand(reg, LSR, 24));
}

inline void NesCpuTranslator::mSta() { mStTemplate(mA); }
inline void NesCpuTranslator::mStx() { mStTemplate(mX); }
inline void NesCpuTranslator::mSty() { mStTemplate(mY); }

//----------------------------- move instructions ------------------------------

/*!
	dst = src
	P.Z = (dst == 0)
	P.N = dst[7]
 */
inline void NesCpuTranslator::mTransferTemplate(Register dst, Register src)
{
	Q_ASSERT(src == mA || src == mX || src == mY);
	Q_ASSERT(dst == mA || dst == mX || dst == mY);
	Q_ASSERT(src != dst && (dst == mA || src == mA));
	__ mov(dst, src, SetCC);
}

inline void NesCpuTranslator::mTax() { mTransferTemplate(mX, mA); }
inline void NesCpuTranslator::mTxa() { mTransferTemplate(mA, mX); }
inline void NesCpuTranslator::mTay() { mTransferTemplate(mY, mA); }
inline void NesCpuTranslator::mTya() { mTransferTemplate(mA, mY); }

/*!
	X = S
	P.Z = (X == 0)
	P.N = X[7]
 */
inline void NesCpuTranslator::mTsx()
{
	__ mov(mX, mS, SetCC);
}

/*!
	S = X
 */
inline void NesCpuTranslator::mTxs()
{
	__ mov(mS, mX);
}

#endif // NESCPUREC_TRANSFER_H
