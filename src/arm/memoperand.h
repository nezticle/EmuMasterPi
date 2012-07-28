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

#ifndef ARM_MEMOPERAND_H
#define ARM_MEMOPERAND_H

#include "instruction.h"
#include "utils.h"

namespace Arm {

// Class MemOperand represents a memory operand in load and store instructions
class MemOperand BASE_EMBEDDED
{
public:
	// [rn +/- offset]      Offset/NegOffset
	// [rn +/- offset]!     PreIndex/NegPreIndex
	// [rn], +/- offset     PostIndex/NegPostIndex
	// offset is any signed 32-bit value; offset is first loaded to register ip if
	// it does not fit the addressing mode (12-bit unsigned and sign bit)
	explicit MemOperand(Register rn, s32 offset = 0, AddrMode am = Offset);

	// [rn +/- rm]          Offset/NegOffset
	// [rn +/- rm]!         PreIndex/NegPreIndex
	// [rn], +/- rm         PostIndex/NegPostIndex
	explicit MemOperand(Register rn, Register rm, AddrMode am = Offset);

	// [rn +/- rm <shift_op> shift_imm]      Offset/NegOffset
	// [rn +/- rm <shift_op> shift_imm]!     PreIndex/NegPreIndex
	// [rn], +/- rm <shift_op> shift_imm     PostIndex/NegPostIndex
	explicit MemOperand(Register rn, Register rm,
						ShiftOp shift_op, int shift_imm, AddrMode am = Offset);

	void setOffset(s32 offset);
	s32 offset() const;

	Register rn() const { return rn_; }
	Register rm() const { return rm_; }
	AddrMode am() const { return am_; }

	bool offsetIsUint12Encodable() const {
		return offset_ >= 0 ? isUint12(offset_) : isUint12(-offset_);
	}

private:
	Register rn_;  // base
	Register rm_;  // register offset
	s32 offset_;  // valid if rm_ == no_reg
	ShiftOp shift_op_;
	int shift_imm_;  // valid if rm_ != no_reg && rs_ == no_reg
	AddrMode am_;  // bits P, U, and W

	friend class Assembler;
};

inline MemOperand::MemOperand(Register rn, s32 offset, AddrMode am)
{
	rn_ = rn;
	rm_ = no_reg;
	offset_ = offset;
	am_ = am;
}

inline MemOperand::MemOperand(Register rn, Register rm, AddrMode am)
{
	rn_ = rn;
	rm_ = rm;
	shift_op_ = LSL;
	shift_imm_ = 0;
	am_ = am;
}

inline MemOperand::MemOperand(Register rn, Register rm,
							  ShiftOp shift_op, int shift_imm, AddrMode am)
{
	Q_ASSERT(isUint5(shift_imm));
	rn_ = rn;
	rm_ = rm;
	shift_op_ = shift_op;
	shift_imm_ = shift_imm & 31;
	am_ = am;
}

inline void MemOperand::setOffset(s32 offset)
{
	Q_ASSERT(rm_.is(no_reg));
	offset_ = offset;
}

inline s32 MemOperand::offset() const
{
	Q_ASSERT(rm_.is(no_reg));
	return offset_;
}

} // namespace Arm

#endif // ARM_MEMOPERAND_H
