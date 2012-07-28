/*
	Copyright 2012 the V8 project authors. All rights reserved.
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

#ifndef ARM_FRAMES_H
#define ARM_FRAMES_H

#include "constants.h"

namespace Arm {

typedef u32 RegList;

// The ARM ABI does not specify the usage of register r9, which may be reserved
// as the static base or thread register on some platforms, in which case we
// leave it alone. Adjust ARM_R9_AVAILABLE accordingly:
#define ARM_R9_AVAILABLE


// Register list in load/store instructions
// Note that the bit values must match those used in actual instruction encoding
static const int kNumRegs = 16;

// Callee-saved registers
static const RegList kCalleeSaved =
		1 <<  4 |  //  r4 v1
		1 <<  5 |  //  r5 v2
		1 <<  6 |  //  r6 v3
		1 <<  7 |  //  r7 v4
		1 <<  8 |  //  r8 v5
#if defined(ARM_R9_AVAILABLE)
		1 <<  9 |  //  r9 v6
#endif
		1 << 10 |  // r10 v7
		1 << 11;   // r11 v8

// The call code will take care of lr, fp, etc.
static const RegList kCallerSaved =
		1 << 12 |
#if defined(ARM_R9_AVAILABLE)
		1 <<  9 |  // r9
#endif
		1 <<  0 |  // r0
		1 <<  1 |  // r1
		1 <<  2 |  // r2
		1 <<  3;   // r3

#if defined(ARM_R9_AVAILABLE)
static const int kNumCalleeSaved = 8;
static const int kNumCallerSaved = 6;
#else
static const int kNumCalleeSaved = 7;
static const int kNumCalleeSaved = 5;
#endif

// Double registers d8 to d15 are callee-saved.
static const int kNumDoubleCalleeSaved = 8;

} // namespace Arm

#endif  // ARM_FRAMES_H
