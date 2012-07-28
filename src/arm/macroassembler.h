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

#ifndef ARM_MACROASSEMBLER_H
#define ARM_MACROASSEMBLER_H

#include "assem_all.h"
#include "cpu.h"

namespace Arm {

class MacroAssembler : public Assembler
{
public:
	MacroAssembler(void* buffer, int size);

	void lowLevelDebug(const char *s,
					   Register ra = r1,
					   Register rb = r2,
					   Register rc = r3);

	// Jump, Call, and Ret pseudo instructions implementing inter-working.
	void Jump(Register target, Condition cond = al);
	void Jump(intptr_t target, Condition cond = al);
	static int CallSize(Register target, Condition cond = al);
	void Call(Register target, Condition cond = al);
	void Ret(Condition cond = al);

	// Emit code to discard a non-negative number of pointer-sized elements
	// from the stack, clobbering only the sp register.
	void Drop(int count, Condition cond = al);

	void Ret(int drop, Condition cond = al);

	// Swap two registers.  If the scratch register is omitted then a slightly
	// less efficient form using xor instead of mov is emitted.
	void Swap(Register reg1,
			  Register reg2,
			  Register scratch = no_reg,
			  Condition cond = al);

	void And(Register dst, Register src1, const Operand& src2,
			 Condition cond = al);
	void Ubfx(Register dst, Register src, int lsb, int width,
			  Condition cond = al);
	void Sbfx(Register dst, Register src, int lsb, int width,
			  Condition cond = al);
	// The scratch register is not used for ARMv7.
	// scratch can be the same register as src (in which case it is trashed), but
	// not the same as dst.
	void Bfi(Register dst,
			 Register src,
			 Register scratch,
			 int lsb,
			 int width,
			 Condition cond = al);
	void Bfc(Register dst, int lsb, int width, Condition cond = al);
	void Usat(Register dst, int satpos, const Operand& src,
			  Condition cond = al);

	void Call(Label* target);

	// Register move. May do nothing if the registers are identical.
	void Move(Register dst, Register src, Condition cond = al);
	void Move(DoubleRegister dst, DoubleRegister src);

	// Push two registers.  Pushes leftmost register first (to highest address).
	void Push(Register src1, Register src2, Condition cond = al);

	// Push three registers.  Pushes leftmost register first (to highest address).
	void Push(Register src1, Register src2, Register src3, Condition cond = al);

	// Push four registers.  Pushes leftmost register first (to highest address).
	void Push(Register src1,
			  Register src2,
			  Register src3,
			  Register src4,
			  Condition cond = al);

	// Pop two registers. Pops rightmost register first (from lower address).
	void Pop(Register src1, Register src2, Condition cond = al);

	// Pop three registers.  Pops rightmost register first (from lower address).
	void Pop(Register src1, Register src2, Register src3, Condition cond = al);

	// Pop four registers.  Pops rightmost register first (from lower address).
	void Pop(Register src1,
			 Register src2,
			 Register src3,
			 Register src4,
			 Condition cond = al);

	// Load two consecutive registers with two consecutive memory locations.
	void Ldrd(Register dst1,
			  Register dst2,
			  const MemOperand& src,
			  Condition cond = al);

	// Store two consecutive registers to two consecutive memory locations.
	void Strd(Register src1,
			  Register src2,
			  const MemOperand& dst,
			  Condition cond = al);

#if defined(CAN_USE_VFP_INSTRUCTIONS)
	// Clear specified FPSCR bits.
	void ClearFPSCRBits(const u32 bits_to_clear,
						const Register scratch,
						const Condition cond = al);

	// Compare double values and move the result to the normal condition flags.
	void VFPCompareAndSetFlags(const DwVfpRegister src1,
							   const DwVfpRegister src2,
							   const Condition cond = al);
	void VFPCompareAndSetFlags(const DwVfpRegister src1,
							   const double src2,
							   const Condition cond = al);

	// Compare double values and then load the fpscr flags to a register.
	void VFPCompareAndLoadFlags(const DwVfpRegister src1,
								const DwVfpRegister src2,
								const Register fpscr_flags,
								const Condition cond = al);
	void VFPCompareAndLoadFlags(const DwVfpRegister src1,
								const double src2,
								const Register fpscr_flags,
								const Condition cond = al);

	void Vmov(const DwVfpRegister dst,
			  const double imm,
			  const Condition cond = al);
#endif // CAN_USE_VFP_INSTRUCTIONS
};


inline void MacroAssembler::Jump(Register target, Condition cond)
{
#if USE_BX
	bx(target, cond);
#else
	mov(pc, Operand(target), LeaveCC, cond);
#endif
}

inline void MacroAssembler::Jump(intptr_t target, Condition cond)
{
#if USE_BX
	mov(ip, Operand(target));
	bx(ip, cond);
#else
	mov(pc, Operand(target), LeaveCC, cond);
#endif
}

inline int MacroAssembler::CallSize(Register target, Condition cond)
{
	Q_UNUSED(target)
	Q_UNUSED(cond)
#if USE_BLX
	return kInstrSize;
#else
	return 2 * kInstrSize;
#endif
}

inline void MacroAssembler::Call(Register target, Condition cond)
{
	// Block constant pool for the call instruction sequence.
	BlockConstPoolScope blockConstPool(this);
	Label start;
	bind(&start);
#if USE_BLX
	blx(target, cond);
#else
	// set lr for return at current pc + 8
	mov(lr, Operand(pc), LeaveCC, cond);
	mov(pc, Operand(target), LeaveCC, cond);
#endif
	Q_ASSERT_EQ(CallSize(target, cond), sizeOfCodeGeneratedSince(&start));
}

inline void MacroAssembler::Ret(Condition cond)
{
#if USE_BX
	bx(lr, cond);
#else
	mov(pc, Operand(lr), LeaveCC, cond);
#endif
}

inline void MacroAssembler::Drop(int count, Condition cond)
{
	if (count > 0)
		add(sp, sp, Operand(count * kPointerSize), LeaveCC, cond);
}


inline void MacroAssembler::Ret(int drop, Condition cond)
{
	Drop(drop, cond);
	Ret(cond);
}

inline void MacroAssembler::Swap(Register reg1,
								 Register reg2,
								 Register scratch,
								 Condition cond)
{
	if (scratch.is(no_reg)) {
		eor(reg1, reg1, Operand(reg2), LeaveCC, cond);
		eor(reg2, reg2, Operand(reg1), LeaveCC, cond);
		eor(reg1, reg1, Operand(reg2), LeaveCC, cond);
	} else {
		mov(scratch, reg1, LeaveCC, cond);
		mov(reg1, reg2, LeaveCC, cond);
		mov(reg2, scratch, LeaveCC, cond);
	}
}


inline void MacroAssembler::Call(Label* target)
{
	bl(target);
}

inline void MacroAssembler::Move(Register dst, Register src, Condition cond)
{
	if (!dst.is(src))
		mov(dst, src, LeaveCC, cond);
}

#if defined(CAN_USE_VFP_INSTRUCTIONS)
inline void MacroAssembler::Move(DoubleRegister dst, DoubleRegister src)
{
	if (!dst.is(src))
		vmov(dst, src);
}
#endif

inline void MacroAssembler::And(Register dst, Register src1, const Operand& src2,
								Condition cond)
{
	if (!src2.isReg() &&
		src2.immediate() == 0) {
		mov(dst, Operand(0), LeaveCC, cond);
#if defined(CAN_USE_ARMV7_INSTRUCTIONS)
	} else if (!src2.isSingleInstruction() &&
			   isPowerOf2(src2.immediate() + 1)) {
		ubfx(dst, src1, 0,
			 whichPowerOf2(static_cast<u32>(src2.immediate()) + 1), cond);
#endif
	} else {
		and_(dst, src1, src2, LeaveCC, cond);
	}
}

inline void MacroAssembler::Ubfx(Register dst, Register src1, int lsb, int width,
								 Condition cond)
{
	Q_ASSERT(lsb < 32);
#if !defined(CAN_USE_ARMV7_INSTRUCTIONS)
	int mask = (1 << (width + lsb)) - 1 - ((1 << lsb) - 1);
	and_(dst, src1, Operand(mask), LeaveCC, cond);
	if (lsb != 0) {
		mov(dst, Operand(dst, LSR, lsb), LeaveCC, cond);
	}
#else
	ubfx(dst, src1, lsb, width, cond);
#endif
}

inline void MacroAssembler::Sbfx(Register dst, Register src1, int lsb, int width,
								 Condition cond)
{
	Q_ASSERT(lsb < 32);
#if !defined(CAN_USE_ARMV7_INSTRUCTIONS)
	int mask = (1 << (width + lsb)) - 1 - ((1 << lsb) - 1);
	and_(dst, src1, Operand(mask), LeaveCC, cond);
	int shift_up = 32 - lsb - width;
	int shift_down = lsb + shift_up;
	if (shift_up != 0) {
		mov(dst, Operand(dst, LSL, shift_up), LeaveCC, cond);
	}
	if (shift_down != 0) {
		mov(dst, Operand(dst, ASR, shift_down), LeaveCC, cond);
	}
#else
	sbfx(dst, src1, lsb, width, cond);
#endif
}


inline void MacroAssembler::Bfi(Register dst,
								Register src,
								Register scratch,
								int lsb,
								int width,
								Condition cond)
{
	Q_ASSERT(0 <= lsb && lsb < 32);
	Q_ASSERT(0 <= width && width < 32);
	Q_ASSERT(lsb + width < 32);
	Q_ASSERT(!scratch.is(dst));
	if (width == 0)
		return;
#if !defined(CAN_USE_ARMV7_INSTRUCTIONS)
	int mask = (1 << (width + lsb)) - 1 - ((1 << lsb) - 1);
	bic(dst, dst, Operand(mask));
	and_(scratch, src, Operand((1 << width) - 1));
	mov(scratch, Operand(scratch, LSL, lsb));
	orr(dst, dst, scratch);
#else
	Q_UNUSED(scratch)
	bfi(dst, src, lsb, width, cond);
#endif
}


inline void MacroAssembler::Bfc(Register dst, int lsb, int width, Condition cond)
{
	Q_ASSERT(lsb < 32);
#if !defined(CAN_USE_ARMV7_INSTRUCTIONS)
	int mask = (1 << (width + lsb)) - 1 - ((1 << lsb) - 1);
	bic(dst, dst, Operand(mask));
#else
	bfc(dst, lsb, width, cond);
#endif
}


inline void MacroAssembler::Usat(Register dst, int satpos, const Operand& src,
								 Condition cond)
{
#if !defined(CAN_USE_ARMV7_INSTRUCTIONS)
	Q_ASSERT(!dst.is(pc) && !src.rm().is(pc));
	Q_ASSERT((satpos >= 0) && (satpos <= 31));

	// These Q_ASSERTs are required to ensure compatibility with the ARMv7
	// implementation.
    Q_ASSERT((src.shiftOp() == ASR) || (src.shiftOp() == LSL));
	Q_ASSERT(src.rs().is(no_reg));

	Label done;
	int satval = (1 << satpos) - 1;

	if (cond != al) {
		b(NegateCondition(cond), &done);  // Skip saturate if !condition.
	}
    if (!(src.isReg() && dst.is(src.rm()))) {
		mov(dst, src);
	}
	tst(dst, Operand(~satval));
	b(eq, &done);
	mov(dst, Operand(0), LeaveCC, mi);  // 0 if negative.
	mov(dst, Operand(satval), LeaveCC, pl);  // satval if positive.
	bind(&done);
#else
	usat(dst, satpos, src, cond);
#endif
}

// Push two registers.  Pushes leftmost register first (to highest address).
inline void MacroAssembler::Push(Register src1, Register src2, Condition cond)
{
	Q_ASSERT(!src1.is(src2));
	if (src1.code() > src2.code()) {
		stm(db_w, sp, src1.bit() | src2.bit(), cond);
	} else {
		str(src1, MemOperand(sp, 4, NegPreIndex), cond);
		str(src2, MemOperand(sp, 4, NegPreIndex), cond);
	}
}

// Push three registers.  Pushes leftmost register first (to highest address).
inline void MacroAssembler::Push(Register src1, Register src2, Register src3, Condition cond)
{
	Q_ASSERT(!src1.is(src2));
	Q_ASSERT(!src2.is(src3));
	Q_ASSERT(!src1.is(src3));
	if (src1.code() > src2.code()) {
		if (src2.code() > src3.code()) {
			stm(db_w, sp, src1.bit() | src2.bit() | src3.bit(), cond);
		} else {
			stm(db_w, sp, src1.bit() | src2.bit(), cond);
			str(src3, MemOperand(sp, 4, NegPreIndex), cond);
		}
	} else {
		str(src1, MemOperand(sp, 4, NegPreIndex), cond);
		Push(src2, src3, cond);
	}
}

// Push four registers.  Pushes leftmost register first (to highest address).
inline void MacroAssembler::Push(Register src1,
								 Register src2,
								 Register src3,
								 Register src4,
								 Condition cond)
{
	Q_ASSERT(!src1.is(src2));
	Q_ASSERT(!src2.is(src3));
	Q_ASSERT(!src1.is(src3));
	Q_ASSERT(!src1.is(src4));
	Q_ASSERT(!src2.is(src4));
	Q_ASSERT(!src3.is(src4));
	if (src1.code() > src2.code()) {
		if (src2.code() > src3.code()) {
			if (src3.code() > src4.code()) {
				stm(db_w,
					sp,
					src1.bit() | src2.bit() | src3.bit() | src4.bit(),
					cond);
			} else {
				stm(db_w, sp, src1.bit() | src2.bit() | src3.bit(), cond);
				str(src4, MemOperand(sp, 4, NegPreIndex), cond);
			}
		} else {
			stm(db_w, sp, src1.bit() | src2.bit(), cond);
			Push(src3, src4, cond);
		}
	} else {
		str(src1, MemOperand(sp, 4, NegPreIndex), cond);
		Push(src2, src3, src4, cond);
	}
}

// Pop two registers. Pops rightmost register first (from lower address).
inline void MacroAssembler::Pop(Register src1, Register src2, Condition cond)
{
	Q_ASSERT(!src1.is(src2));
	if (src1.code() > src2.code()) {
		ldm(ia_w, sp, src1.bit() | src2.bit(), cond);
	} else {
		ldr(src2, MemOperand(sp, 4, PostIndex), cond);
		ldr(src1, MemOperand(sp, 4, PostIndex), cond);
	}
}

// Pop three registers.  Pops rightmost register first (from lower address).
inline void MacroAssembler::Pop(Register src1, Register src2, Register src3, Condition cond) {
	Q_ASSERT(!src1.is(src2));
	Q_ASSERT(!src2.is(src3));
	Q_ASSERT(!src1.is(src3));
	if (src1.code() > src2.code()) {
		if (src2.code() > src3.code()) {
			ldm(ia_w, sp, src1.bit() | src2.bit() | src3.bit(), cond);
		} else {
			ldr(src3, MemOperand(sp, 4, PostIndex), cond);
			ldm(ia_w, sp, src1.bit() | src2.bit(), cond);
		}
	} else {
		Pop(src2, src3, cond);
		str(src1, MemOperand(sp, 4, PostIndex), cond);
	}
}

// Pop four registers.  Pops rightmost register first (from lower address).
inline void MacroAssembler::Pop(Register src1,
								Register src2,
								Register src3,
								Register src4,
								Condition cond) {
	Q_ASSERT(!src1.is(src2));
	Q_ASSERT(!src2.is(src3));
	Q_ASSERT(!src1.is(src3));
	Q_ASSERT(!src1.is(src4));
	Q_ASSERT(!src2.is(src4));
	Q_ASSERT(!src3.is(src4));
	if (src1.code() > src2.code()) {
		if (src2.code() > src3.code()) {
			if (src3.code() > src4.code()) {
				ldm(ia_w,
					sp,
					src1.bit() | src2.bit() | src3.bit() | src4.bit(),
					cond);
			} else {
				ldr(src4, MemOperand(sp, 4, PostIndex), cond);
				ldm(ia_w, sp, src1.bit() | src2.bit() | src3.bit(), cond);
			}
		} else {
			Pop(src3, src4, cond);
			ldm(ia_w, sp, src1.bit() | src2.bit(), cond);
		}
	} else {
		Pop(src2, src3, src4, cond);
		ldr(src1, MemOperand(sp, 4, PostIndex), cond);
	}
}

inline void MacroAssembler::Ldrd(Register dst1, Register dst2,
								 const MemOperand& src, Condition cond)
{
	Q_ASSERT(src.rm().is(no_reg));
	Q_ASSERT(!dst1.is(lr));  // r14.
	Q_ASSERT_EQ(0, dst1.code() % 2);
	Q_ASSERT_EQ(dst1.code() + 1, dst2.code());

	// V8 does not use this addressing mode, so the fallback code
	// below doesn't support it yet.
	Q_ASSERT((src.am() != PreIndex) && (src.am() != NegPreIndex));

	// Generate two ldr instructions if ldrd is not available.
#if defined(CAN_USE_ARMV7_INSTRUCTIONS)
	ldrd(dst1, dst2, src, cond);
#else
	if ((src.am() == Offset) || (src.am() == NegOffset)) {
		MemOperand src2(src);
        src2.setOffset(src2.offset() + 4);
		if (dst1.is(src.rn())) {
			ldr(dst2, src2, cond);
			ldr(dst1, src, cond);
		} else {
			ldr(dst1, src, cond);
			ldr(dst2, src2, cond);
		}
	} else {  // PostIndex or NegPostIndex.
		Q_ASSERT((src.am() == PostIndex) || (src.am() == NegPostIndex));
		if (dst1.is(src.rn())) {
			ldr(dst2, MemOperand(src.rn(), 4, Offset), cond);
			ldr(dst1, src, cond);
		} else {
			MemOperand src2(src);
            src2.setOffset(src2.offset() - 4);
			ldr(dst1, MemOperand(src.rn(), 4, PostIndex), cond);
			ldr(dst2, src2, cond);
		}
	}
#endif
}


inline void MacroAssembler::Strd(Register src1, Register src2,
								 const MemOperand& dst, Condition cond)
{
	Q_ASSERT(dst.rm().is(no_reg));
	Q_ASSERT(!src1.is(lr));  // r14.
	Q_ASSERT_EQ(0, src1.code() % 2);
	Q_ASSERT_EQ(src1.code() + 1, src2.code());

	// V8 does not use this addressing mode, so the fallback code
	// below doesn't support it yet.
	Q_ASSERT((dst.am() != PreIndex) && (dst.am() != NegPreIndex));

	// Generate two str instructions if strd is not available.
#if defined(CAN_USE_ARMV7_INSTRUCTIONS)
	strd(src1, src2, dst, cond);
#else
	MemOperand dst2(dst);
	if ((dst.am() == Offset) || (dst.am() == NegOffset)) {
        dst2.setOffset(dst2.offset() + 4);
		str(src1, dst, cond);
		str(src2, dst2, cond);
	} else {  // PostIndex or NegPostIndex.
		Q_ASSERT((dst.am() == PostIndex) || (dst.am() == NegPostIndex));
        dst2.setOffset(dst2.offset() - 4);
		str(src1, MemOperand(dst.rn(), 4, PostIndex), cond);
		str(src2, dst2, cond);
	}
#endif
}

#if defined(CAN_USE_VFP_INSTRUCTIONS)
inline void MacroAssembler::ClearFPSCRBits(const u32 bits_to_clear,
										   const Register scratch,
										   const Condition cond)
{
	vmrs(scratch, cond);
	bic(scratch, scratch, Operand(bits_to_clear), LeaveCC, cond);
	vmsr(scratch, cond);
}


inline void MacroAssembler::VFPCompareAndSetFlags(const DwVfpRegister src1,
												  const DwVfpRegister src2,
												  const Condition cond)
{
	// Compare and move FPSCR flags to the normal condition flags.
	VFPCompareAndLoadFlags(src1, src2, pc, cond);
}

inline void MacroAssembler::VFPCompareAndSetFlags(const DwVfpRegister src1,
												  const double src2,
												  const Condition cond)
{
	// Compare and move FPSCR flags to the normal condition flags.
	VFPCompareAndLoadFlags(src1, src2, pc, cond);
}


inline void MacroAssembler::VFPCompareAndLoadFlags(const DwVfpRegister src1,
												   const DwVfpRegister src2,
												   const Register fpscr_flags,
												   const Condition cond)
{
	// Compare and load FPSCR.
	vcmp(src1, src2, cond);
	vmrs(fpscr_flags, cond);
}

inline void MacroAssembler::VFPCompareAndLoadFlags(const DwVfpRegister src1,
												   const double src2,
												   const Register fpscr_flags,
												   const Condition cond)
{
	// Compare and load FPSCR.
	vcmp(src1, src2, cond);
	vmrs(fpscr_flags, cond);
}

inline void MacroAssembler::Vmov(const DwVfpRegister dst,
								 const double imm,
								 const Condition cond)
{
	static const DoubleRepresentation minus_zero(-0.0);
	static const DoubleRepresentation zero(0.0);
	DoubleRepresentation value(imm);
	// Handle special values first.
	if (value.bits == zero.bits) {
		vmov(dst, kDoubleRegZero, cond);
	} else if (value.bits == minus_zero.bits) {
		vneg(dst, kDoubleRegZero, cond);
	} else {
		vmov(dst, imm, cond);
	}
}

#endif // defined(CAN_USE_VFP_INSTRUCTIONS)

} // namespace Arm

#endif // ARM_MACROASSEMBLER_H
