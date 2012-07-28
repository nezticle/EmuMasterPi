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

#ifndef ARM_ASSEM_INSTR_H
#define ARM_ASSEM_INSTR_H

#include "assembler.h"

namespace Arm {

// Utils for instruction info and manipulation

inline Condition Assembler::GetCondition(Instr instr)
{
	return Instruction::ConditionField(instr);
}

inline bool Assembler::IsBranch(Instr instr)
{
	return (instr & (B27 | B25)) == (B27 | B25);
}

inline int Assembler::GetBranchOffset(Instr instr)
{
	Q_ASSERT(IsBranch(instr));
	// Take the jump offset in the lower 24 bits, sign extend it and multiply it
	// with 4 to get the offset in bytes.
	return ((instr & kImm24Mask) << 8) >> 6;
}

inline bool Assembler::IsLdrRegisterImmediate(Instr instr)
{
	return (instr & (B27 | B26 | B25 | B22 | B20)) == (B26 | B20);
}

inline int Assembler::GetLdrRegisterImmediateOffset(Instr instr)
{
	Q_ASSERT(IsLdrRegisterImmediate(instr));
	bool positive = (instr & B23) == B23;
	int offset = instr & kOff12Mask;  // Zero extended offset.
	return positive ? offset : -offset;
}

inline Instr Assembler::SetLdrRegisterImmediateOffset(Instr instr, int offset)
{
	Q_ASSERT(IsLdrRegisterImmediate(instr));
	bool positive = offset >= 0;
	if (!positive) offset = -offset;
	Q_ASSERT(isUint12(offset));
	// Set bit indicating whether the offset should be added.
	instr = (instr & ~B23) | (positive ? B23 : 0);
	// Set the actual offset.
	return (instr & ~kOff12Mask) | offset;
}

inline bool Assembler::IsStrRegisterImmediate(Instr instr)
{
	return (instr & (B27 | B26 | B25 | B22 | B20)) == B26;
}

inline Instr Assembler::SetStrRegisterImmediateOffset(Instr instr, int offset)
{
	Q_ASSERT(IsStrRegisterImmediate(instr));
	bool positive = offset >= 0;
	if (!positive) offset = -offset;
	Q_ASSERT(isUint12(offset));
	// Set bit indicating whether the offset should be added.
	instr = (instr & ~B23) | (positive ? B23 : 0);
	// Set the actual offset.
	return (instr & ~kOff12Mask) | offset;
}

inline bool Assembler::IsAddRegisterImmediate(Instr instr)
{
	return (instr & (B27 | B26 | B25 | B24 | B23 | B22 | B21)) == (B25 | B23);
}

inline Instr Assembler::SetAddRegisterImmediateOffset(Instr instr, int offset)
{
	Q_ASSERT(IsAddRegisterImmediate(instr));
	Q_ASSERT(offset >= 0);
	Q_ASSERT(isUint12(offset));
	// Set the offset.
	return (instr & ~kOff12Mask) | offset;
}

inline Register Assembler::GetRd(Instr instr)
{
	Register reg;
	reg.code_ = Instruction::RdValue(instr);
	return reg;
}

inline Register Assembler::GetRn(Instr instr)
{
	Register reg;
	reg.code_ = Instruction::RnValue(instr);
	return reg;
}

inline Register Assembler::GetRm(Instr instr)
{
	Register reg;
	reg.code_ = Instruction::RmValue(instr);
	return reg;
}

inline bool Assembler::IsPush(Instr instr)
{
	return ((instr & ~kRdMask) == kPushRegPattern);
}

inline bool Assembler::IsPop(Instr instr)
{
	return ((instr & ~kRdMask) == kPopRegPattern);
}

inline bool Assembler::IsStrRegFpOffset(Instr instr)
{
	return ((instr & kLdrStrInstrTypeMask) == kStrRegFpOffsetPattern);
}

inline bool Assembler::IsLdrRegFpOffset(Instr instr)
{
	return ((instr & kLdrStrInstrTypeMask) == kLdrRegFpOffsetPattern);
}

inline bool Assembler::IsStrRegFpNegOffset(Instr instr)
{
	return ((instr & kLdrStrInstrTypeMask) == kStrRegFpNegOffsetPattern);
}

inline bool Assembler::IsLdrRegFpNegOffset(Instr instr)
{
	return ((instr & kLdrStrInstrTypeMask) == kLdrRegFpNegOffsetPattern);
}

inline bool Assembler::IsLdrPcImmediateOffset(Instr instr)
{
	// Check the instruction is indeed a
	// ldr<cond> <Rd>, [pc +/- offset_12].
	return (instr & (kLdrPCMask & ~kCondMask)) == 0x051f0000;
}

inline bool Assembler::IsTstImmediate(Instr instr)
{
	return (instr & (B27 | B26 | I | kOpCodeMask | S | kRdMask)) ==
			(I | TST | S);
}

inline bool Assembler::IsCmpRegister(Instr instr)
{
	return (instr & (B27 | B26 | I | kOpCodeMask | S | kRdMask | B4)) ==
			(CMP | S);
}

inline bool Assembler::IsCmpImmediate(Instr instr)
{
	return (instr & (B27 | B26 | I | kOpCodeMask | S | kRdMask)) ==
			(I | CMP | S);
}

inline Register Assembler::GetCmpImmediateRegister(Instr instr)
{
	Q_ASSERT(IsCmpImmediate(instr));
	return GetRn(instr);
}

inline int Assembler::GetCmpImmediateRawImmediate(Instr instr)
{
	Q_ASSERT(IsCmpImmediate(instr));
	return instr & kOff12Mask;
}

inline bool Assembler::ImmediateFitsAddrMode1Instruction(u32 imm32)
{
	u32 dummy1;
	u32 dummy2;
	return fitsShifter(imm32, &dummy1, &dummy2, 0);
}

} // namespace Arm

#endif // ARM_ASSEM_INSTR_H
