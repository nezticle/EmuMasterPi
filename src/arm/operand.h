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

#ifndef ARM_OPERAND_H
#define ARM_OPERAND_H

#include "instruction.h"
#include "utils.h"
#include "shifter.h"

namespace Arm {

// -----------------------------------------------------------------------------
// Machine instruction Operands

// Class Operand represents a shifter operand in data processing instructions
class Operand BASE_EMBEDDED
{
public:
	// immediate
	explicit Operand(s32 immediate);
	static Operand Zero();
	// rm
	explicit Operand(Register rm);

	// rm <shift_op> shift_imm
	explicit Operand(Register rm, ShiftOp shiftOp, int shift_imm);

	// rm <shift_op> rs
	explicit Operand(Register rm, ShiftOp shiftOp, Register rs);

	// Return true if this is a register operand.
	bool isReg() const;

	// Return true if this operand fits in one instruction so that no
	// 2-instruction solution with a load into the ip register is necessary. If
	// the instruction this operand is used for is a MOV or MVN instruction the
	// actual instruction to use is required for this calculation. For other
	// instructions instr is ignored.
	bool isSingleInstruction(Instr instr = 0) const;

	s32 immediate() const;

	Register rm() const { return rm_; }
	Register rs() const { return rs_; }
	ShiftOp shiftOp() const { return shift_op_; }

private:
	Register rm_;
	Register rs_;
	ShiftOp shift_op_;
	int shift_imm_;  // valid if rm_ != no_reg && rs_ == no_reg
	s32 imm32_;  // valid if rm_ == no_reg

	friend class Assembler;
};

inline Operand::Operand(s32 immediate)
{
	rm_ = no_reg;
	imm32_ = immediate;
}

inline Operand Operand::Zero()
{
	return Operand(static_cast<s32>(0));
}

inline Operand::Operand(Register rm)
{
	rm_ = rm;
	rs_ = no_reg;
	shift_op_ = LSL;
	shift_imm_ = 0;
}

inline Operand::Operand(Register rm, ShiftOp shift_op, int shift_imm)
{
	Q_ASSERT(isUint5(shift_imm));
	Q_ASSERT(shift_op != ROR || shift_imm != 0);  // use RRX if you mean it
	rm_ = rm;
	rs_ = no_reg;
	shift_op_ = shift_op;
	shift_imm_ = shift_imm & 31;
	if (shift_op == RRX) {
		// encoded as ROR with shift_imm == 0
		Q_ASSERT(shift_imm == 0);
		shift_op_ = ROR;
		shift_imm_ = 0;
	}
}

inline Operand::Operand(Register rm, ShiftOp shift_op, Register rs)
{
	Q_ASSERT(shift_op != RRX);
	rm_ = rm;
	rs_ = no_reg;
	shift_op_ = shift_op;
	rs_ = rs;
}

inline bool Operand::isReg() const
{
	return rm_.isValid() &&
			rs_.is(no_reg) &&
			shift_op_ == LSL &&
			shift_imm_ == 0;
}

inline s32 Operand::immediate() const
{
	Q_ASSERT(!rm_.isValid());
	return imm32_;
}

inline bool Operand::isSingleInstruction(Instr instr) const
{
	if (rm_.isValid())
		return true;
	u32 dummy1, dummy2;
	if (!fitsShifter(imm32_, &dummy1, &dummy2, &instr)) {
		// The immediate operand cannot be encoded as a shifter operand, or use of
		// constant pool is required. For a mov instruction not setting the
		// condition code additional instruction conventions can be used.
		if ((instr & ~kCondMask) == 13*B21) {  // mov, S not set
#if !defined(CAN_USE_ARMV7_INSTRUCTIONS)
			// mov instruction will be an ldr from constant pool (one instruction).
			return true;
#else
			// mov instruction will be a mov or movw followed by movt (two
			// instructions).
			return false;
#endif
		} else {
			// If this is not a mov or mvn instruction there will always an additional
			// instructions - either mov or ldr. The mov might actually be two
			// instructions mov or movw followed by movt so including the actual
			// instruction two or three instructions will be generated.
			return false;
		}
	} else {
		// No use of constant pool and the immediate operand can be encoded as a
		// shifter operand.
		return true;
	}
}

} // namespace Arm

#endif // ARM_OPERAND_H
