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

#ifndef ARM_ASSEM_PSEUDO_H
#define ARM_ASSEM_PSEUDO_H

#include "assembler.h"

namespace Arm {

// Pseudo instructions.

inline void Assembler::nop(int type)
{
	// This is mov rx, rx.
	Q_ASSERT(0 <= type && type <= 14);  // mov pc, pc is not a nop.
	iemit(al | 13*B21 | type*B12 | type);
}


inline bool Assembler::IsNop(Instr instr, int type)
{
	// Check for mov rx, rx where x = type.
	Q_ASSERT(0 <= type && type <= 14);  // mov pc, pc is not a nop.
	return instr == (al | 13*B21 | type*B12 | type);
}

inline void Assembler::align(int m)
{
	Q_ASSERT(m >= 4 && isPowerOf2(m));
	while ((pcOffset() & (m - 1)) != 0)
		nop();
}

inline void Assembler::codeTargetAlign()
{
	// Preferred alignment of jump targets on some ARM chips.
	align(8);
}

inline void Assembler::push(Register src, Condition cond)
{
	str(src, MemOperand(sp, 4, NegPreIndex), cond);
}

inline void Assembler::pop(Register dst, Condition cond)
{
	ldr(dst, MemOperand(sp, 4, PostIndex), cond);
}

inline void Assembler::pop()
{
	add(sp, sp, Operand(kPointerSize));
}

} // namespace Arm

#endif // ARM_ASSEM_PSEUDO_H
