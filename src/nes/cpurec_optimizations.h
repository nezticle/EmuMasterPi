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

#ifndef NESCPUREC_OPTIMIZATIONS_H
#define NESCPUREC_OPTIMIZATIONS_H

#include "cpurec_p.h"

bool NesCpuTranslator::mTryOptimize()
{
	u8 op = nesCpuReadDirect(m_recPc);
	bool optimized;
	switch (op) {
	case 0x0a:	optimized = mOptimAslAcc(); break;
	case 0x0c:	optimized = mOptimNop(op); break;
	case 0x1a:	optimized = mOptimNop(op); break;
	case 0x1c:	optimized = mOptimNop(op); break;
	case 0x3a:	optimized = mOptimNop(op); break;
	case 0x3c:	optimized = mOptimNop(op); break;
	case 0x4a:	optimized = mOptimLsrAcc(); break;
	case 0x5a:	optimized = mOptimNop(op); break;
	case 0x5c:	optimized = mOptimNop(op); break;
	case 0x7a:	optimized = mOptimNop(op); break;
	case 0x7c:	optimized = mOptimNop(op); break;
	case 0x2c:	optimized = mOptimBitAbs(); break;
	case 0x4c:	optimized = mOptimJmpAbs(); break;
	case 0xad:	optimized = mOptimLdaAbs(); break;
	case 0x88:	optimized = mOptimDecReg(0x88, mY); break;
	case 0xa5:	optimized = mOptimLdaZpg(); break;
	case 0xca:	optimized = mOptimDecReg(0xca, mX); break;
	case 0xc5:	optimized = mOptimCmpZpg(); break;
	case 0xc8:	optimized = mOptimIncReg(0xc8, mY); break;
	case 0xda:	optimized = mOptimNop(op); break;
	case 0xdc:	optimized = mOptimNop(op); break;
	case 0xea:	optimized = mOptimNop(op); break;
	case 0xe8:	optimized = mOptimIncReg(0xe8, mX); break;
	case 0xfa:	optimized = mOptimNop(op); break;
	case 0xfc:	optimized = mOptimNop(op); break;
	default:	optimized = false; break;
	}
	return optimized;
}

bool NesCpuTranslator::mOptimAslAcc()
{
	int count = 1;
	while (nesCpuReadDirect(m_recPc + count) == 0x0a)
		count++;
	if (count <= 1)
		return false;

	__ mov(mA, Operand(mA, LSL, qMin(count, 31)), SetCC);
	mAddCycles(count * 2);
	m_recPc += count;
	return true;
}

bool NesCpuTranslator::mOptimLsrAcc()
{
	int count = 1;
	while (nesCpuReadDirect(m_recPc + count) == 0x4a)
		count++;
	if (count <= 1)
		return false;

	__ mov(mA, Operand(mA, LSR, 24));
	__ mov(mA, Operand(mA, LSR, qMin(count, 31)), SetCC);
	__ mov(mA, Operand(mA, LSL, 24));
	mAddCycles(count * 2);
	m_recPc += count;
	return true;
}

bool NesCpuTranslator::mOptimBitAbs()
{
	u8 lo = nesCpuReadDirect(m_recPc+1);
	u8 hi = nesCpuReadDirect(m_recPc+2);
	u16 addr = lo | (hi << 8);

	if (addr != 0x2002) // PPU Status Register
		return false;

	u8 nextOp = nesCpuReadDirect(m_recPc+3);
	Condition cond = bxxToCondition(nextOp);
	if (cond == kNoCondition)
		return false;

	return mOptimBitAbsBxx(cond);
}

bool NesCpuTranslator::mOptimBitAbsBxx(Condition cond)
{
	u8 disp = nesCpuReadDirect(m_recPc+4);
	if (disp != 0xfb)
		return false;

	Label begin;
	Label end;
	__ bind(&begin);
	mRead8(0x2002, mDT, 0);
	mBit();
	mAddCycles(4+2, NegateCondition(cond));
	__ b(&end, NegateCondition(cond));
	u16 ET = m_recPc + 5;
	u16 EA = m_recPc;
	int cycles = 4+2+1;
	bool boundary = ((ET^EA) >> 8) & 1;
	if (boundary)
		cycles++;
	mSaturateCycles(cycles);
	__ mov(r0, Operand(m_recPc));
	__ bl(&m_syncLabel);
	__ b(&begin);
	__ bind(&end);
	m_recPc += 5;
	return true;
}

bool NesCpuTranslator::mOptimJmpAbs()
{
	u8 lo = nesCpuReadDirect(m_recPc+1);
	u8 hi = nesCpuReadDirect(m_recPc+2);
	u16 dst = lo | (hi << 8);

	if (dst != m_recPc)
		return false;

	Label begin;
	__ bind(&begin);
	mSaturateCycles(3);
	__ mov(r0, Operand(m_recPc));
	__ bl(&m_syncLabel);
	__ b(&begin);
	m_recPc += 3;
	return true;
}

bool NesCpuTranslator::mOptimLdaAbs()
{
	u8 lo = nesCpuReadDirect(m_recPc+1);
	u8 hi = nesCpuReadDirect(m_recPc+2);
	u16 addr = lo | (hi << 8);

	if (addr != 0x2002) // PPU Status Register
		return false;

	u8 nextOp = nesCpuReadDirect(m_recPc+3);
	if (nextOp == 0x10) // bpl
		return mOptimLdaAbsBpl();

	if (nextOp == 0x29) // and
		return mOptimLdaAbsAndImm();

	return false;
}

bool NesCpuTranslator::mOptimLdaAbsBpl()
{
	u8 disp = nesCpuReadDirect(m_recPc+4);
	if (disp != 0xfb)
		return false;

	Label begin;
	Label end;
	__ bind(&begin);
	mRead8(0x2002, mDT, 0);
	mLda();
	mAddCycles(3+2, mi);
	__ b(&end, mi);
	u16 ET = m_recPc + 5;
	u16 EA = m_recPc;
	int cycles = 3+2+1;
	bool boundary = ((ET^EA) >> 8) & 1;
	if (boundary)
		cycles++;
	mSaturateCycles(cycles);
	__ mov(r0, Operand(m_recPc));
	__ bl(&m_syncLabel);
	__ b(&begin);
	__ bind(&end);
	m_recPc += 5;
	return true;
}

bool NesCpuTranslator::mOptimLdaAbsAndImm()
{
	u8 nextOp = nesCpuReadDirect(m_recPc+5);
	if (nextOp == 0xd0 || nextOp == 0xf0) {
		Condition cond = ((nextOp == 0xd0) ? ne : eq);
		return mOptimLdaAbsAndImmBneBeq(cond);
	}
	return false;
}

bool NesCpuTranslator::mOptimLdaAbsAndImmBneBeq(Condition cond)
{
	u8 disp = nesCpuReadDirect(m_recPc+6);
	if (disp != 0xf9)
		return false;

	Label begin;
	Label end;
	__ bind(&begin);
	// lda, and
	mRead8(0x2002, mDT, 0);
	__ mov(mA, Operand(mDT, LSL, 24));
	__ mov(mDT, Operand(nesCpuReadDirect(m_recPc+4) << 24)); // do not touch P.C
	__ and_(mA, mA, Operand(mDT), SetCC);
	// bxx
	mAddCycles(3+2+2, NegateCondition(cond));
	__ b(&end, NegateCondition(cond));
	u16 ET = m_recPc + 7;
	u16 EA = m_recPc;
	int cycles = 3+2+2+1;
	bool boundary = ((ET^EA) >> 8) & 1;
	if (boundary)
		cycles++;
	mSaturateCycles(cycles);
	__ mov(r0, Operand(m_recPc));
	__ bl(&m_syncLabel);
	__ b(&begin);
	__ bind(&end);
	m_recPc += 7;
	return true;
}

bool NesCpuTranslator::mOptimIncReg(u8 op, Register reg)
{
	int count = 1;
	while (nesCpuReadDirect(m_recPc + count) == op)
		count++;

	mIncRegMultiple(reg, count);
	mAddCycles(count * 2);
	m_recPc += count;
	return true;
}

bool NesCpuTranslator::mOptimDecReg(u8 op, Register reg)
{
	if (nesCpuReadDirect(m_recPc+1) == 0xd0)
		return mOptimDecRegBne(reg);

	int count = 1;
	while (nesCpuReadDirect(m_recPc + count) == op)
		count++;

	mDecRegMultiple(reg, count);
	mAddCycles(count * 2);
	m_recPc += count;
	return true;
}

bool NesCpuTranslator::mOptimDecRegBne(Register reg)
{
	u8 disp = nesCpuReadDirect(m_recPc+2);
	if (disp != 0xfd)
		return false;

	u16 ET = m_recPc + 3;
	u16 EA = m_recPc;
	int cycles = 2+2+1;
	bool boundary = ((ET^EA) >> 8) & 1;
	if (boundary)
		cycles++;

	__ sub(ip, reg, Operand(1<<24));
	__ mov(ip, Operand(ip, LSR, 24));
	__ add(mCycles, mCycles, Operand(ip, LSL, 2));
	if (cycles == 5)
		__ add(mCycles, mCycles, Operand(ip));
	else if (cycles == 6)
		__ add(mCycles, mCycles, Operand(ip, LSL, 1));
	else
		UNREACHABLE();
	__ add(mCycles, mCycles, Operand(2+2));
	__ eor(reg, reg, Operand(reg), SetCC);
	mCheckSync();
	m_recPc += 3;
	return true;
}

bool NesCpuTranslator::mOptimCmpZpg()
{
	u8 nextOp = nesCpuReadDirect(m_recPc+2);

	Condition cond = bxxToCondition(nextOp);
	if (cond == kNoCondition)
		return false;
	return mOptimCmpZpgBxx(cond);
}

bool NesCpuTranslator::mOptimCmpZpgBxx(Condition cond)
{
	u8 disp = nesCpuReadDirect(m_recPc+3);
	if (disp != 0xfc)
		return false;

	Label begin;
	Label end;
	__ bind(&begin);
	mReadZp8(nesCpuReadDirect(m_recPc+1), mDT);
	mCmp();
	mAddCycles(3+2, NegateCondition(cond));
	__ b(&end, NegateCondition(cond));
	u16 ET = m_recPc + 4;
	u16 EA = m_recPc;
	int cycles = 3+2+1;
	bool boundary = ((ET^EA) >> 8) & 1;
	if (boundary)
		cycles++;
	mSaturateCycles(cycles);
	__ mov(r0, Operand(m_recPc));
	__ bl(&m_syncLabel);
	__ b(&begin);
	__ bind(&end);
	m_recPc += 4;
	return true;
}

bool NesCpuTranslator::mOptimLdaZpg()
{
	u8 nextOp = nesCpuReadDirect(m_recPc+2);

	Condition cond = bxxToCondition(nextOp);
	if (cond == kNoCondition)
		return false;
	return mOptimLdaZpgBxx(cond);
}

bool NesCpuTranslator::mOptimLdaZpgBxx(Condition cond)
{
	u8 disp = nesCpuReadDirect(m_recPc+3);
	if (disp != 0xfc)
		return false;

	Label begin;
	Label end;
	__ bind(&begin);
	mReadZp8(nesCpuReadDirect(m_recPc+1), mDT);
	mLda();
	mAddCycles(3+2, NegateCondition(cond));
	__ b(&end, NegateCondition(cond));
	u16 ET = m_recPc + 4;
	u16 EA = m_recPc;
	int cycles = 3+2+1;
	bool boundary = ((ET^EA) >> 8) & 1;
	if (boundary)
		cycles++;
	mSaturateCycles(cycles);
	__ mov(r0, Operand(m_recPc));
	__ bl(&m_syncLabel);
	__ b(&begin);
	__ bind(&end);
	m_recPc += 4;
	return true;
}

bool NesCpuTranslator::mOptimNop(u8 op)
{
	int count = 1;
	while (nesCpuReadDirect(m_recPc + count) == op)
		count++;

	int nopCycles = ((op&0x0f) == 0x0c) ? 4 : 2;
	bool nopJmpAbs = false;
	u8 nextOp = nesCpuReadDirect(m_recPc + count);
	if (nextOp == 0x4c)
		nopJmpAbs = mOptimNopJmpAbs(nopCycles, count);

	if (!nopJmpAbs) {
		mAddCycles(count * nopCycles);
		m_recPc += count;
	}
	return true;
}

bool NesCpuTranslator::mOptimNopJmpAbs(int nopCycles, int nopCount)
{
	u8 lo = nesCpuReadDirect(m_recPc+nopCount+1);
	u8 hi = nesCpuReadDirect(m_recPc+nopCount+2);
	u16 dst = lo | (hi << 8);

	if (dst < m_recPc || dst > m_recPc+nopCount)
		return false;

	int nNop = nopCount - (dst-m_recPc);
	Label begin;
	__ bind(&begin);
	mSaturateCycles(3+nNop*nopCycles);
	__ mov(r0, Operand(m_recPc));
	__ bl(&m_syncLabel);
	__ b(&begin);
	m_recPc += nopCount + 3;
	return true;
}

void NesCpuTranslator::mSaturateCycles(int modValue)
{
	if (modValue == 8) { // uncomment if needed || modValue == 4 || modValue == 2) {
		// executed at least one time
		__ add(ip, ip, Operand(modValue));
		// a clever trick to jump over the saturation if mCycles >= 0
		__ mov(ip, Operand(mCycles, LSR, 31));
		__ eor(ip, ip, Operand(1));
		__ add(pc, pc, Operand(ip, LSL, 2));
		// some pad needed - write current pc
		__ dd(currentPc());
		// if mCycles negative - saturate it
		__ and_(mCycles, mCycles, Operand(modValue-1));
		// continue otherwise
	} else {
		__ mrs(ip, CPSR);
		Label loop;
		__ bind(&loop);
		__ add(mCycles, mCycles, Operand(modValue), SetCC);
		__ b(&loop, mi);
		__ msr(CPSR_f, Operand(ip));
	}
}

Condition NesCpuTranslator::bxxToCondition(u8 op) const
{
	Condition cond;
	switch (op) {
	case 0x10:	cond = pl; break;
	case 0x30:	cond = mi; break;
	case 0x50:	cond = vc; break;
	case 0x70:	cond = vs; break;
	case 0xd0:	cond = ne; break;
	case 0xf0:	cond = eq; break;
	default:	cond = kNoCondition; break;
	}
	return cond;
}

#endif // NESCPUREC_OPTIMIZATIONS_H
