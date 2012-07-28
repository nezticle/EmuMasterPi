/*
	Copyright 2009 the V8 project authors. All rights reserved.
	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are
	met:

		* Redistributions of source code must retain the above copyright
		  notice, this list of conditions and the following disclaimer.
		* Redistributions in binary form must reproduce the above
		  copyright notice, this list of conditions and the following
		  disclaimer in the documentation and/or other materials provided
		  with the distribution.
		* Neither the name of Google Inc. nor the names of its
		  contributors may be used to endorse or promote products derived
		  from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
	A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
	OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
	LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
	DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
	THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "instruction.h"

namespace Arm {

double Instruction::DoubleImmedVmov() const
{
	// Reconstruct a double from the immediate encoded in the vmov instruction.
	//
	//   instruction: [xxxxxxxx,xxxxabcd,xxxxxxxx,xxxxefgh]
	//   double: [aBbbbbbb,bbcdefgh,00000000,00000000,
	//            00000000,00000000,00000000,00000000]
	//
	// where B = ~b. Only the high 16 bits are affected.
	u64 high16;
	high16  = (Bits(17, 16) << 4) | Bits(3, 0);   // xxxxxxxx,xxcdefgh.
	high16 |= (0xff * Bit(18)) << 6;              // xxbbbbbb,bbxxxxxx.
	high16 |= (Bit(18) ^ 1) << 14;                // xBxxxxxx,xxxxxxxx.
	high16 |= Bit(19) << 15;                      // axxxxxxx,xxxxxxxx.

	u64 imm = high16 << 48;
	double d;
	memcpy(&d, &imm, 8);
	return d;
}

} // namespace Arm
