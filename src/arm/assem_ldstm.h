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

#ifndef ARM_ASSEM_LDSTM_H
#define ARM_ASSEM_LDSTM_H

#include "assem_addrmod.h"
#include "frames.h"

namespace Arm {

// Load/Store multiple instructions.
inline void Assembler::ldm(BlockAddrMode am,
						   Register base,
						   RegList dst,
						   Condition cond)
{
	// ABI stack constraint: ldmxx base, {..sp..}  base != sp  is not restartable.
	Q_ASSERT(base.is(sp) || (dst & sp.bit()) == 0);

	addrmod4(cond | B27 | am | L, base, dst);

	// Emit the constant pool after a function return implemented by ldm ..{..pc}.
	if (cond == al && (dst & pc.bit()) != 0) {
		// There is a slight chance that the ldm instruction was actually a call,
		// in which case it would be wrong to return into the constant pool; we
		// recognize this case by checking if the emission of the pool was blocked
		// at the pc of the ldm instruction by a mov lr, pc instruction; if this is
		// the case, we emit a jump over the pool.
		constPoolCheck(true, m_noConstPoolBefore == pcOffset() - kInstrSize);
	}
}


inline void Assembler::stm(BlockAddrMode am,
						   Register base,
						   RegList src,
						   Condition cond)
{
	addrmod4(cond | B27 | am, base, src);
}

} // namespace Arm

#endif // ARM_ASSEM_LDSTM_H
