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

void Assembler::constPoolCheck(bool forceEmit, bool requireJump)
{
	// Some short sequence of instruction mustn't be broken up by constant pool
	// emission, such sequences are protected by calls to BlockConstPoolFor and
	// BlockConstPoolScope.
	if (constPoolIsBlocked()) {
		// Something is wrong if emission is forced and blocked at the same time.
		Q_ASSERT(!forceEmit);
		return;
	}

	// There is nothing to do if there are no pending constant pool entries.
	if (m_constPoolNumPendingItems == 0)  {
		// Calculate the offset of the next check.
		m_constPoolNextBufferCheck = pcOffset() + kCheckPoolInterval;
		return;
	}

	// We emit a constant pool when:
	//  * requested to do so by parameter force_emit (e.g. after each function).
	//  * the distance to the first instruction accessing the constant pool is
	//    kAvgDistToPool or more.
	//  * no jump is required and the distance to the first instruction accessing
	//    the constant pool is at least kMaxDistToPool / 2.
	Q_ASSERT(m_constPoolFirstUse >= 0);
	int dist = pcOffset() - m_constPoolFirstUse;
	if (!forceEmit && dist < kAvgDistToPool &&
		(requireJump || (dist < (kMaxDistToPool / 2)))) {
		return;
	}

	// Check that the code buffer is large enough before emitting the constant
	// pool (include the jump over the pool and the constant pool marker and
	// the gap to the relocation information).
#if !defined(QT_NO_DEBUG)
	int jumpInstr = requireJump ? kInstrSize : 0;
	int neededSpace = jumpInstr + kInstrSize +
			m_constPoolNumPendingItems * kInstrSize + kGap;
	Q_ASSERT(bufferSpace() > neededSpace);
#endif

	// Block recursive calls to CheckConstPool.
	BlockConstPoolScope blockConstPool(this);

	// Emit jump over constant pool if necessary.
	Label afterPool;
	if (requireJump)
		b(&afterPool);

	// Put down constant pool marker "Undefined instruction" as specified by
	// A5.6 (ARMv7) Instruction set encoding.
	iemit(kConstantPoolMarker | m_constPoolNumPendingItems);

	// Emit constant pool entries.
	for (int i = 0; i < m_constPoolNumPendingItems; i++) {
		const ConstPoolItem &item = m_constPoolPendingItems[i];

		Instr instr = instrAt(item.pc);
		// Instruction to patch must be 'ldr rd, [pc, #offset]' with offset == 0.
		Q_ASSERT(IsLdrPcImmediateOffset(instr) &&
				 GetLdrRegisterImmediateOffset(instr) == 0);

		int delta = (pc_ - item.pc) - kPcLoadDelta;
		// 0 is the smallest delta:
		//   ldr rd, [pc, #0]
		//   constant pool marker
		//   data
		Q_ASSERT(isUint12(delta));

		Instr patched = SetLdrRegisterImmediateOffset(instr, delta);
		putInstrAt(item.pc, patched);
		iemit(item.data);
	}

	m_constPoolNumPendingItems = 0;
	m_constPoolFirstUse = -1;

	if (afterPool.isLinked())
		bind(&afterPool);

	// Since a constant pool was just emitted, move the check offset forward by
	// the standard interval.
	m_constPoolNextBufferCheck = pcOffset() + kCheckPoolInterval;
}

}
