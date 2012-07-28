/*
	Copyright (c) 1994-2006 Sun Microsystems Inc.
	All Rights Reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

	- Redistributions of source code must retain the above copyright notice,
	this list of conditions and the following disclaimer.

	- Redistribution in binary form must reproduce the above copyright
	notice, this list of conditions and the following disclaimer in the
	documentation and/or other materials provided with the
	distribution.

	- Neither the name of Sun Microsystems or the names of contributors may
	be used to endorse or promote products derived from this software without
	specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
	FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
	HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
	STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
	OF THE POSSIBILITY OF SUCH DAMAGE.

	The original source code covered by the above license above has been
	modified significantly by Google Inc.
	Copyright 2012 the V8 project authors. All rights reserved.

	The original source code covered by the above license above has been
	modified by elemental in order to fit in EmuMaster
 */

#ifndef ARM_ASSEM_ADDRMOD_H
#define ARM_ASSEM_ADDRMOD_H

#include "assembler.h"
#include "utils.h"

namespace Arm {

inline void Assembler::addrmod1(Instr instr,
								Register rn,
								Register rd,
								const Operand& x)
{
	checkBuffer();
	Q_ASSERT((instr & ~(kCondMask | kOpCodeMask | S)) == 0);
	if (!x.rm_.isValid()) {
		// Immediate.
		u32 rotate_imm;
		u32 immed_8;
		if (!fitsShifter(x.imm32_, &rotate_imm, &immed_8, &instr)) {
			// The immediate operand cannot be encoded as a shifter operand, so load
			// it first to register ip and change the original instruction to use ip.
			// However, if the original instruction is a 'mov rd, x' (not setting the
			// condition code), then replace it with a 'ldr rd, [pc]'.
			Q_ASSERT(!rn.is(ip));  // rn should never be ip, or will be trashed
			Condition cond = Instruction::ConditionField(instr);
			if ((instr & ~kCondMask) == kMovMvnPattern) {  // mov, S not set
#if !defined(CAN_USE_ARMV7_INSTRUCTIONS)
				constPoolRecordLdrStrItem(x.imm32_);
				ldr(rd, MemOperand(pc, 0), cond);
#else
				// Will probably use movw, will certainly not use constant pool.
				mov(rd, Operand(x.imm32_ & 0xffff), LeaveCC, cond);
				movt(rd, static_cast<u32>(x.imm32_) >> 16, cond);
#endif
			} else {
				// If this is not a mov or mvn instruction we may still be able to avoid
				// a constant pool entry by using mvn or movw.
				if ((instr & kMovMvnMask) != kMovMvnPattern) {
					mov(ip, x, LeaveCC, cond);
				} else {
					constPoolRecordLdrStrItem(x.imm32_);
					ldr(ip, MemOperand(pc, 0), cond);
				}
				addrmod1(instr, rn, rd, Operand(ip));
			}
			return;
		}
		instr |= I | rotate_imm*B8 | immed_8;
	} else if (!x.rs_.isValid()) {
		// Immediate shift.
		instr |= x.shift_imm_*B7 | x.shift_op_ | x.rm_.code();
	} else {
		// Register shift.
		Q_ASSERT(!rn.is(pc) && !rd.is(pc) && !x.rm_.is(pc) && !x.rs_.is(pc));
		instr |= x.rs_.code()*B8 | x.shift_op_ | B4 | x.rm_.code();
	}
	iemit(instr | rn.code()*B16 | rd.code()*B12);
	if (rn.is(pc) || x.rm_.is(pc)) {
		// Block constant pool emission for one instruction after reading pc.
		constPoolBlockFor(1);
	}
}


inline void Assembler::addrmod2(Instr instr, Register rd, const MemOperand& x)
{
	Q_ASSERT((instr & ~(kCondMask | B | L)) == B26);
	int am = x.am_;
	if (!x.rm_.isValid()) {
		// Immediate offset.
		int offset_12 = x.offset_;
		if (offset_12 < 0) {
			offset_12 = -offset_12;
			am ^= U;
		}
		if (!isUint12(offset_12)) {
			// Immediate offset cannot be encoded, load it first to register ip
			// rn (and rd in a load) should never be ip, or will be trashed.
			Q_ASSERT(!x.rn_.is(ip) && ((instr & L) == L || !rd.is(ip)));
			mov(ip, Operand(x.offset_), LeaveCC, Instruction::ConditionField(instr));
			addrmod2(instr, rd, MemOperand(x.rn_, ip, x.am_));
			return;
		}
		Q_ASSERT(offset_12 >= 0);  // no masking needed
		instr |= offset_12;
	} else {
		// Register offset (shift_imm_ and shift_op_ are 0) or scaled
		// register offset the constructors make sure than both shift_imm_
		// and shift_op_ are initialized.
		Q_ASSERT(!x.rm_.is(pc));
		instr |= B25 | x.shift_imm_*B7 | x.shift_op_ | x.rm_.code();
	}
	Q_ASSERT((am & (P|W)) == P || !x.rn_.is(pc));  // no pc base with writeback
	iemit(instr | am | x.rn_.code()*B16 | rd.code()*B12);
}


inline void Assembler::addrmod3(Instr instr, Register rd, const MemOperand& x)
{
	Q_ASSERT((instr & ~(kCondMask | L | S6 | H)) == (B4 | B7));
	Q_ASSERT(x.rn_.isValid());
	int am = x.am_;
	if (!x.rm_.isValid()) {
		// Immediate offset.
		int offset_8 = x.offset_;
		if (offset_8 < 0) {
			offset_8 = -offset_8;
			am ^= U;
		}
		if (!isUint8(offset_8)) {
			// Immediate offset cannot be encoded, load it first to register ip
			// rn (and rd in a load) should never be ip, or will be trashed.
			Q_ASSERT(!x.rn_.is(ip) && ((instr & L) == L || !rd.is(ip)));
			mov(ip, Operand(x.offset_), LeaveCC, Instruction::ConditionField(instr));
			addrmod3(instr, rd, MemOperand(x.rn_, ip, x.am_));
			return;
		}
		Q_ASSERT(offset_8 >= 0);  // no masking needed
		instr |= B | (offset_8 >> 4)*B8 | (offset_8 & 0xf);
	} else if (x.shift_imm_ != 0) {
		// Scaled register offset not supported, load index first
		// rn (and rd in a load) should never be ip, or will be trashed.
		Q_ASSERT(!x.rn_.is(ip) && ((instr & L) == L || !rd.is(ip)));
		mov(ip, Operand(x.rm_, x.shift_op_, x.shift_imm_), LeaveCC,
			Instruction::ConditionField(instr));
		addrmod3(instr, rd, MemOperand(x.rn_, ip, x.am_));
		return;
	} else {
		// Register offset.
		Q_ASSERT((am & (P|W)) == P || !x.rm_.is(pc));  // no pc index with writeback
		instr |= x.rm_.code();
	}
	Q_ASSERT((am & (P|W)) == P || !x.rn_.is(pc));  // no pc base with writeback
	iemit(instr | am | x.rn_.code()*B16 | rd.code()*B12);
}


inline void Assembler::addrmod4(Instr instr, Register rn, RegList rl)
{
	Q_ASSERT((instr & ~(kCondMask | P | U | W | L)) == B27);
	Q_ASSERT(rl != 0);
	Q_ASSERT(!rn.is(pc));
	iemit(instr | rn.code()*B16 | rl);
}


inline void Assembler::addrmod5(Instr instr, CRegister crd, const MemOperand& x)
{
	// Unindexed addressing is not encoded by this function.
	Q_ASSERT_EQ((B27 | B26), (instr & ~(kCondMask | kCoprocessorMask | P | U | N | W | L)));
	Q_ASSERT(x.rn_.isValid() && !x.rm_.isValid());
	int am = x.am_;
	int offset_8 = x.offset_;
	Q_ASSERT((offset_8 & 3) == 0);  // offset must be an aligned word offset
	offset_8 >>= 2;
	if (offset_8 < 0) {
		offset_8 = -offset_8;
		am ^= U;
	}
	Q_ASSERT(isUint8(offset_8));  // unsigned word offset must fit in a byte
	Q_ASSERT((am & (P|W)) == P || !x.rn_.is(pc));  // no pc base with writeback

	// Post-indexed addressing requires W == 1; different than in addrmod2/3.
	if ((am & P) == 0)
		am |= W;

	Q_ASSERT(offset_8 >= 0);  // no masking needed
	iemit(instr | am | x.rn_.code()*B16 | crd.code()*B12 | offset_8);
}

} // namespace Arm

#endif // ARM_ASSEM_ADDRMOD_H
