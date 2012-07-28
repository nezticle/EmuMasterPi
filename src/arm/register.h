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

#ifndef ARM_REGISTER_H
#define ARM_REGISTER_H

#include "constants.h"

namespace Arm {

// CPU Registers.
//
// 1) We would prefer to use an enum, but enum values are assignment-
// compatible with int, which has caused code-generation bugs.
//
// 2) We would prefer to use a class instead of a struct but we don't like
// the register initialization to depend on the particular initialization
// order (which appears to be different on OS X, Linux, and Windows for the
// installed versions of C++ we tried). Using a struct permits C-style
// "initialization". Also, the Register objects cannot be const as this
// forces initialization stubs in MSVC, making us dependent on initialization
// order.
//
// 3) By not using an enum, we are possibly preventing the compiler from
// doing certain constant folds, which may significantly reduce the
// code generated for some assembly instructions (because they boil down
// to a few constants). If this is a problem, we could change the code
// such that we use an enum in optimized mode, and the struct in debug
// mode. This way we get the compile-time error checking in debug mode
// and best performance in optimized code.

// Core register
struct Register
{
	static const int kNumRegisters = 16;
	static const int kNumAllocatableRegisters = 8;
	static const int kSizeInBytes = 4;

	static int ToAllocationIndex(Register reg) {
		Q_ASSERT(reg.code() < kNumAllocatableRegisters);
		return reg.code();
	}

	static Register FromAllocationIndex(int index) {
		Q_ASSERT(index >= 0 && index < kNumAllocatableRegisters);
		return fromCode(index);
	}

	static const char* AllocationIndexToString(int index) {
		Q_ASSERT(index >= 0 && index < kNumAllocatableRegisters);
		const char* const names[] = {
			"r0",
			"r1",
			"r2",
			"r3",
			"r4",
			"r5",
			"r6",
			"r7",
		};
		return names[index];
	}

	static Register fromCode(int code) {
		Register r = { code };
		return r;
	}

	bool isValid() const { return 0 <= code_ && code_ < kNumRegisters; }
	bool is(Register reg) const { return code_ == reg.code_; }
	int code() const {
		Q_ASSERT(isValid());
		return code_;
	}
	int bit() const {
		Q_ASSERT(isValid());
		return 1 << code_;
	}

	void setCode(int code) {
		code_ = code;
		Q_ASSERT(isValid());
	}

	bool operator ==(const Register &r) const { return code_ == r.code_; }
	bool operator !=(const Register &r) const { return code_ != r.code_; }

	// Unfortunately we can't make this private in a struct.
	int code_;
};

static const Register no_reg = { -1 };

static const Register r0  = {  0 };
static const Register r1  = {  1 };
static const Register r2  = {  2 };
static const Register r3  = {  3 };
static const Register r4  = {  4 };
static const Register r5  = {  5 };
static const Register r6  = {  6 };
static const Register r7  = {  7 };
static const Register r8  = {  8 };
static const Register r9  = {  9 };
static const Register r10 = { 10 };
static const Register fp  = { 11 };
static const Register ip  = { 12 };
static const Register sp  = { 13 };
static const Register lr  = { 14 };
static const Register pc  = { 15 };

// Single word VFP register.
struct SwVfpRegister
{
	bool isValid() const { return 0 <= code_ && code_ < 32; }
	bool is(SwVfpRegister reg) const { return code_ == reg.code_; }
	int code() const {
		Q_ASSERT(isValid());
		return code_;
	}
	int bit() const {
		Q_ASSERT(isValid());
		return 1 << code_;
	}
	void splitCode(int* vm, int* m) const {
		Q_ASSERT(isValid());
		*m = code_ & 0x1;
		*vm = code_ >> 1;
	}

	int code_;
};


// Double word VFP register.
struct DwVfpRegister
{
	static const int kNumRegisters = 16;
	// A few double registers are reserved: one as a scratch register and one to
	// hold 0.0, that does not fit in the immediate field of vmov instructions.
	//  d14: 0.0
	//  d15: scratch register.
	static const int kNumReservedRegisters = 2;
	static const int kNumAllocatableRegisters = kNumRegisters -
			kNumReservedRegisters;

	inline static int ToAllocationIndex(DwVfpRegister reg);

	static DwVfpRegister FromAllocationIndex(int index) {
		Q_ASSERT(index >= 0 && index < kNumAllocatableRegisters);
		return fromCode(index);
	}

	static const char* AllocationIndexToString(int index) {
		Q_ASSERT(index >= 0 && index < kNumAllocatableRegisters);
		const char* const names[] = {
			"d0",
			"d1",
			"d2",
			"d3",
			"d4",
			"d5",
			"d6",
			"d7",
			"d8",
			"d9",
			"d10",
			"d11",
			"d12",
			"d13"
		};
		return names[index];
	}

	static DwVfpRegister fromCode(int code) {
		DwVfpRegister r = { code };
		return r;
	}

	// Supporting d0 to d15, can be later extended to d31.
	bool isValid() const { return 0 <= code_ && code_ < 16; }
	bool is(DwVfpRegister reg) const { return code_ == reg.code_; }

	SwVfpRegister firstSingle() const {
		SwVfpRegister reg;
		reg.code_ = code_ * 2;

		Q_ASSERT(reg.isValid());
		return reg;
	}

	SwVfpRegister secondSingle() const {
		SwVfpRegister reg;
		reg.code_ = (code_ * 2) + 1;

		Q_ASSERT(reg.isValid());
		return reg;
	}
	int code() const {
		Q_ASSERT(isValid());
		return code_;
	}
	int bit() const {
		Q_ASSERT(isValid());
		return 1 << code_;
	}
	void splitCode(int* vm, int* m) const {
		Q_ASSERT(isValid());
		*m = (code_ & 0x10) >> 4;
		*vm = code_ & 0x0F;
	}

	int code_;
};


typedef DwVfpRegister DoubleRegister;


// Support for the VFP registers s0 to s31 (d0 to d15).
// Note that "s(N):s(N+1)" is the same as "d(N/2)".
static const SwVfpRegister s0  = {  0 };
static const SwVfpRegister s1  = {  1 };
static const SwVfpRegister s2  = {  2 };
static const SwVfpRegister s3  = {  3 };
static const SwVfpRegister s4  = {  4 };
static const SwVfpRegister s5  = {  5 };
static const SwVfpRegister s6  = {  6 };
static const SwVfpRegister s7  = {  7 };
static const SwVfpRegister S8  = {  8 };
static const SwVfpRegister s9  = {  9 };
static const SwVfpRegister s10 = { 10 };
static const SwVfpRegister s11 = { 11 };
static const SwVfpRegister s12 = { 12 };
static const SwVfpRegister s13 = { 13 };
static const SwVfpRegister s14 = { 14 };
static const SwVfpRegister s15 = { 15 };
static const SwVfpRegister S16 = { 16 };
static const SwVfpRegister s17 = { 17 };
static const SwVfpRegister s18 = { 18 };
static const SwVfpRegister s19 = { 19 };
static const SwVfpRegister s20 = { 20 };
static const SwVfpRegister s21 = { 21 };
static const SwVfpRegister s22 = { 22 };
static const SwVfpRegister s23 = { 23 };
static const SwVfpRegister s24 = { 24 };
static const SwVfpRegister s25 = { 25 };
static const SwVfpRegister s26 = { 26 };
static const SwVfpRegister s27 = { 27 };
static const SwVfpRegister s28 = { 28 };
static const SwVfpRegister s29 = { 29 };
static const SwVfpRegister s30 = { 30 };
static const SwVfpRegister s31 = { 31 };

static const DwVfpRegister no_dreg = { -1 };
static const DwVfpRegister d0  = {  0 };
static const DwVfpRegister d1  = {  1 };
static const DwVfpRegister d2  = {  2 };
static const DwVfpRegister d3  = {  3 };
static const DwVfpRegister d4  = {  4 };
static const DwVfpRegister d5  = {  5 };
static const DwVfpRegister d6  = {  6 };
static const DwVfpRegister d7  = {  7 };
static const DwVfpRegister d8  = {  8 };
static const DwVfpRegister d9  = {  9 };
static const DwVfpRegister d10 = { 10 };
static const DwVfpRegister d11 = { 11 };
static const DwVfpRegister d12 = { 12 };
static const DwVfpRegister d13 = { 13 };
static const DwVfpRegister d14 = { 14 };
static const DwVfpRegister d15 = { 15 };

// Aliases for double registers.  Defined using #define instead of
// "static const DwVfpRegister&" because Clang complains otherwise when a
// compilation unit that includes this header doesn't use the variables.
#define kFirstCalleeSavedDoubleReg d8
#define kLastCalleeSavedDoubleReg d15
#define kDoubleRegZero d14
#define kScratchDoubleReg d15


// Coprocessor register
struct CRegister {
	bool isValid() const { return 0 <= code_ && code_ < 16; }
	bool is(CRegister creg) const { return code_ == creg.code_; }
	int code() const {
		Q_ASSERT(isValid());
		return code_;
	}
	int bit() const {
		Q_ASSERT(isValid());
		return 1 << code_;
	}

	// Unfortunately we can't make this private in a struct.
	int code_;
};


static const CRegister no_creg = { -1 };

static const CRegister cr0  = {  0 };
static const CRegister cr1  = {  1 };
static const CRegister cr2  = {  2 };
static const CRegister cr3  = {  3 };
static const CRegister cr4  = {  4 };
static const CRegister cr5  = {  5 };
static const CRegister cr6  = {  6 };
static const CRegister cr7  = {  7 };
static const CRegister cr8  = {  8 };
static const CRegister cr9  = {  9 };
static const CRegister cr10 = { 10 };
static const CRegister cr11 = { 11 };
static const CRegister cr12 = { 12 };
static const CRegister cr13 = { 13 };
static const CRegister cr14 = { 14 };
static const CRegister cr15 = { 15 };


// Coprocessor number
enum Coprocessor {
	p0  = 0,
	p1  = 1,
	p2  = 2,
	p3  = 3,
	p4  = 4,
	p5  = 5,
	p6  = 6,
	p7  = 7,
	p8  = 8,
	p9  = 9,
	p10 = 10,
	p11 = 11,
	p12 = 12,
	p13 = 13,
	p14 = 14,
	p15 = 15
};

} // namespace Arm

#endif // ARM_REGISTER_H
