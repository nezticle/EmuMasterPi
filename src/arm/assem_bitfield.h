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

#ifndef ARM_ASSEM_BITFIELD_H
#define ARM_ASSEM_BITFIELD_H

#include "assembler.h"

namespace Arm {

// Bitfield manipulation instructions.

// Unsigned bit field extract.
// Extracts #width adjacent bits from position #lsb in a register, and
// writes them to the low bits of a destination register.
//   ubfx dst, src, #lsb, #width
#if defined(CAN_USE_ARMV7_INSTRUCTIONS)
inline void Assembler::ubfx(Register dst,
							Register src,
							int lsb,
							int width,
							Condition cond)
{
	// v7 and above.
	Q_ASSERT(!dst.is(pc) && !src.is(pc));
	Q_ASSERT((lsb >= 0) && (lsb <= 31));
	Q_ASSERT((width >= 1) && (width <= (32 - lsb)));
	iemit(cond | 0xf*B23 | B22 | B21 | (width - 1)*B16 | dst.code()*B12 |
		  lsb*B7 | B6 | B4 | src.code());
}
#endif


// Signed bit field extract.
// Extracts #width adjacent bits from position #lsb in a register, and
// writes them to the low bits of a destination register. The extracted
// value is sign extended to fill the destination register.
//   sbfx dst, src, #lsb, #width
#if defined(CAN_USE_ARMV7_INSTRUCTIONS)
inline void Assembler::sbfx(Register dst,
							Register src,
							int lsb,
							int width,
							Condition cond)
{
	// v7 and above.
	Q_ASSERT(!dst.is(pc) && !src.is(pc));
	Q_ASSERT((lsb >= 0) && (lsb <= 31));
	Q_ASSERT((width >= 1) && (width <= (32 - lsb)));
	iemit(cond | 0xf*B23 | B21 | (width - 1)*B16 | dst.code()*B12 |
		  lsb*B7 | B6 | B4 | src.code());
}
#endif


// Bit field clear.
// Sets #width adjacent bits at position #lsb in the destination register
// to zero, preserving the value of the other bits.
//   bfc dst, #lsb, #width
#if defined(CAN_USE_ARMV7_INSTRUCTIONS)
inline void Assembler::bfc(Register dst, int lsb, int width, Condition cond)
{
	// v7 and above.
	Q_ASSERT(!dst.is(pc));
	Q_ASSERT((lsb >= 0) && (lsb <= 31));
	Q_ASSERT((width >= 1) && (width <= (32 - lsb)));
	int msb = lsb + width - 1;
	iemit(cond | 0x1f*B22 | msb*B16 | dst.code()*B12 | lsb*B7 | B4 | 0xf);
}
#endif

// Bit field insert.
// Inserts #width adjacent bits from the low bits of the source register
// into position #lsb of the destination register.
//   bfi dst, src, #lsb, #width
#if defined(CAN_USE_ARMV7_INSTRUCTIONS)
inline void Assembler::bfi(Register dst,
						   Register src,
						   int lsb,
						   int width,
						   Condition cond)
{
	// v7 and above.
	Q_ASSERT(!dst.is(pc) && !src.is(pc));
	Q_ASSERT((lsb >= 0) && (lsb <= 31));
	Q_ASSERT((width >= 1) && (width <= (32 - lsb)));
	int msb = lsb + width - 1;
	iemit(cond | 0x1f*B22 | msb*B16 | dst.code()*B12 | lsb*B7 | B4 |
		  src.code());
}
#endif

} // namespace Arm

#endif // ARM_ASSEM_BITFIELD_H
