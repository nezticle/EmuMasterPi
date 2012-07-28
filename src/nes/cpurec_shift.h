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

#ifndef NESCPUREC_SHIFT_H
#define NESCPUREC_SHIFT_H

#include "cpurec_p.h"

//---------------------------- shift instructions ------------------------------

/*!
	P.C = DT[7]
	DT  = DT[6:0]:0
	P.Z = (DT[7:0] == 0)
	P.N = DT[7]
 */
inline void NesCpuTranslator::mAsl()
{
	__ mov(mDT, Operand(mDT, LSL, 25), SetCC);
	__ mov(mDT, Operand(mDT, LSR, 24));
}

/*!
	P.C = A[7]
	A   = A[6:0]:0
	P.Z = (A == 0)
	P.N = A[7]
 */
inline void NesCpuTranslator::mAslAcc()
{
	__ mov(mA, Operand(mA, LSL, 1), SetCC);
}

/*!
	P.C = DT[0]
	DT  = 0:DT[7:1]
	P.Z = (DT[7:0] == 0)
	P.N = DT[7] // will be cleared
 */
inline void NesCpuTranslator::mLsr()
{
	__ mov(mDT, Operand(mDT, LSR, 1), SetCC); // update P.C and P.Z, clear P.N
}

/*!
	P.C = A[0]
	A   = 0:A[7:1]
	P.Z = (A == 0)
	P.N = A[7] // will be cleared
 */
inline void NesCpuTranslator::mLsrAcc()
{
	__ mov(mA, Operand(mA, LSR, 25), SetCC); // update P.C and P.Z, clear P.N
	__ mov(mA, Operand(mA, LSL, 24));
}

/*!
	WT  = DT[7:0]:P.C
	P.C = WT[8]
	DT  = WT[7:0]
	P.Z = (DT == 0)
	P.N = DT[7]
 */
inline void NesCpuTranslator::mRol()
{
	__ adc(mDT, mDT, Operand(mDT));
	__ mov(mDT, Operand(mDT, LSL, 24), SetCC);
	__ mov(mDT, Operand(mDT, LSR, 24));
}

/*!
	WT  = A:P.C
	P.C = WT[8]
	A   = WT[7:0]
	P.Z = (A == 0)
	P.N = A[7]
 */
inline void NesCpuTranslator::mRolAcc()
{
	__ orr(mA, mA, Operand(1 << 23), LeaveCC, cs);
	__ mov(mA, Operand(mA, LSL, 1), SetCC);
}

/*!
	WT  = P.C:DT[7:0]
	P.C = WT[0]
	DT  = WT[8:1]
	P.Z = (DT == 0)
	P.N = DT[7]
 */
inline void NesCpuTranslator::mRor()
{
	__ mov(mDT, Operand(mDT, RRX, 0), SetCC);
	__ orr(mDT, mDT, Operand(0x80), LeaveCC, mi);
	__ and_(mDT, mDT, Operand(0xff), LeaveCC, mi);
}

/*!
	WT  = P.C:A[7:0]
	P.C = WT[0]
	A   = WT[8:1]
	P.Z = (A == 0)
	P.N = A[7]
 */
inline void NesCpuTranslator::mRorAcc()
{
	__ mov(mA, Operand(mA, LSR, 24));
	__ mov(mA, Operand(mA, RRX, 0), SetCC);
	__ orr(mA, mA, Operand(0x80), LeaveCC, mi);
	__ mov(mA, Operand(mA, LSL, 24));
}

#endif // NESCPUREC_SHIFT_H
