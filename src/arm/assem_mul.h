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

#ifndef ARM_ASSEM_MUL_H
#define ARM_ASSEM_MUL_H

#include "assembler.h"

namespace Arm {

// Multiply instructions.
inline void Assembler::mla(Register dst, Register src1, Register src2, Register srcA,
						   SBit s, Condition cond)
{
	Q_ASSERT(!dst.is(pc) && !src1.is(pc) && !src2.is(pc) && !srcA.is(pc));
	iemit(cond | A | s | dst.code()*B16 | srcA.code()*B12 |
		  src2.code()*B8 | B7 | B4 | src1.code());
}


inline void Assembler::mul(Register dst, Register src1, Register src2,
						   SBit s, Condition cond)
{
	Q_ASSERT(!dst.is(pc) && !src1.is(pc) && !src2.is(pc));
	// dst goes in bits 16-19 for this instruction!
	iemit(cond | s | dst.code()*B16 | src2.code()*B8 | B7 | B4 | src1.code());
}


inline void Assembler::smlal(Register dstL,
							 Register dstH,
							 Register src1,
							 Register src2,
							 SBit s,
							 Condition cond)
{
	Q_ASSERT(!dstL.is(pc) && !dstH.is(pc) && !src1.is(pc) && !src2.is(pc));
	Q_ASSERT(!dstL.is(dstH));
	iemit(cond | B23 | B22 | A | s | dstH.code()*B16 | dstL.code()*B12 |
		  src2.code()*B8 | B7 | B4 | src1.code());
}


inline void Assembler::smull(Register dstL,
							 Register dstH,
							 Register src1,
							 Register src2,
							 SBit s,
							 Condition cond)
{
	Q_ASSERT(!dstL.is(pc) && !dstH.is(pc) && !src1.is(pc) && !src2.is(pc));
	Q_ASSERT(!dstL.is(dstH));
	iemit(cond | B23 | B22 | s | dstH.code()*B16 | dstL.code()*B12 |
		  src2.code()*B8 | B7 | B4 | src1.code());
}


inline void Assembler::umlal(Register dstL,
							 Register dstH,
							 Register src1,
							 Register src2,
							 SBit s,
							 Condition cond)
{
	Q_ASSERT(!dstL.is(pc) && !dstH.is(pc) && !src1.is(pc) && !src2.is(pc));
	Q_ASSERT(!dstL.is(dstH));
	iemit(cond | B23 | A | s | dstH.code()*B16 | dstL.code()*B12 |
		  src2.code()*B8 | B7 | B4 | src1.code());
}


inline void Assembler::umull(Register dstL,
							 Register dstH,
							 Register src1,
							 Register src2,
							 SBit s,
							 Condition cond)
{
	Q_ASSERT(!dstL.is(pc) && !dstH.is(pc) && !src1.is(pc) && !src2.is(pc));
	Q_ASSERT(!dstL.is(dstH));
	iemit(cond | B23 | s | dstH.code()*B16 | dstL.code()*B12 |
		  src2.code()*B8 | B7 | B4 | src1.code());
}

} // namespace Arm

#endif // ARM_ASSEM_MUL_H
