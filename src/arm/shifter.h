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

#ifndef ARM_SHIFTER_H
#define ARM_SHIFTER_H

#include "register.h"

namespace Arm {

// If this returns true then you have to use the rotate_imm and immed_8
// that it returns, because it may have already changed the instruction
// to match them!
static inline bool fitsShifter(u32 imm32,
							   u32* rotate_imm,
							   u32* immed_8,
							   Instr* instr) {
	// imm32 must be unsigned.
	for (int rot = 0; rot < 16; rot++) {
		u32 imm8 = (imm32 << 2*rot) | (imm32 >> (32 - 2*rot));
		if ((imm8 <= 0xff)) {
			*rotate_imm = rot;
			*immed_8 = imm8;
			return true;
		}
	}
	// If the opcode is one with a complementary version and the complementary
	// immediate fits, change the opcode.
	if (instr) {
		if ((*instr & kMovMvnMask) == kMovMvnPattern) {
			if (fitsShifter(~imm32, rotate_imm, immed_8, 0)) {
				*instr ^= kMovMvnFlip;
				return true;
			} else if ((*instr & kMovLeaveCCMask) == kMovLeaveCCPattern) {
#if defined(CAN_USE_ARMV7_INSTRUCTIONS)
				if (imm32 < 0x10000) {
					*instr ^= kMovwLeaveCCFlip;
					*instr |= EncodeMovwImmediate(imm32);
					*rotate_imm = *immed_8 = 0;  // Not used for movw.
					return true;
				}
#endif
			}
		} else if ((*instr & kCmpCmnMask) == kCmpCmnPattern) {
			if (fitsShifter(-imm32, rotate_imm, immed_8, 0)) {
				*instr ^= kCmpCmnFlip;
				return true;
			}
		} else {
			Instr alu_insn = (*instr & kALUMask);
			if (alu_insn == ADD ||
				alu_insn == SUB) {
				if (fitsShifter(-imm32, rotate_imm, immed_8, 0)) {
					*instr ^= kAddSubFlip;
					return true;
				}
			} else if (alu_insn == AND ||
					   alu_insn == BIC) {
				if (fitsShifter(~imm32, rotate_imm, immed_8, 0)) {
					*instr ^= kAndBicFlip;
					return true;
				}
			}
		}
	}
	return false;
}

} // namespace Arm

#endif // ARM_SHIFTER_H
