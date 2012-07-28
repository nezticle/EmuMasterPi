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

#ifndef ARM_ASSEM_LDSTR_H
#define ARM_ASSEM_LDSTR_H

#include "assem_addrmod.h"

namespace Arm {

// Load/Store instructions.
inline void Assembler::ldr(Register dst, const MemOperand& src, Condition cond)
{
	addrmod2(cond | B26 | L, dst, src);
}


inline void Assembler::str(Register src, const MemOperand& dst, Condition cond)
{
	addrmod2(cond | B26, src, dst);
}


inline void Assembler::ldrb(Register dst, const MemOperand& src, Condition cond)
{
	addrmod2(cond | B26 | B | L, dst, src);
}


inline void Assembler::strb(Register src, const MemOperand& dst, Condition cond)
{
	addrmod2(cond | B26 | B, src, dst);
}


inline void Assembler::ldrh(Register dst, const MemOperand& src, Condition cond)
{
	addrmod3(cond | L | B7 | H | B4, dst, src);
}


inline void Assembler::strh(Register src, const MemOperand& dst, Condition cond)
{
	addrmod3(cond | B7 | H | B4, src, dst);
}


inline void Assembler::ldrsb(Register dst, const MemOperand& src, Condition cond)
{
	addrmod3(cond | L | B7 | S6 | B4, dst, src);
}


inline void Assembler::ldrsh(Register dst, const MemOperand& src, Condition cond)
{
	addrmod3(cond | L | B7 | S6 | H | B4, dst, src);
}


#if defined(CAN_USE_ARMV7_INSTRUCTIONS)
inline void Assembler::ldrd(Register dst1, Register dst2,
							const MemOperand& src, Condition cond)
{
	Q_UNUSED(dst2)
	Q_ASSERT(src.rm().is(no_reg));
	Q_ASSERT(!dst1.is(lr));  // r14.
	Q_ASSERT_EQ(0, dst1.code() % 2);
	Q_ASSERT_EQ(dst1.code() + 1, dst2.code());
	addrmod3(cond | B7 | B6 | B4, dst1, src);
}
#endif

#if defined(CAN_USE_ARMV7_INSTRUCTIONS)
inline void Assembler::strd(Register src1, Register src2,
							const MemOperand& dst, Condition cond)
{
	Q_UNUSED(src2)
	Q_ASSERT(dst.rm().is(no_reg));
	Q_ASSERT(!src1.is(lr));  // r14.
	Q_ASSERT_EQ(0, src1.code() % 2);
	Q_ASSERT_EQ(src1.code() + 1, src2.code());
	addrmod3(cond | B7 | B6 | B5 | B4, src1, dst);
}
#endif

} // namespace Arm

#endif // ARM_ASSEM_LDSTR_H
