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
 */

#ifndef ARM_INSTRUCTION_H
#define ARM_INSTRUCTION_H

#include "register.h"

namespace Arm {

// -----------------------------------------------------------------------------
// Specific instructions, constants, and masks.

// add(sp, sp, 4) instruction (aka Pop())
static const Instr kPopInstruction =
	al | PostIndex | 4 | LeaveCC | I | sp.code() * B16 | sp.code() * B12;
// str(r, MemOperand(sp, 4, NegPreIndex), al) instruction (aka push(r))
// register r is not encoded.
static const Instr kPushRegPattern =
	al | B26 | 4 | NegPreIndex | sp.code() * B16;
// ldr(r, MemOperand(sp, 4, PostIndex), al) instruction (aka pop(r))
// register r is not encoded.
static const Instr kPopRegPattern =
	al | B26 | L | 4 | PostIndex | sp.code() * B16;
// mov lr, pc
static const Instr kMovLrPc = al | MOV | pc.code() | lr.code() * B12;
// ldr rd, [pc, #offset]
static const Instr kLdrPCMask = kCondMask | 15 * B24 | 7 * B20 | 15 * B16;
static const Instr kLdrPCPattern = al | 5 * B24 | L | pc.code() * B16;
// blxcc rm
static const Instr kBlxRegMask =
	15 * B24 | 15 * B20 | 15 * B16 | 15 * B12 | 15 * B8 | 15 * B4;
static const Instr kBlxRegPattern =
	B24 | B21 | 15 * B16 | 15 * B12 | 15 * B8 | BLX;
static const Instr kMovMvnMask = 0x6d * B21 | 0xf * B16;
static const Instr kMovMvnPattern = 0xd * B21;
static const Instr kMovMvnFlip = B22;
static const Instr kMovLeaveCCMask = 0xdff * B16;
static const Instr kMovLeaveCCPattern = 0x1a0 * B16;
static const Instr kMovwMask = 0xff * B20;
static const Instr kMovwPattern = 0x30 * B20;
static const Instr kMovwLeaveCCFlip = 0x5 * B21;
static const Instr kCmpCmnMask = 0xdd * B20 | 0xf * B12;
static const Instr kCmpCmnPattern = 0x15 * B20;
static const Instr kCmpCmnFlip = B21;
static const Instr kAddSubFlip = 0x6 * B21;
static const Instr kAndBicFlip = 0xe * B21;

// A mask for the Rd register for push, pop, ldr, str instructions.
static const Instr kLdrRegFpOffsetPattern =
	al | B26 | L | Offset | fp.code() * B16;
static const Instr kStrRegFpOffsetPattern =
	al | B26 | Offset | fp.code() * B16;
static const Instr kLdrRegFpNegOffsetPattern =
	al | B26 | L | NegOffset | fp.code() * B16;
static const Instr kStrRegFpNegOffsetPattern =
	al | B26 | NegOffset | fp.code() * B16;
static const Instr kLdrStrInstrTypeMask = 0xffff0000;
static const Instr kLdrStrInstrArgumentMask = 0x0000ffff;
static const Instr kLdrStrOffsetMask = 0x00000fff;

static inline Instr EncodeMovwImmediate(u32 immediate)
{
	Q_ASSERT(immediate < 0x10000);
	return ((immediate & 0xf000) << 4) | (immediate & 0xfff);
}

// -----------------------------------------------------------------------------
// Instruction abstraction.

// The class Instruction enables access to individual fields defined in the ARM
// architecture instruction set encoding as described in figure A3-1.
// Note that the Assembler uses typedef s32 Instr.
//
// Example: Test whether the instruction at ptr does set the condition code
// bits.
//
// bool InstructionSetsConditionCodes(u8* ptr) {
//   Instruction* instr = Instruction::At(ptr);
//   int type = instr->TypeValue();
//   return ((type == 0) || (type == 1)) && instr->HasS();
// }
//
class Instruction
{
public:
	enum {
		kInstrSize = 4,
		kInstrSizeLog2 = 2,
		kPCReadOffset = 8
	};

	// Helper macro to define static accessors.
	// We use the cast to char* trick to bypass the strict anti-aliasing rules.
#define DECLARE_STATIC_TYPED_ACCESSOR(return_type, Name)                     \
	static inline return_type Name(Instr instr) {                              \
	char* temp = reinterpret_cast<char*>(&instr);                            \
	return reinterpret_cast<Instruction*>(temp)->Name();                     \
}

#define DECLARE_STATIC_ACCESSOR(Name) DECLARE_STATIC_TYPED_ACCESSOR(int, Name)

	// Get the raw instruction bits.
	inline Instr InstructionBits() const {
		return *reinterpret_cast<const Instr*>(this);
	}

	// Set the raw instruction bits to value.
	inline void SetInstructionBits(Instr value) {
		*reinterpret_cast<Instr*>(this) = value;
	}

	// Read one particular bit out of the instruction bits.
	inline int Bit(int nr) const {
		return (InstructionBits() >> nr) & 1;
	}

	// Read a bit field's value out of the instruction bits.
	inline int Bits(int hi, int lo) const {
		return (InstructionBits() >> lo) & ((2 << (hi - lo)) - 1);
	}

	// Read a bit field out of the instruction bits.
	inline int BitField(int hi, int lo) const {
		return InstructionBits() & (((2 << (hi - lo)) - 1) << lo);
	}

	// Static support.

	// Read one particular bit out of the instruction bits.
	static inline int Bit(Instr instr, int nr) {
		return (instr >> nr) & 1;
	}

	// Read the value of a bit field out of the instruction bits.
	static inline int Bits(Instr instr, int hi, int lo) {
		return (instr >> lo) & ((2 << (hi - lo)) - 1);
	}


	// Read a bit field out of the instruction bits.
	static inline int BitField(Instr instr, int hi, int lo) {
		return instr & (((2 << (hi - lo)) - 1) << lo);
	}


	// Accessors for the different named fields used in the ARM encoding.
	// The naming of these accessor corresponds to figure A3-1.
	//
	// Two kind of accessors are declared:
	// - <Name>Field() will return the raw field, i.e. the field's bits at their
	//   original place in the instruction encoding.
	//   e.g. if instr is the 'addgt r0, r1, r2' instruction, encoded as
	//   0xC0810002 ConditionField(instr) will return 0xC0000000.
	// - <Name>Value() will return the field value, shifted back to bit 0.
	//   e.g. if instr is the 'addgt r0, r1, r2' instruction, encoded as
	//   0xC0810002 ConditionField(instr) will return 0xC.


	// Generally applicable fields
	inline Condition ConditionValue() const {
		return static_cast<Condition>(Bits(31, 28));
	}
	inline Condition ConditionField() const {
		return static_cast<Condition>(BitField(31, 28));
	}
	DECLARE_STATIC_TYPED_ACCESSOR(Condition, ConditionValue)
	DECLARE_STATIC_TYPED_ACCESSOR(Condition, ConditionField)

	inline int TypeValue() const { return Bits(27, 25); }

	inline int RnValue() const { return Bits(19, 16); }
	DECLARE_STATIC_ACCESSOR(RnValue)
	inline int RdValue() const { return Bits(15, 12); }
	DECLARE_STATIC_ACCESSOR(RdValue)

	inline int CoprocessorValue() const { return Bits(11, 8); }
	// Support for VFP.
	// Vn(19-16) | Vd(15-12) |  Vm(3-0)
	inline int VnValue() const { return Bits(19, 16); }
	inline int VmValue() const { return Bits(3, 0); }
	inline int VdValue() const { return Bits(15, 12); }
	inline int NValue() const { return Bit(7); }
	inline int MValue() const { return Bit(5); }
	inline int DValue() const { return Bit(22); }
	inline int RtValue() const { return Bits(15, 12); }
	inline int PValue() const { return Bit(24); }
	inline int UValue() const { return Bit(23); }
	inline int Opc1Value() const { return (Bit(23) << 2) | Bits(21, 20); }
	inline int Opc2Value() const { return Bits(19, 16); }
	inline int Opc3Value() const { return Bits(7, 6); }
	inline int SzValue() const { return Bit(8); }
	inline int VLValue() const { return Bit(20); }
	inline int VCValue() const { return Bit(8); }
	inline int VAValue() const { return Bits(23, 21); }
	inline int VBValue() const { return Bits(6, 5); }
	inline int VFPNRegValue(VFPRegPrecision pre) {
		return VFPGlueRegValue(pre, 16, 7);
	}
	inline int VFPMRegValue(VFPRegPrecision pre) {
		return VFPGlueRegValue(pre, 0, 5);
	}
	inline int VFPDRegValue(VFPRegPrecision pre) {
		return VFPGlueRegValue(pre, 12, 22);
	}

	// Fields used in Data processing instructions
	inline int OpcodeValue() const {
		return static_cast<Opcode>(Bits(24, 21));
	}
	inline Opcode OpcodeField() const {
		return static_cast<Opcode>(BitField(24, 21));
	}
	inline int SValue() const { return Bit(20); }
	// with register
	inline int RmValue() const { return Bits(3, 0); }
	DECLARE_STATIC_ACCESSOR(RmValue)
	inline int ShiftValue() const { return static_cast<ShiftOp>(Bits(6, 5)); }
	inline ShiftOp ShiftField() const {
		return static_cast<ShiftOp>(BitField(6, 5));
	}
	inline int RegShiftValue() const { return Bit(4); }
	inline int RsValue() const { return Bits(11, 8); }
	inline int ShiftAmountValue() const { return Bits(11, 7); }
	// with immediate
	inline int RotateValue() const { return Bits(11, 8); }
	inline int Immed8Value() const { return Bits(7, 0); }
	inline int Immed4Value() const { return Bits(19, 16); }
	inline int ImmedMovwMovtValue() const {
		return Immed4Value() << 12 | Offset12Value(); }

	// Fields used in Load/Store instructions
	inline int PUValue() const { return Bits(24, 23); }
	inline int PUField() const { return BitField(24, 23); }
	inline int  BValue() const { return Bit(22); }
	inline int  WValue() const { return Bit(21); }
	inline int  LValue() const { return Bit(20); }
	// with register uses same fields as Data processing instructions above
	// with immediate
	inline int Offset12Value() const { return Bits(11, 0); }
	// multiple
	inline int RlistValue() const { return Bits(15, 0); }
	// extra loads and stores
	inline int SignValue() const { return Bit(6); }
	inline int HValue() const { return Bit(5); }
	inline int ImmedHValue() const { return Bits(11, 8); }
	inline int ImmedLValue() const { return Bits(3, 0); }

	// Fields used in Branch instructions
	inline int LinkValue() const { return Bit(24); }
	inline int SImmed24Value() const { return ((InstructionBits() << 8) >> 8); }

	// Fields used in Software interrupt instructions
	inline SoftwareInterruptCodes SvcValue() const {
		return static_cast<SoftwareInterruptCodes>(Bits(23, 0));
	}

	// Test for special encodings of type 0 instructions (extra loads and stores,
	// as well as multiplications).
	inline bool IsSpecialType0() const { return (Bit(7) == 1) && (Bit(4) == 1); }

	// Test for miscellaneous instructions encodings of type 0 instructions.
	inline bool IsMiscType0() const { return (Bit(24) == 1)
				&& (Bit(23) == 0)
				&& (Bit(20) == 0)
				&& ((Bit(7) == 0)); }

	// Test for a stop instruction.
	inline bool IsStop() const {
		return (TypeValue() == 7) && (Bit(24) == 1) && (SvcValue() >= kStopCode);
	}

	// Special accessors that test for existence of a value.
	inline bool HasS()    const { return SValue() == 1; }
	inline bool HasB()    const { return BValue() == 1; }
	inline bool HasW()    const { return WValue() == 1; }
	inline bool HasL()    const { return LValue() == 1; }
	inline bool HasU()    const { return UValue() == 1; }
	inline bool HasSign() const { return SignValue() == 1; }
	inline bool HasH()    const { return HValue() == 1; }
	inline bool HasLink() const { return LinkValue() == 1; }

	// Decoding the double immediate in the vmov instruction.
	double DoubleImmedVmov() const;

	// Instructions are read of out a code stream. The only way to get a
	// reference to an instruction is to convert a pointer. There is no way
	// to allocate or create instances of class Instruction.
	// Use the At(pc) function to create references to Instruction.
	static Instruction* At(u8* pc) {
		return reinterpret_cast<Instruction*>(pc);
	}


private:
	// Join split register codes, depending on single or double precision.
	// four_bit is the position of the least-significant bit of the four
	// bit specifier. one_bit is the position of the additional single bit
	// specifier.
	inline int VFPGlueRegValue(VFPRegPrecision pre, int four_bit, int one_bit) {
		if (pre == kSinglePrecision) {
			return (Bits(four_bit + 3, four_bit) << 1) | Bit(one_bit);
		}
		return (Bit(one_bit) << 4) | Bits(four_bit + 3, four_bit);
	}

	// We need to prevent the creation of instances of class Instruction.
	DISABLE_IMPLICIT_CONSTRUCTORS(Instruction)
};

} // namespace Arm

#endif // ARM_INSTRUCTION_H
