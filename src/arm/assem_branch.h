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

#ifndef ARM_ASSEM_BRANCH_H
#define ARM_ASSEM_BRANCH_H

#include "assembler.h"

namespace Arm {

// Branch instructions.

inline void Assembler::b(int branch_offset, Condition cond)
{
	Q_ASSERT((branch_offset & 3) == 0);
	int imm24 = branch_offset >> 2;
	Q_ASSERT(isInt24(imm24));
	iemit(cond | B27 | B25 | (imm24 & kImm24Mask));

	if (cond == al) {
		// Dead code is a good location to emit the constant pool.
		constPoolCheck(false, false);
	}
}

inline void Assembler::bl(int branch_offset, Condition cond)
{
	Q_ASSERT((branch_offset & 3) == 0);
	int imm24 = branch_offset >> 2;
	Q_ASSERT(isInt24(imm24));
	iemit(cond | B27 | B25 | B24 | (imm24 & kImm24Mask));
}

inline void Assembler::blx(int branch_offset)  // v5 and above
{
	Q_ASSERT((branch_offset & 1) == 0);
	int h = ((branch_offset & 2) >> 1)*B24;
	int imm24 = branch_offset >> 2;
	Q_ASSERT(isInt24(imm24));
	iemit(kSpecialCondition | B27 | B25 | h | (imm24 & kImm24Mask));
}

inline void Assembler::blx(Register target, Condition cond)  // v5 and above
{
	Q_ASSERT(!target.is(pc));
	iemit(cond | B24 | B21 | 15*B16 | 15*B12 | 15*B8 | BLX | target.code());
}

inline void Assembler::bx(Register target, Condition cond)  // v5 and above, plus v4t
{
	Q_ASSERT(!target.is(pc));  // use of pc is actually allowed, but discouraged
	iemit(cond | B24 | B21 | 15*B16 | 15*B12 | 15*B8 | BX | target.code());
}

} // namespace Arm

#endif // ARM_ASSEM_BRANCH_H
