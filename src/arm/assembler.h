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

#ifndef ARM_ASSEMBLER_H
#define ARM_ASSEMBLER_H

#include "operand.h"
#include "memoperand.h"
#include "label.h"
#include "frames.h"

namespace Arm {

class Assembler
{
public:
	// Create an assembler. Instructions and relocation information are emitted
	// into a buffer, with the instructions starting from the beginning and the
	// relocation information starting from the end of the buffer. See CodeDesc
	// for a detailed comment on the layout (globals.h).
	//
	// The assembler uses the provided buffer for code generation and assumes
	// its size to be buffer_size. If the buffer is too small, a fatal error
	// occurs. No deallocation of the buffer is done upon destruction of the assembler.
	Assembler(void* buffer, int buffer_size);
	~Assembler();

	// flush() emits any pending (non-emitted) code
	void flush();

	// Takes a branch opcode (cc) and a label (L) and generates
	// either a backward branch or a forward branch and links it
	// to the label fixup chain. Usage:
	//
	// Label L;    // unbound label
	// j(cc, &L);  // forward branch to unbound label
	// bind(&L);   // bind label to the current pc
	// j(cc, &L);  // backward branch to bound label
	// bind(&L);   // illegal: a label may be bound only once
	//
	// Note: The same Label can be used for forward and backward branches
	// but it may be bound only once.
	void bind(Label* L);  // binds an unbound label L to the current code position

	// Returns the branch offset to the given label from the current code position
	// Links the label to the current position if it is still unbound
	// Manages the jump elimination optimization if the second parameter is true.
	int branchOffset(Label* L, bool jumpEliminationAllowed);

	// Here we are patching the address in the constant pool, not the actual call
	// instruction.  The address in the constant pool is the same size as a
	// pointer.
	static const int kCallTargetSize = kPointerSize;
	static const int kExternalTargetSize = kPointerSize;

	// Size of an instruction.
	static const int kInstrSize = sizeof(Instr);

	// Distance between the instruction referring to the address of the call
	// target and the return address.
#ifdef USE_BLX
	// Call sequence is:
	//  ldr  ip, [pc, #...] @ call address
	//  blx  ip
	//                      @ return address
	static const int kCallTargetAddressOffset = 2 * kInstrSize;
#else
	// Call sequence is:
	//  mov  lr, pc
	//  ldr  pc, [pc, #...] @ call address
	//                      @ return address
	static const int kCallTargetAddressOffset = kInstrSize;
#endif

	// Distance between start of patched return sequence and the emitted address
	// to jump to.
#ifdef USE_BLX
	// Patched return sequence is:
	//  ldr  ip, [pc, #0]   @ emited address and start
	//  blx  ip
	static const int kPatchReturnSequenceAddressOffset =  0 * kInstrSize;
#else
	// Patched return sequence is:
	//  mov  lr, pc         @ start of sequence
	//  ldr  pc, [pc, #-4]  @ emited address
	static const int kPatchReturnSequenceAddressOffset =  kInstrSize;
#endif

	// Distance between start of patched debug break slot and the emitted address
	// to jump to.
#ifdef USE_BLX
	// Patched debug break slot code is:
	//  ldr  ip, [pc, #0]   @ emited address and start
	//  blx  ip
	static const int kPatchDebugBreakSlotAddressOffset =  0 * kInstrSize;
#else
	// Patched debug break slot code is:
	//  mov  lr, pc         @ start of sequence
	//  ldr  pc, [pc, #-4]  @ emited address
	static const int kPatchDebugBreakSlotAddressOffset =  kInstrSize;
#endif

	// Difference between address of current opcode and value read from pc
	// register.
	static const int kPcLoadDelta = 8;

	// ---------------------------------------------------------------------------
	// Code generation

	// Insert the smallest number of nop instructions
	// possible to align the pc offset to a multiple
	// of m. m must be a power of 2 (>= 4).
	void align(int m);
	// Aligns code to something that's optimal for a jump target for the platform.
	void codeTargetAlign();

	// Branch instructions
	void b(int branchOffset, Condition cond = al);
	void bl(int branchOffset, Condition cond = al);
	void blx(int branchOffset);  // v5 and above
	void blx(Register target, Condition cond = al);  // v5 and above
	void bx(Register target, Condition cond = al);  // v5 and above, plus v4t

	// Convenience branch instructions using labels
	void b(Label* L, Condition cond = al)  {
		b(branchOffset(L, cond == al), cond);
	}
	void b(Condition cond, Label* L)  { b(branchOffset(L, cond == al), cond); }
	void bl(Label* L, Condition cond = al)  { bl(branchOffset(L, false), cond); }
	void bl(Condition cond, Label* L)  { bl(branchOffset(L, false), cond); }
	void blx(Label* L)  { blx(branchOffset(L, false)); }  // v5 and above

	// Data-processing instructions

	void and_(Register dst, Register src1, const Operand& src2,
			  SBit s = LeaveCC, Condition cond = al);

	void eor(Register dst, Register src1, const Operand& src2,
			 SBit s = LeaveCC, Condition cond = al);

	void sub(Register dst, Register src1, const Operand& src2,
			 SBit s = LeaveCC, Condition cond = al);
	void sub(Register dst, Register src1, Register src2,
			 SBit s = LeaveCC, Condition cond = al) {
		sub(dst, src1, Operand(src2), s, cond);
	}

	void rsb(Register dst, Register src1, const Operand& src2,
			 SBit s = LeaveCC, Condition cond = al);

	void add(Register dst, Register src1, const Operand& src2,
			 SBit s = LeaveCC, Condition cond = al);
	void add(Register dst, Register src1, Register src2,
			 SBit s = LeaveCC, Condition cond = al) {
		add(dst, src1, Operand(src2), s, cond);
	}

	void adc(Register dst, Register src1, const Operand& src2,
			 SBit s = LeaveCC, Condition cond = al);

	void sbc(Register dst, Register src1, const Operand& src2,
			 SBit s = LeaveCC, Condition cond = al);

	void rsc(Register dst, Register src1, const Operand& src2,
			 SBit s = LeaveCC, Condition cond = al);

	void tst(Register src1, const Operand& src2, Condition cond = al);
	void tst(Register src1, Register src2, Condition cond = al) {
		tst(src1, Operand(src2), cond);
	}

	void teq(Register src1, const Operand& src2, Condition cond = al);

	void cmp(Register src1, const Operand& src2, Condition cond = al);
	void cmp(Register src1, Register src2, Condition cond = al) {
		cmp(src1, Operand(src2), cond);
	}
	void cmp_raw_immediate(Register src1, int raw_immediate, Condition cond = al);

	void cmn(Register src1, const Operand& src2, Condition cond = al);

	void orr(Register dst, Register src1, const Operand& src2,
			 SBit s = LeaveCC, Condition cond = al);
	void orr(Register dst, Register src1, Register src2,
			 SBit s = LeaveCC, Condition cond = al) {
		orr(dst, src1, Operand(src2), s, cond);
	}

	void mov(Register dst, const Operand& src,
			 SBit s = LeaveCC, Condition cond = al);
	void mov(Register dst, Register src, SBit s = LeaveCC, Condition cond = al) {
		mov(dst, Operand(src), s, cond);
	}

	// ARMv7 instructions for loading a 32 bit immediate in two instructions.
	// This may actually emit a different mov instruction, but on an ARMv7 it
	// is guaranteed to only emit one instruction.
	void movw(Register reg, u32 immediate, Condition cond = al);
	// The constant for movt should be in the range 0-0xffff.
	void movt(Register reg, u32 immediate, Condition cond = al);

	void bic(Register dst, Register src1, const Operand& src2,
			 SBit s = LeaveCC, Condition cond = al);

	void mvn(Register dst, const Operand& src,
			 SBit s = LeaveCC, Condition cond = al);

	// Multiply instructions

	void mla(Register dst, Register src1, Register src2, Register srcA,
			 SBit s = LeaveCC, Condition cond = al);

	void mul(Register dst, Register src1, Register src2,
			 SBit s = LeaveCC, Condition cond = al);

	void smlal(Register dstL, Register dstH, Register src1, Register src2,
			   SBit s = LeaveCC, Condition cond = al);

	void smull(Register dstL, Register dstH, Register src1, Register src2,
			   SBit s = LeaveCC, Condition cond = al);

	void umlal(Register dstL, Register dstH, Register src1, Register src2,
			   SBit s = LeaveCC, Condition cond = al);

	void umull(Register dstL, Register dstH, Register src1, Register src2,
			   SBit s = LeaveCC, Condition cond = al);

	// Miscellaneous arithmetic instructions

	void clz(Register dst, Register src, Condition cond = al);  // v5 and above

#if defined(CAN_USE_ARMV7_INSTRUCTIONS)
	// Saturating instructions. v6 and above.

	// Unsigned saturate.
	//
	// Saturate an optionally shifted signed value to an unsigned range.
	//
	//   usat dst, #satpos, src
	//   usat dst, #satpos, src, lsl #sh
	//   usat dst, #satpos, src, asr #sh
	//
	// Register dst will contain:
	//
	//   0,                 if s < 0
	//   (1 << satpos) - 1, if s > ((1 << satpos) - 1)
	//   s,                 otherwise
	//
	// where s is the contents of src after shifting (if used.)
	void usat(Register dst, int satpos, const Operand& src, Condition cond = al);

	// Bitfield manipulation instructions. v7 and above.

	void ubfx(Register dst, Register src, int lsb, int width,
			  Condition cond = al);

	void sbfx(Register dst, Register src, int lsb, int width,
			  Condition cond = al);

	void bfc(Register dst, int lsb, int width, Condition cond = al);

	void bfi(Register dst, Register src, int lsb, int width,
			 Condition cond = al);
#endif // defined(CAN_USE_ARMV7_INSTRUCTIONS)

	// Status register access instructions

	void mrs(Register dst, SRegister s, Condition cond = al);
	void msr(SRegisterFieldMask fields, const Operand& src, Condition cond = al);

	// Load/Store instructions
	void ldr(Register dst, const MemOperand& src, Condition cond = al);
	void str(Register src, const MemOperand& dst, Condition cond = al);
	void ldrb(Register dst, const MemOperand& src, Condition cond = al);
	void strb(Register src, const MemOperand& dst, Condition cond = al);
	void ldrh(Register dst, const MemOperand& src, Condition cond = al);
	void strh(Register src, const MemOperand& dst, Condition cond = al);
	void ldrsb(Register dst, const MemOperand& src, Condition cond = al);
	void ldrsh(Register dst, const MemOperand& src, Condition cond = al);
	void ldrd(Register dst1,
			  Register dst2,
			  const MemOperand& src, Condition cond = al);
	void strd(Register src1,
			  Register src2,
			  const MemOperand& dst, Condition cond = al);

	// Load/Store multiple instructions
	void ldm(BlockAddrMode am, Register base, RegList dst, Condition cond = al);
	void stm(BlockAddrMode am, Register base, RegList src, Condition cond = al);

	// Exception-generating instructions and debugging support
	void stop(const char* msg,
			  Condition cond = al,
			  s32 code = kDefaultStopCode);

	void bkpt(u32 imm16);  // v5 and above
	void svc(u32 imm24, Condition cond = al);

	// Coprocessor instructions

	void cdp(Coprocessor coproc, int opcode_1,
			 CRegister crd, CRegister crn, CRegister crm,
			 int opcode_2, Condition cond = al);

	void cdp2(Coprocessor coproc, int opcode_1,
			  CRegister crd, CRegister crn, CRegister crm,
			  int opcode_2);  // v5 and above

	void mcr(Coprocessor coproc, int opcode_1,
			 Register rd, CRegister crn, CRegister crm,
			 int opcode_2 = 0, Condition cond = al);

	void mcr2(Coprocessor coproc, int opcode_1,
			  Register rd, CRegister crn, CRegister crm,
			  int opcode_2 = 0);  // v5 and above

	void mrc(Coprocessor coproc, int opcode_1,
			 Register rd, CRegister crn, CRegister crm,
			 int opcode_2 = 0, Condition cond = al);

	void mrc2(Coprocessor coproc, int opcode_1,
			  Register rd, CRegister crn, CRegister crm,
			  int opcode_2 = 0);  // v5 and above

	void ldc(Coprocessor coproc, CRegister crd, const MemOperand& src,
			 LFlag l = Short, Condition cond = al);
	void ldc(Coprocessor coproc, CRegister crd, Register base, int option,
			 LFlag l = Short, Condition cond = al);

	void ldc2(Coprocessor coproc, CRegister crd, const MemOperand& src,
			  LFlag l = Short);  // v5 and above
	void ldc2(Coprocessor coproc, CRegister crd, Register base, int option,
			  LFlag l = Short);  // v5 and above

#if defined(CAN_USE_VFP_INSTRUCTIONS)
	// Support for VFP.
	// All these APIs support S0 to S31 and D0 to D15.
	// Currently these APIs do not support extended D registers, i.e, D16 to D31.
	// However, some simple modifications can allow
	// these APIs to support D16 to D31.

	void vldr(const DwVfpRegister dst,
			  const Register base,
			  int offset,
			  const Condition cond = al);
	void vldr(const DwVfpRegister dst,
			  const MemOperand& src,
			  const Condition cond = al);

	void vldr(const SwVfpRegister dst,
			  const Register base,
			  int offset,
			  const Condition cond = al);
	void vldr(const SwVfpRegister dst,
			  const MemOperand& src,
			  const Condition cond = al);

	void vstr(const DwVfpRegister src,
			  const Register base,
			  int offset,
			  const Condition cond = al);
	void vstr(const DwVfpRegister src,
			  const MemOperand& dst,
			  const Condition cond = al);

	void vstr(const SwVfpRegister src,
			  const Register base,
			  int offset,
			  const Condition cond = al);
	void vstr(const SwVfpRegister src,
			  const MemOperand& dst,
			  const Condition cond = al);

	void vldm(BlockAddrMode am,
			  Register base,
			  DwVfpRegister first,
			  DwVfpRegister last,
			  Condition cond = al);

	void vstm(BlockAddrMode am,
			  Register base,
			  DwVfpRegister first,
			  DwVfpRegister last,
			  Condition cond = al);

	void vldm(BlockAddrMode am,
			  Register base,
			  SwVfpRegister first,
			  SwVfpRegister last,
			  Condition cond = al);

	void vstm(BlockAddrMode am,
			  Register base,
			  SwVfpRegister first,
			  SwVfpRegister last,
			  Condition cond = al);

	void vmov(const DwVfpRegister dst,
			  double imm,
			  const Condition cond = al);
	void vmov(const SwVfpRegister dst,
			  const SwVfpRegister src,
			  const Condition cond = al);
	void vmov(const DwVfpRegister dst,
			  const DwVfpRegister src,
			  const Condition cond = al);
	void vmov(const DwVfpRegister dst,
			  const Register src1,
			  const Register src2,
			  const Condition cond = al);
	void vmov(const Register dst1,
			  const Register dst2,
			  const DwVfpRegister src,
			  const Condition cond = al);
	void vmov(const SwVfpRegister dst,
			  const Register src,
			  const Condition cond = al);
	void vmov(const Register dst,
			  const SwVfpRegister src,
			  const Condition cond = al);
	void vcvt_f64_s32(const DwVfpRegister dst,
					  const SwVfpRegister src,
					  VFPConversionMode mode = kDefaultRoundToZero,
					  const Condition cond = al);
	void vcvt_f32_s32(const SwVfpRegister dst,
					  const SwVfpRegister src,
					  VFPConversionMode mode = kDefaultRoundToZero,
					  const Condition cond = al);
	void vcvt_f64_u32(const DwVfpRegister dst,
					  const SwVfpRegister src,
					  VFPConversionMode mode = kDefaultRoundToZero,
					  const Condition cond = al);
	void vcvt_s32_f64(const SwVfpRegister dst,
					  const DwVfpRegister src,
					  VFPConversionMode mode = kDefaultRoundToZero,
					  const Condition cond = al);
	void vcvt_u32_f64(const SwVfpRegister dst,
					  const DwVfpRegister src,
					  VFPConversionMode mode = kDefaultRoundToZero,
					  const Condition cond = al);
	void vcvt_f64_f32(const DwVfpRegister dst,
					  const SwVfpRegister src,
					  VFPConversionMode mode = kDefaultRoundToZero,
					  const Condition cond = al);
	void vcvt_f32_f64(const SwVfpRegister dst,
					  const DwVfpRegister src,
					  VFPConversionMode mode = kDefaultRoundToZero,
					  const Condition cond = al);

	void vneg(const DwVfpRegister dst,
			  const DwVfpRegister src,
			  const Condition cond = al);
	void vabs(const DwVfpRegister dst,
			  const DwVfpRegister src,
			  const Condition cond = al);
	void vadd(const DwVfpRegister dst,
			  const DwVfpRegister src1,
			  const DwVfpRegister src2,
			  const Condition cond = al);
	void vsub(const DwVfpRegister dst,
			  const DwVfpRegister src1,
			  const DwVfpRegister src2,
			  const Condition cond = al);
	void vmul(const DwVfpRegister dst,
			  const DwVfpRegister src1,
			  const DwVfpRegister src2,
			  const Condition cond = al);
	void vdiv(const DwVfpRegister dst,
			  const DwVfpRegister src1,
			  const DwVfpRegister src2,
			  const Condition cond = al);
	void vcmp(const DwVfpRegister src1,
			  const DwVfpRegister src2,
			  const Condition cond = al);
	void vcmp(const DwVfpRegister src1,
			  const double src2,
			  const Condition cond = al);
	void vmrs(const Register dst,
			  const Condition cond = al);
	void vmsr(const Register dst,
			  const Condition cond = al);
	void vsqrt(const DwVfpRegister dst,
			   const DwVfpRegister src,
			   const Condition cond = al);
#endif // defined(CAN_USE_VFP_INSTRUCTIONS)

	// Pseudo instructions
	void nop(int type = 0);   // 0 is the default non-marking type.
	void push(Register src, Condition cond = al);
	void pop(Register dst, Condition cond = al);
	void pop();
	// Jump unconditionally to given label.
	void jmp(Label* L) { b(L, al); }

	// Check the code size generated from label to here.
	int sizeOfCodeGeneratedSince(Label* label) {
		return pcOffset() - label->pos();
	}

	// Check the number of instructions generated from label to here.
	int instructionsGeneratedSince(Label* label) {
		return sizeOfCodeGeneratedSince(label) / kInstrSize;
	}

	// Writes a single byte or word of data in the code stream.  Used
	// for inline tables, e.g., jump-tables. The constant pool should be
	// emitted before any use of db and dd to ensure that constant pools
	// are not emitted as part of the tables generated.
	void db(u8 data);
	void dd(u32 data);

	void setPcOffset(int offset) {
		pc_ = buffer_ + offset;
		m_noConstPoolBefore = offset;
	}
	int pcOffset() const { return pc_ - buffer_; }

	// Read/patch instructions
	Instr instrAt(int pos) { return *reinterpret_cast<Instr *>(buffer_ + pos); }
	void putInstrAt(int pos, Instr instr) {
		*reinterpret_cast<Instr *>(buffer_ + pos) = instr;
	}
	static Instr instrAt(u8 *pc) { return *reinterpret_cast<Instr *>(pc); }
	static void putInstrAt(u8 *pc, Instr instr) {
		*reinterpret_cast<Instr *>(pc) = instr;
	}
	static Condition GetCondition(Instr instr);
	static bool IsBranch(Instr instr);
	static int GetBranchOffset(Instr instr);
	static bool IsLdrRegisterImmediate(Instr instr);
	static int GetLdrRegisterImmediateOffset(Instr instr);
	static Instr SetLdrRegisterImmediateOffset(Instr instr, int offset);
	static bool IsStrRegisterImmediate(Instr instr);
	static Instr SetStrRegisterImmediateOffset(Instr instr, int offset);
	static bool IsAddRegisterImmediate(Instr instr);
	static Instr SetAddRegisterImmediateOffset(Instr instr, int offset);
	static Register GetRd(Instr instr);
	static Register GetRn(Instr instr);
	static Register GetRm(Instr instr);
	static bool IsPush(Instr instr);
	static bool IsPop(Instr instr);
	static bool IsStrRegFpOffset(Instr instr);
	static bool IsLdrRegFpOffset(Instr instr);
	static bool IsStrRegFpNegOffset(Instr instr);
	static bool IsLdrRegFpNegOffset(Instr instr);
	static bool IsLdrPcImmediateOffset(Instr instr);
	static bool IsTstImmediate(Instr instr);
	static bool IsCmpRegister(Instr instr);
	static bool IsCmpImmediate(Instr instr);
	static Register GetCmpImmediateRegister(Instr instr);
	static int GetCmpImmediateRawImmediate(Instr instr);
	static bool IsNop(Instr instr, int type = 0);

	// Check whether an immediate fits an addressing mode 1 instruction.
	bool ImmediateFitsAddrMode1Instruction(u32 imm32);

	// Constants in pools are accessed via pc relative addressing, which can
	// reach +/-4KB thereby defining a maximum distance between the instruction
	// and the accessed constant.
	static const int kMaxDistToPool = 4*1024;
	static const int kMaxNumPendingRelocInfo = kMaxDistToPool/sizeof(Instr);

	// Postpone the generation of the constant pool for the specified number of
	// instructions.
	void constPoolBlockFor(int instructions);

	// Check if is time to emit a constant pool.
	void constPoolCheck(bool forceEmit, bool requireJump);

	// Record reloc info for current pc_
	void constPoolRecordLdrStrItem(u32 offset = 0);

	// Decode branch instruction at pos and return branch target pos
	int targetAt(int pos);

	// Patch branch instruction at pos to branch to given branch target pos
	void putTargetAt(int pos, int target_pos);
private:
	// Code buffer:
	// The buffer into which code and relocation info are generated.
	u8* buffer_;
	int buffer_size_;

	// Code generation
	// The relocation writer's position is at least kGap bytes below the end of
	// the generated instructions. This is so that multi-instruction sequences do
	// not have to check for overflow. The same is true for writes of large
	// relocation info entries.
	static const int kGap = 32;
	u8* pc_;  // the program counter; moves forward

	// Code emission
	inline void checkBuffer();
	inline int bufferSpace();
	inline void iemit(Instr x);

	// Instruction generation
	void addrmod1(Instr instr, Register rn, Register rd, const Operand& x);
	void addrmod2(Instr instr, Register rd, const MemOperand& x);
	void addrmod3(Instr instr, Register rd, const MemOperand& x);
	void addrmod4(Instr instr, Register rn, RegList rl);
	void addrmod5(Instr instr, CRegister crd, const MemOperand& x);

	// Labels
	void print(Label* L);
	void bindTo(Label* L, int pos);
	void linkTo(Label* L, Label* appendix);
	void next(Label* L);
	bool constPoolIsBlocked() const;
	void constPoolBeginBlock();
	void constPoolEndBlock();

	// Constant pool generation
	// Pools are emitted in the instruction stream, preferably after unconditional
	// jumps or after returns from functions (in dead code locations).
	// If a long code sequence does not contain unconditional jumps, it is
	// necessary to emit the constant pool before the pool gets too far from the
	// location it is accessed from. In this case, we emit a jump over the emitted
	// constant pool.
	// Constants in the pool may be addresses of functions that gets relocated;
	// if so, a relocation info entry is associated to the constant pool entry.

	// Repeated checking whether the constant pool should be emitted is rather
	// expensive. By default we only check again once a number of instructions
	// has been generated. That also means that the sizing of the buffers is not
	// an exact science, and that we rely on some slop to not overrun buffers.
	static const int kCheckPoolIntervalInst = 32;
	static const int kCheckPoolInterval = kCheckPoolIntervalInst * kInstrSize;

	// Average distance beetween a constant pool and the first instruction
	// accessing the constant pool. Longer distance should result in less I-cache
	// pollution.
	// In practice the distance will be smaller since constant pool emission is
	// forced after function return and sometimes after unconditional branches.
	static const int kAvgDistToPool = kMaxDistToPool - kCheckPoolInterval;

	// Emission of the constant pool may be blocked in some code sequences.
	int m_constPoolBlockedNesting;  // Block emission if this is not zero.
	int m_noConstPoolBefore;  // Block emission before this pc offset.

	// Keep track of the first instruction requiring a constant pool entry
	// since the previous constant pool was emitted.
	int m_constPoolFirstUse;

	// Relocation info records are also used during code generation as temporary
	// containers for constants and code target addresses until they are emitted
	// to the constant pool. These pending relocation info records are temporarily
	// stored in a separate buffer until a constant pool is emitted.
	// If every instruction in a long sequence is accessing the pool, we need one
	// pending relocation entry per instruction.

	class ConstPoolItem
	{
	public:
		u8 *pc;
		u32 data;
	};

	// the buffer of pending relocation info
	ConstPoolItem m_constPoolPendingItems[kMaxNumPendingRelocInfo];
	// number of pending reloc info entries in the buffer
	int m_constPoolNumPendingItems;
	int m_constPoolNextBufferCheck;  // pc offset of next buffer check

	friend class BlockConstPoolScope;
	friend class MacroAssembler;
};

inline void Assembler::svc(u32 imm24, Condition cond)
{
	Q_ASSERT(isUint24(imm24));
	iemit(cond | 15*B24 | imm24);
}

// Class for scoping postponing the constant pool generation.
class BlockConstPoolScope
{
public:
	explicit BlockConstPoolScope(Assembler* assembler) : m_assembler(assembler) {
		m_assembler->constPoolBeginBlock();
	}
	~BlockConstPoolScope() {
		m_assembler->constPoolEndBlock();
	}
private:
	DISABLE_IMPLICIT_CONSTRUCTORS(BlockConstPoolScope)
	Assembler* m_assembler;
};


} // namespace Arm

#endif  // ARM_ASSEMBLER_H
