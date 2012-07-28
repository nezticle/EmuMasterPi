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

#ifndef ARM_ASSEM_RAW_H
#define ARM_ASSEM_RAW_H

#include "assembler.h"

namespace Arm {

inline void Assembler::db(u8 data)
{
	// No relocation info should be pending while using db. db is used
	// to write pure data with no pointers and the constant pool should
	// be emitted before using db.
	Q_ASSERT(m_constPoolNumPendingItems == 0);
	checkBuffer();
	*pc_ = data;
	pc_ += sizeof(u8);
}

inline void Assembler::dd(u32 data)
{
	// No relocation info should be pending while using dd. dd is used
	// to write pure data with no pointers and the constant pool should
	// be emitted before using dd.
	Q_ASSERT(m_constPoolNumPendingItems == 0);
	checkBuffer();
	*reinterpret_cast<u32*>(pc_) = data;
	pc_ += sizeof(u32);
}

inline void Assembler::checkBuffer()
{
	Q_ASSERT(bufferSpace() > kGap);
	if (pcOffset() >= m_constPoolNextBufferCheck)
		constPoolCheck(false, true);
}

inline int Assembler::bufferSpace()
{
	return buffer_size_ - pcOffset();
}

inline void Assembler::iemit(Instr x)
{
	checkBuffer();
	*reinterpret_cast<Instr*>(pc_) = x;
	pc_ += kInstrSize;
}

inline void Assembler::flush()
{
	// Emit constant pool if necessary.
	constPoolCheck(true, false);
	Q_ASSERT(m_constPoolNumPendingItems == 0);
}

} // namespace Arm

#endif // ARM_ASSEM_RAW_H
