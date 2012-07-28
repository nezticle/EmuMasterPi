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

#ifndef ARM_UTILS_H
#define ARM_UTILS_H

#include <base/emu.h>

namespace Arm {

// -----------------------------------------------------------------------------
// Utility functions

inline bool isIntn(int x, int n)  {
	return -(1 << (n-1)) <= x && x < (1 << (n-1));
}

inline bool isInt8(int x)  { return isIntn(x, 8); }
inline bool isInt16(int x)  { return isIntn(x, 16); }
inline bool isInt18(int x)  { return isIntn(x, 18); }
inline bool isInt24(int x)  { return isIntn(x, 24); }

inline bool isUintn(int x, int n) {
	return (x & -(1 << n)) == 0;
}

inline bool isUint2(int x)  { return isUintn(x, 2); }
inline bool isUint3(int x)  { return isUintn(x, 3); }
inline bool isUint4(int x)  { return isUintn(x, 4); }
inline bool isUint5(int x)  { return isUintn(x, 5); }
inline bool isUint6(int x)  { return isUintn(x, 6); }
inline bool isUint8(int x)  { return isUintn(x, 8); }
inline bool isUint10(int x)  { return isUintn(x, 10); }
inline bool isUint12(int x)  { return isUintn(x, 12); }
inline bool isUint16(int x)  { return isUintn(x, 16); }
inline bool isUint24(int x)  { return isUintn(x, 24); }
inline bool isUint26(int x)  { return isUintn(x, 26); }
inline bool isUint28(int x)  { return isUintn(x, 28); }

#define IS_POWER_OF_TWO(x) (((x) & ((x) - 1)) == 0)

// Returns true iff x is a power of 2 (or zero). Cannot be used with the
// maximally negative value of the type T (the -1 overflows).
template <typename T>
inline bool isPowerOf2(T x)
{
	return IS_POWER_OF_TWO(x);
}


// X must be a power of 2.  Returns the number of trailing zeros.
inline int whichPowerOf2(u32 x)
{
	Q_ASSERT(isPowerOf2(x));
	Q_ASSERT(x != 0);
	int bits = 0;
#if !defined(QT_NO_DEBUG)
	int originalX = x;
#endif
	if (x >= 0x10000) {
		bits += 16;
		x >>= 16;
	}
	if (x >= 0x100) {
		bits += 8;
		x >>= 8;
	}
	if (x >= 0x10) {
		bits += 4;
		x >>= 4;
	}
	switch (x) {
	default: UNREACHABLE();
	case 8: bits++;  // Fall through.
	case 4: bits++;  // Fall through.
	case 2: bits++;  // Fall through.
	case 1: break;
	}
	Q_ASSERT_EQ(1 << bits, originalX);
	return bits;
}

// Union used for fast testing of specific double values.
union DoubleRepresentation
{
	double  value;
	s64 bits;
	DoubleRepresentation(double x) { value = x; }
};

} // namespace Arm

#endif // ARM_UTILS_H
