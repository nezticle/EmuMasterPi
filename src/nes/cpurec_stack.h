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

#ifndef NESCPUREC_STACK_H
#define NESCPUREC_STACK_H

#include "cpurec_p.h"

/*!
	ram[StackBase+S] = data
	S--
 */
inline void NesCpuTranslator::mPush8(Register val)
{
	Q_ASSERT(val != ip);
	__ ldr(ip, MemOperand(mDataBase, offsetof(NesCpuRecData,stack)));
	__ strb(val, MemOperand(ip, mS, LSR, 24));
	__ sub(mS, mS, Operand(1 << 24));
}

/*!
	S++
	return ram[StackBase+S]
 */
inline void NesCpuTranslator::mPop8(Register val)
{
	__ ldr(val, MemOperand(mDataBase, offsetof(NesCpuRecData,stack)));
	__ add(mS, mS, Operand(1 << 24));
	__ ldrb(val, MemOperand(val, mS, LSR, 24));
}


/*!
	PUSH8(data>>8)
	PUSH8(data)
 */
inline void NesCpuTranslator::mPush16(Register val)
{
	Q_ASSERT(val != ip);
	__ ldr(ip, MemOperand(mDataBase, offsetof(NesCpuRecData,stack)));

	__ mov(val, Operand(val, ROR, 8));
	__ strb(val, MemOperand(ip, mS, LSR, 24));
	__ sub(mS, mS, Operand(1 << 24));

	__ mov(val, Operand(val, ROR, 24));
	__ strb(val, MemOperand(ip, mS, LSR, 24));
	__ sub(mS, mS, Operand(1 << 24));
}

/*!
	u8 lo = POP8()
	u8 hi = POP8()
	return lo | (hi<<8)
 */
inline void NesCpuTranslator::mPop16(Register val)
{
	Q_ASSERT(val != ip);
	__ ldr(ip, MemOperand(mDataBase, offsetof(NesCpuRecData,stack)));
	__ add(mS, mS, Operand(1 << 24));
	__ ldrb(val, MemOperand(ip, mS, LSR, 24));
	__ add(mS, mS, Operand(1 << 24));
	__ ldrb(ip, MemOperand(ip, mS, LSR, 24));
	__ orr(val, val, Operand(ip, LSL, 8));
}

/*!
	PUSH8(A)
 */
inline void NesCpuTranslator::mPha()
{
	__ mov(mDT, Operand(mA, LSR, 24));
	mPush8(mDT);
}

/*!
	A   = POP8()
	P.Z = (A == 0);
	P.N = A[7]
 */
inline void NesCpuTranslator::mPla()
{
	mPop8(mA);
	__ mov(mA, Operand(mA, LSL, 24));
	__ mov(mA, mA, SetCC);
}

/*!
	PUSH8(P|U|B)
 */
inline void NesCpuTranslator::mPhp()
{
	mPackFlags(mDT);
	__ orr(mDT, mDT, Operand(NesCpuBase::Break|NesCpuBase::Unused));
	mPush8(mDT);
}

/*!
	POP8(P)
 */
inline void NesCpuTranslator::mPlp()
{
	mPop8(mDT);
	mUnpackFlags(mDT, mWT);
}

#endif // NESCPUREC_STACK_H
