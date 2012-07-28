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

#ifndef ARM_CPU_H
#define ARM_CPU_H

#include "constants.h"
#include <syscall.h>

namespace Arm {

class Cpu BASE_EMBEDDED
{
public:
	// Flush instruction cache.
	static void flushICache(void* start, u32 size);

	// Try to activate a system level debugger.
	static void debugBreak();
private:
	Q_DISABLE_COPY(Cpu)
};

inline void Cpu::flushICache(void* start, u32 size)
{
	// Nothing to do flushing no instructions.
	if (size == 0)
		return;

	// Ideally, we would call
	//   syscall(__ARM_NR_cacheflush, start,
	//           reinterpret_cast<intptr_t>(start) + size, 0);
	// however, syscall(int, ...) is not supported on all platforms, especially
	// not when using EABI, so we call the __ARM_NR_cacheflush syscall directly.

	register u32 beg asm("a1") = reinterpret_cast<u32>(start);
	register u32 end asm("a2") = reinterpret_cast<u32>(start) + size;
	register u32 flg asm("a3") = 0;
#if defined (__arm__) && !defined(__thumb__)
	// __arm__ may be defined in thumb mode.
	register u32 scno asm("r7") = __ARM_NR_cacheflush;
	asm volatile(
				"svc 0x0"
				: "=r" (beg)
				: "0" (beg), "r" (end), "r" (flg), "r" (scno));
#else
	// r7 is reserved by the EABI in thumb mode.
	asm volatile(
			"@   Enter ARM Mode  \n\t"
				"adr r3, 1f      \n\t"
				"bx  r3          \n\t"
				".ALIGN 4        \n\t"
				".ARM            \n"
			"1:  push {r7}       \n\t"
				"mov r7, %4      \n\t"
				"svc 0x0         \n\t"
				"pop {r7}        \n\t"
			"@   Enter THUMB Mode\n\t"
				"adr r3, 2f+1    \n\t"
				"bx  r3          \n\t"
				".THUMB          \n"
			"2:                  \n\t"
				: "=r" (beg)
				: "0" (beg), "r" (end), "r" (flg), "r" (__ARM_NR_cacheflush)
				: "r3");
#endif
}


inline void Cpu::debugBreak()
{
#if !defined (__arm__) || !defined(CAN_USE_ARMV5_INSTRUCTIONS)
	UNIMPLEMENTED();  // when building ARM emulator target
#else
	asm volatile("bkpt 0");
#endif
}

} // namespace Arm

#endif // ARM_CPU_H
