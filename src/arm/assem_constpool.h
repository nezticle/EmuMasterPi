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

#ifndef ARM_ASSEM_CONSTPOOL_H
#define ARM_ASSEM_CONSTPOOL_H

#include "assembler.h"

namespace Arm {

inline void Assembler::constPoolRecordLdrStrItem(u32 offset)
{
	Q_ASSERT(m_constPoolNumPendingItems < kMaxNumPendingRelocInfo);
	if (m_constPoolNumPendingItems == 0)
		m_constPoolFirstUse = pcOffset();
	ConstPoolItem item = { pc_, offset };
	m_constPoolPendingItems[m_constPoolNumPendingItems++] = item;
	// Make sure the constant pool is not emitted in place of the next
	// instruction for which we just recorded relocation info.
	constPoolBlockFor(1);
}

inline void Assembler::constPoolBlockFor(int instructions)
{
	int pc_limit = pcOffset() + instructions * kInstrSize;
	if (m_noConstPoolBefore < pc_limit) {
		// If there are some pending entries, the constant pool cannot be blocked
		// further than first_const_pool_use_ + kMaxDistToPool
		Q_ASSERT((m_constPoolNumPendingItems == 0) ||
				 (pc_limit < (m_constPoolFirstUse + kMaxDistToPool)));
		m_noConstPoolBefore = pc_limit;
	}

	if (m_constPoolNextBufferCheck < m_noConstPoolBefore) {
		m_constPoolNextBufferCheck = m_noConstPoolBefore;
	}
}

inline bool Assembler::constPoolIsBlocked() const
{
	return (m_constPoolBlockedNesting > 0) ||
			(pcOffset() < m_noConstPoolBefore);
}

// Prevent contant pool emission until EndBlockConstPool is called.
// Call to this function can be nested but must be followed by an equal
// number of call to EndBlockConstpool.
inline void Assembler::constPoolBeginBlock()
{
	if (m_constPoolBlockedNesting++ == 0) {
		// Prevent constant pool checks happening by setting the next check to
		// the biggest possible offset.
		m_constPoolNextBufferCheck = INT_MAX;
	}
}

// Resume constant pool emission. Need to be called as many time as
// StartBlockConstPool to have an effect.
inline void Assembler::constPoolEndBlock()
{
	if (--m_constPoolBlockedNesting == 0) {
		// Check the constant pool hasn't been blocked for too long.
		Q_ASSERT((m_constPoolNumPendingItems == 0) ||
				 (pcOffset() < (m_constPoolFirstUse + kMaxDistToPool)));
		// Two cases:
		//  * no_const_pool_before_ >= next_buffer_check_ and the emission is
		//    still blocked
		//  * no_const_pool_before_ < next_buffer_check_ and the next emit will
		//    trigger a check.
		m_constPoolNextBufferCheck = m_noConstPoolBefore;
	}
}

} // namespace Arm

#endif // ARM_ASSEM_CONSTPOOL_H
