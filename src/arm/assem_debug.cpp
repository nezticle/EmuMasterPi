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

#include "assem_all.h"

namespace Arm {

// Exception-generating instructions and debugging support.
// Stops with a non-negative code less than kNumOfWatchedStops support
// enabling/disabling and a counter feature. See simulator-arm.h .
void Assembler::stop(const char* msg, Condition cond, s32 code)
{
	Q_UNUSED(msg)
	Q_UNUSED(code)
#if defined(CAN_USE_ARMV5_INSTRUCTIONS)
	if (cond != al) {
		Label skip;
		b(&skip, NegateCondition(cond));
		bkpt(0);
		bind(&skip);
	} else {
		bkpt(0);
	}
#else
	svc(0x9f0001, cond);
#endif
}

void Assembler::bkpt(u32 imm16)
{
	// v5 and above
	Q_ASSERT(isUint16(imm16));
	iemit(al | B24 | B21 | (imm16 >> 4)*B8 | BKPT | (imm16 & 0xf));
}

} // namespace Arm
