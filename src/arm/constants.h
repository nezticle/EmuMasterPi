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

#ifndef ARM_CONSTANTS_H
#define ARM_CONSTANTS_H

#include <base/emu.h>

// ARM EABI is required.
#if defined(__arm__) && !defined(__ARM_EABI__)
#error ARM EABI support is required.
#endif

// This means that interwork-compatible jump instructions are generated.  We
// want to generate them on the simulator too so it makes snapshots that can
// be used on real hardware.
#if defined(__THUMB_INTERWORK__) || !defined(__arm__)
# define USE_THUMB_INTERWORK 1
#endif

#if defined(__ARM_ARCH_7A__) || \
	defined(__ARM_ARCH_7R__) || \
	defined(__ARM_ARCH_7__)
# define CAN_USE_ARMV7_INSTRUCTIONS 1
#endif

#if defined(CAN_USE_ARMV7_INSTRUCTIONS) || \
	defined(__ARM_ARCH_6__)   ||   \
	defined(__ARM_ARCH_6J__)  ||  \
	defined(__ARM_ARCH_6K__)  ||  \
	defined(__ARM_ARCH_6Z__)  ||  \
	defined(__ARM_ARCH_6ZK__) || \
	defined(__ARM_ARCH_6T2__)
# define CAN_USE_ARMV6_INSTRUCTIONS 1
#endif

#if defined(CAN_USE_ARMV6_INSTRUCTIONS) || \
	defined(__ARM_ARCH_5T__)            || \
	defined(__ARM_ARCH_5TE__)
# define CAN_USE_ARMV5_INSTRUCTIONS 1
# define CAN_USE_THUMB_INSTRUCTIONS 1
#endif

#if defined(__VFP_FP__) && !defined(__SOFTFP__)
# define CAN_USE_VFP_INSTRUCTIONS 1
#endif

// Using blx may yield better code, so use it when required or when available
#if defined(USE_THUMB_INTERWORK) || defined(CAN_USE_ARMV5_INSTRUCTIONS)
# define USE_BLX 1
#endif

// Using bx does not yield better code, so use it only when required
#if defined(USE_THUMB_INTERWORK)
#define USE_BX 1
#endif

#if defined(__ARM_NEON__)
#define CAN_USE_NEON_INSTRUCTIONS 1
#endif

#define Q_ASSERT_EQ(a,b) Q_ASSERT((a) == (b))
#define Q_ASSERT_GT(a,b) Q_ASSERT((a) >  (b))
#define Q_ASSERT_GE(a,b) Q_ASSERT((a) >= (b))
#define Q_ASSERT_LT(a,b) Q_ASSERT((a) <  (b))
#define Q_ASSERT_LE(a,b) Q_ASSERT((a) <= (b))

namespace Arm {

// A macro is used for defining the base class used for embedded instances.
// The reason is some compilers allocate a minimum of one word for the
// superclass. The macro prevents the use of new & delete in debug mode.
// In release mode we are not willing to pay this overhead.

#if !defined(QT_NO_DEBUG)
// Superclass for classes with instances allocated inside stack
// activations or inside other objects.
class Embedded
{
public:
	void* operator new(size_t size);
	void  operator delete(void* p);
};
#define BASE_EMBEDDED : public Embedded
#else
#define BASE_EMBEDDED
#endif

static const int kPointerSize  = sizeof(void*);     // NOLINT

// Constant pool marker.
static const int kConstantPoolMarkerMask = 0xffe00000;
static const int kConstantPoolMarker = 0x0c000000;
static const int kConstantPoolLengthMask = 0x001ffff;

// Number of registers in normal ARM mode.
static const int kNumRegisters = 16;

// VFP support.
static const int kNumVFPSingleRegisters = 32;
static const int kNumVFPDoubleRegisters = 16;
static const int kNumVFPRegisters = kNumVFPSingleRegisters + kNumVFPDoubleRegisters;

// PC is register 15.
static const int kPCRegister = 15;
static const int kNoRegister = -1;

// -----------------------------------------------------------------------------
// Conditions.

// Defines constants and accessor classes to assemble, disassemble and
// simulate ARM instructions.
//
// Section references in the code refer to the "ARM Architecture Reference
// Manual" from July 2005 (available at http://www.arm.com/miscPDFs/14128.pdf)
//
// Constants for specific fields are defined in their respective named enums.
// General constants are in an anonymous enum in class Instr.

// Values for the condition field as defined in section A3.2
enum Condition {
	kNoCondition = -1,

	eq =  0 << 28,                 // Z set            Equal.
	ne =  1 << 28,                 // Z clear          Not equal.
	cs =  2 << 28,                 // C set            Unsigned higher or same.
	cc =  3 << 28,                 // C clear          Unsigned lower.
	mi =  4 << 28,                 // N set            Negative.
	pl =  5 << 28,                 // N clear          Positive or zero.
	vs =  6 << 28,                 // V set            Overflow.
	vc =  7 << 28,                 // V clear          No overflow.
	hi =  8 << 28,                 // C set, Z clear   Unsigned higher.
	ls =  9 << 28,                 // C clear or Z set Unsigned lower or same.
	ge = 10 << 28,                 // N == V           Greater or equal.
	lt = 11 << 28,                 // N != V           Less than.
	gt = 12 << 28,                 // Z clear, N == V  Greater than.
	le = 13 << 28,                 // Z set or N != V  Less then or equal
	al = 14 << 28,                 //                  Always.

	kSpecialCondition = 15 << 28,  // Special condition (refer to section A3.2.1).
	kNumberOfConditions = 16,

	// Aliases.
	hs = cs,                       // C set            Unsigned higher or same.
	lo = cc                        // C clear          Unsigned lower.
};


static inline Condition NegateCondition(Condition cond)
{
	Q_ASSERT(cond != al);
	return static_cast<Condition>(cond ^ ne);
}


// Corresponds to transposing the operands of a comparison.
static inline Condition ReverseCondition(Condition cond)
{
	switch (cond) {
	case lo: return hi;
	case hi: return lo;
	case hs: return ls;
	case ls: return hs;
	case lt: return gt;
	case gt: return lt;
	case ge: return le;
	case le: return ge;
	default:
		return cond;
	};
}

// -----------------------------------------------------------------------------
// Instructions encoding.

// Instr is merely used by the Assembler to distinguish 32bit integers
// representing instructions from usual 32 bit values.
// Instruction objects are pointers to 32bit values, and provide methods to
// access the various ISA fields.
typedef s32 Instr;

// Opcodes for Data-processing instructions (instructions with a type 0 and 1)
// as defined in section A3.4
enum Opcode {
	AND =  0 << 21,  // Logical AND.
	EOR =  1 << 21,  // Logical Exclusive OR.
	SUB =  2 << 21,  // Subtract.
	RSB =  3 << 21,  // Reverse Subtract.
	ADD =  4 << 21,  // Add.
	ADC =  5 << 21,  // Add with Carry.
	SBC =  6 << 21,  // Subtract with Carry.
	RSC =  7 << 21,  // Reverse Subtract with Carry.
	TST =  8 << 21,  // Test.
	TEQ =  9 << 21,  // Test Equivalence.
	CMP = 10 << 21,  // Compare.
	CMN = 11 << 21,  // Compare Negated.
	ORR = 12 << 21,  // Logical (inclusive) OR.
	MOV = 13 << 21,  // Move.
	BIC = 14 << 21,  // Bit Clear.
	MVN = 15 << 21   // Move Not.
};

// The bits for bit 7-4 for some type 0 miscellaneous instructions.
enum MiscInstructionsBits74 {
	// With bits 22-21 01.
	BX   =  1 << 4,
	BXJ  =  2 << 4,
	BLX  =  3 << 4,
	BKPT =  7 << 4,

	// With bits 22-21 11.
	CLZ  =  1 << 4
};

// Instruction encoding bits and masks.
enum {
	H   = 1 << 5,   // Halfword (or byte).
	S6  = 1 << 6,   // Signed (or unsigned).
	L   = 1 << 20,  // Load (or store).
	S   = 1 << 20,  // Set condition code (or leave unchanged).
	W   = 1 << 21,  // Writeback base register (or leave unchanged).
	A   = 1 << 21,  // Accumulate in multiply instruction (or not).
	B   = 1 << 22,  // Unsigned byte (or word).
	N   = 1 << 22,  // Long (or short).
	U   = 1 << 23,  // Positive (or negative) offset/index.
	P   = 1 << 24,  // Offset/pre-indexed addressing (or post-indexed addressing).
	I   = 1 << 25,  // Immediate shifter operand (or not).

	B4  = 1 << 4,
	B5  = 1 << 5,
	B6  = 1 << 6,
	B7  = 1 << 7,
	B8  = 1 << 8,
	B9  = 1 << 9,
	B12 = 1 << 12,
	B16 = 1 << 16,
	B18 = 1 << 18,
	B19 = 1 << 19,
	B20 = 1 << 20,
	B21 = 1 << 21,
	B22 = 1 << 22,
	B23 = 1 << 23,
	B24 = 1 << 24,
	B25 = 1 << 25,
	B26 = 1 << 26,
	B27 = 1 << 27,
	B28 = 1 << 28,

	// Instruction bit masks.
	kCondMask   = 15 << 28,
	kALUMask    = 0x6f << 21,
	kRdMask     = 15 << 12,  // In str instruction.
	kCoprocessorMask = 15 << 8,
	kOpCodeMask = 15 << 21,  // In data-processing instructions.
	kImm24Mask  = (1 << 24) - 1,
	kOff12Mask  = (1 << 12) - 1
};

// -----------------------------------------------------------------------------
// Addressing modes and instruction variants.

// Condition code updating mode.
enum SBit {
	SetCC   = 1 << 20,  // Set condition code.
	LeaveCC = 0 << 20   // Leave condition code unchanged.
};

// Status register selection.
enum SRegister {
	CPSR = 0 << 22,
	SPSR = 1 << 22
};

// Shifter types for Data-processing operands as defined in section A5.1.2.
enum ShiftOp {
	LSL = 0 << 5,   // Logical shift left.
	LSR = 1 << 5,   // Logical shift right.
	ASR = 2 << 5,   // Arithmetic shift right.
	ROR = 3 << 5,   // Rotate right.

	// RRX is encoded as ROR with shift_imm == 0.
	// Use a special code to make the distinction. The RRX ShiftOp is only used
	// as an argument, and will never actually be encoded. The Assembler will
	// detect it and emit the correct ROR shift operand with shift_imm == 0.
	RRX = -1,
	kNumberOfShifts = 4
};

// Status register fields.
enum SRegisterField {
	CPSR_c = CPSR | 1 << 16,
	CPSR_x = CPSR | 1 << 17,
	CPSR_s = CPSR | 1 << 18,
	CPSR_f = CPSR | 1 << 19,
	SPSR_c = SPSR | 1 << 16,
	SPSR_x = SPSR | 1 << 17,
	SPSR_s = SPSR | 1 << 18,
	SPSR_f = SPSR | 1 << 19
};

static const u32 kNConditionFlagBit = 1 << 31;
static const u32 kZConditionFlagBit = 1 << 30;
static const u32 kCConditionFlagBit = 1 << 29;
static const u32 kVConditionFlagBit = 1 << 28;

// Status register field mask (or'ed SRegisterField enum values).
typedef u32 SRegisterFieldMask;

// Memory operand addressing mode.
enum AddrMode {
	// Bit encoding P U W.
	Offset       = (8|4|0) << 21,  // Offset (without writeback to base).
	PreIndex     = (8|4|1) << 21,  // Pre-indexed addressing with writeback.
	PostIndex    = (0|4|0) << 21,  // Post-indexed addressing with writeback.
	NegOffset    = (8|0|0) << 21,  // Negative offset (without writeback to base).
	NegPreIndex  = (8|0|1) << 21,  // Negative pre-indexed with writeback.
	NegPostIndex = (0|0|0) << 21   // Negative post-indexed with writeback.
};


// Load/store multiple addressing mode.
enum BlockAddrMode {
	// Bit encoding P U W .
	da           = (0|0|0) << 21,  // Decrement after.
	ia           = (0|4|0) << 21,  // Increment after.
	db           = (8|0|0) << 21,  // Decrement before.
	ib           = (8|4|0) << 21,  // Increment before.
	da_w         = (0|0|1) << 21,  // Decrement after with writeback to base.
	ia_w         = (0|4|1) << 21,  // Increment after with writeback to base.
	db_w         = (8|0|1) << 21,  // Decrement before with writeback to base.
	ib_w         = (8|4|1) << 21,  // Increment before with writeback to base.

	// Alias modes for comparison when writeback does not matter.
	da_x         = (0|0|0) << 21,  // Decrement after.
	ia_x         = (0|4|0) << 21,  // Increment after.
	db_x         = (8|0|0) << 21,  // Decrement before.
	ib_x         = (8|4|0) << 21,  // Increment before.

	kBlockAddrModeMask = (8|4|1) << 21
};


// Coprocessor load/store operand size.
enum LFlag {
	Long  = 1 << 22,  // Long load/store coprocessor.
	Short = 0 << 22   // Short load/store coprocessor.
};


// -----------------------------------------------------------------------------
// Supervisor Call (svc) specific support.

// Special Software Interrupt codes when used in the presence of the ARM
// simulator.
// svc (formerly swi) provides a 24bit immediate value. Use bits 22:0 for
// standard SoftwareInterrupCode. Bit 23 is reserved for the stop feature.
enum SoftwareInterruptCodes {
	// transition to C code
	kCallRtRedirected= 0x10,
	// break point
	kBreakpoint= 0x20,
	// stop
	kStopCode = 1 << 23
};
static const u32 kStopCodeMask = kStopCode - 1;
static const u32 kMaxStopCode = kStopCode - 1;
static const s32 kDefaultStopCode = -1;


// Type of VFP register. Determines register encoding.
enum VFPRegPrecision {
	kSinglePrecision = 0,
	kDoublePrecision = 1
};


// VFP FPSCR constants.
enum VFPConversionMode {
	kFPSCRRounding = 0,
	kDefaultRoundToZero = 1
};

// This mask does not include the "inexact" or "input denormal" cumulative
// exceptions flags, because we usually don't want to check for it.
static const u32 kVFPExceptionMask = 0xf;
static const u32 kVFPInvalidOpExceptionBit = 1 << 0;
static const u32 kVFPOverflowExceptionBit = 1 << 2;
static const u32 kVFPUnderflowExceptionBit = 1 << 3;
static const u32 kVFPInexactExceptionBit = 1 << 4;
static const u32 kVFPFlushToZeroMask = 1 << 24;

static const u32 kVFPNConditionFlagBit = 1 << 31;
static const u32 kVFPZConditionFlagBit = 1 << 30;
static const u32 kVFPCConditionFlagBit = 1 << 29;
static const u32 kVFPVConditionFlagBit = 1 << 28;


// VFP rounding modes. See ARM DDI 0406B Page A2-29.
enum VFPRoundingMode {
	RN = 0 << 22,   // Round to Nearest.
	RP = 1 << 22,   // Round towards Plus Infinity.
	RM = 2 << 22,   // Round towards Minus Infinity.
	RZ = 3 << 22,   // Round towards zero.

	// Aliases.
	kRoundToNearest = RN,
	kRoundToPlusInf = RP,
	kRoundToMinusInf = RM,
	kRoundToZero = RZ
};

static const u32 kVFPRoundingModeMask = 3 << 22;

enum CheckForInexactConversion {
	kCheckForInexactConversion,
	kDontCheckForInexactConversion
};


// Helper functions for converting between register numbers and names.
class Registers
{
public:
	// Return the name of the register.
	static const char* Name(int reg);

	// Lookup the register number for the name provided.
	static int Number(const char* name);

	struct RegisterAlias {
		int reg;
		const char* name;
	};

private:
	static const char* m_names[kNumRegisters];
	static const RegisterAlias m_aliases[];
};

// Helper functions for converting between VFP register numbers and names.
class VFPRegisters
{
public:
	// Return the name of the register.
	static const char* Name(int reg, bool isDouble);

	// Lookup the register number for the name provided.
	// Set flag pointed by is_double to true if register
	// is double-precision.
	static int Number(const char* name, bool* isDouble);

private:
	static const char* m_names[kNumVFPRegisters];
};


} // namespace Arm

#endif // ARM_CONSTANTS_H
