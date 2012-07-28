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

#ifndef ARM_ASSEM_STATUS_H
#define ARM_ASSEM_STATUS_H

#include "assembler.h"

namespace Arm {

// Status register access instructions.

inline void Assembler::mrs(Register dst, SRegister s, Condition cond)
{
	Q_ASSERT(!dst.is(pc));
	iemit(cond | B24 | s | 15*B16 | dst.code()*B12);
}

inline void Assembler::msr(SRegisterFieldMask fields, const Operand& src,
						   Condition cond)
{
	Q_ASSERT(fields >= B16 && fields < B20);  // at least one field set
	Instr instr;
	if (!src.rm_.isValid()) {
		// Immediate.
		u32 rotate_imm;
		u32 immed_8;
		if (!fitsShifter(src.imm32_, &rotate_imm, &immed_8, 0)) {
			// Immediate operand cannot be encoded, load it first to register ip.
			constPoolRecordLdrStrItem(src.imm32_);
			ldr(ip, MemOperand(pc, 0), cond);
			msr(fields, Operand(ip), cond);
			return;
		}
		instr = I | rotate_imm*B8 | immed_8;
	} else {
		Q_ASSERT(!src.rs_.isValid() && src.shift_imm_ == 0);  // only rm allowed
		instr = src.rm_.code();
	}
	iemit(cond | instr | B24 | B21 | fields | 15*B12);
}

} // namespace Arm

#endif // ARM_ASSEM_STATUS_H
