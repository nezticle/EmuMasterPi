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

#ifndef ARM_ASSEM_DATA_H
#define ARM_ASSEM_DATA_H

#include "assem_addrmod.h"

namespace Arm {

// Data-processing instructions.

inline void Assembler::and_(Register dst, Register src1, const Operand& src2,
							SBit s, Condition cond)
{
	addrmod1(cond | AND | s, src1, dst, src2);
}


inline void Assembler::eor(Register dst, Register src1, const Operand& src2,
						   SBit s, Condition cond)
{
	addrmod1(cond | EOR | s, src1, dst, src2);
}


inline void Assembler::sub(Register dst, Register src1, const Operand& src2,
						   SBit s, Condition cond)
{
	addrmod1(cond | SUB | s, src1, dst, src2);
}


inline void Assembler::rsb(Register dst, Register src1, const Operand& src2,
						   SBit s, Condition cond)
{
	addrmod1(cond | RSB | s, src1, dst, src2);
}


inline void Assembler::add(Register dst, Register src1, const Operand& src2,
						   SBit s, Condition cond)
{
	addrmod1(cond | ADD | s, src1, dst, src2);
}


inline void Assembler::adc(Register dst, Register src1, const Operand& src2,
						   SBit s, Condition cond)
{
	addrmod1(cond | ADC | s, src1, dst, src2);
}


inline void Assembler::sbc(Register dst, Register src1, const Operand& src2,
						   SBit s, Condition cond)
{
	addrmod1(cond | SBC | s, src1, dst, src2);
}


inline void Assembler::rsc(Register dst, Register src1, const Operand& src2,
						   SBit s, Condition cond)
{
	addrmod1(cond | RSC | s, src1, dst, src2);
}


inline void Assembler::tst(Register src1, const Operand& src2, Condition cond)
{
	addrmod1(cond | TST | S, src1, r0, src2);
}


inline void Assembler::teq(Register src1, const Operand& src2, Condition cond)
{
	addrmod1(cond | TEQ | S, src1, r0, src2);
}


inline void Assembler::cmp(Register src1, const Operand& src2, Condition cond)
{
	addrmod1(cond | CMP | S, src1, r0, src2);
}


inline void Assembler::cmp_raw_immediate(
		Register src, int raw_immediate, Condition cond)
{
	Q_ASSERT(isUint12(raw_immediate));
	iemit(cond | I | CMP | S | src.code() << 16 | raw_immediate);
}


inline void Assembler::cmn(Register src1, const Operand& src2, Condition cond)
{
	addrmod1(cond | CMN | S, src1, r0, src2);
}


inline void Assembler::orr(Register dst, Register src1, const Operand& src2,
						   SBit s, Condition cond)
{
	addrmod1(cond | ORR | s, src1, dst, src2);
}

inline void Assembler::mov(Register dst, const Operand& src, SBit s, Condition cond)
{
	// Don't allow nop instructions in the form mov rn, rn to be generated using
	// the mov instruction. They must be generated using nop(int/NopMarkerTypes)
	// or MarkCode(int/NopMarkerTypes) pseudo instructions.
	Q_ASSERT(!(src.isReg() && src.rm().is(dst) && s == LeaveCC && cond == al));
	addrmod1(cond | MOV | s, r0, dst, src);
}

inline void Assembler::movw(Register reg, u32 immediate, Condition cond)
{
	Q_ASSERT(immediate < 0x10000);
	mov(reg, Operand(immediate), LeaveCC, cond);
}


inline void Assembler::movt(Register reg, u32 immediate, Condition cond)
{
	iemit(cond | 0x34*B20 | reg.code()*B12 | EncodeMovwImmediate(immediate));
}


inline void Assembler::bic(Register dst, Register src1, const Operand& src2,
						   SBit s, Condition cond)
{
	addrmod1(cond | BIC | s, src1, dst, src2);
}


inline void Assembler::mvn(Register dst, const Operand& src, SBit s, Condition cond)
{
	addrmod1(cond | MVN | s, r0, dst, src);
}

// Miscellaneous arithmetic instructions.
inline void Assembler::clz(Register dst, Register src, Condition cond)
{
	// v5 and above.
	Q_ASSERT(!dst.is(pc) && !src.is(pc));
	iemit(cond | B24 | B22 | B21 | 15*B16 | dst.code()*B12 |
		  15*B8 | CLZ | src.code());
}


// Saturating instructions.

// Unsigned saturate.
#if defined(CAN_USE_ARMV7_INSTRUCTIONS)
inline void Assembler::usat(Register dst,
							int satpos,
							const Operand& src,
							Condition cond)
{
	// v6 and above.
	Q_ASSERT(!dst.is(pc) && !src.rm_.is(pc));
	Q_ASSERT((satpos >= 0) && (satpos <= 31));
	Q_ASSERT((src.shift_op_ == ASR) || (src.shift_op_ == LSL));
	Q_ASSERT(src.rs_.is(no_reg));

	int sh = 0;
	if (src.shift_op_ == ASR)
		sh = 1;

	iemit(cond | 0x6*B24 | 0xe*B20 | satpos*B16 | dst.code()*B12 |
		  src.shift_imm_*B7 | sh*B6 | 0x1*B4 | src.rm_.code());
}
#endif

} // namespace  Arm

#endif // ARM_ASSEM_DATA_H
