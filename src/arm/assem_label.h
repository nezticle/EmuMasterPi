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

#ifndef ARM_ASSEM_LABEL_H
#define ARM_ASSEM_LABEL_H

#include "assem_raw.h"

namespace Arm {

// Labels refer to positions in the (to be) generated code.
// There are bound, linked, and unused labels.
//
// Bound labels refer to known positions in the already
// generated code. pos() is the position the label refers to.
//
// Linked labels refer to unknown positions in the code
// to be generated; pos() is the position of the last
// instruction using the label.

// The link chain is terminated by a negative code position (must be aligned)
static const int kEndOfChain = -4;

inline int Assembler::targetAt(int pos)
{
	Instr instr = instrAt(pos);
	Q_ASSERT((instr & 7*B25) == 5*B25);  // b, bl, or blx imm24
	int imm26 = ((instr & kImm24Mask) << 8) >> 6;
	if ((Instruction::ConditionField(instr) == kSpecialCondition) &&
		((instr & B24) != 0)) {
		// blx uses bit 24 to encode bit 2 of imm26
		imm26 += 2;
	}
	return pos + kPcLoadDelta + imm26;
}

inline void Assembler::putTargetAt(int pos, int target_pos)
{
	Instr instr = instrAt(pos);
	int imm26 = target_pos - (pos + kPcLoadDelta);
	Q_ASSERT((instr & 7*B25) == 5*B25);  // b, bl, or blx imm24
	if (Instruction::ConditionField(instr) == kSpecialCondition) {
		// blx uses bit 24 to encode bit 2 of imm26
		Q_ASSERT((imm26 & 1) == 0);
		instr = (instr & ~(B24 | kImm24Mask)) | ((imm26 & 2) >> 1)*B24;
	} else {
		Q_ASSERT((imm26 & 3) == 0);
		instr &= ~kImm24Mask;
	}
	int imm24 = imm26 >> 2;
	Q_ASSERT(isInt24(imm24));
	putInstrAt(pos, instr | (imm24 & kImm24Mask));
}

inline void Assembler::bind(Label* L)
{
	Q_ASSERT(!L->isBound());  // label can only be bound once
	bindTo(L, pcOffset());
}

inline void Assembler::bindTo(Label* L, int pos)
{
	Q_ASSERT(0 <= pos && pos <= pcOffset());  // must have a valid binding position
	while (L->isLinked()) {
		int fixup_pos = L->pos();
		next(L);  // call next before overwriting link with target at fixup_pos
		putTargetAt(fixup_pos, pos);
	}
	L->bindTo(pos);
}

inline void Assembler::linkTo(Label* L, Label* appendix)
{
	if (appendix->isLinked()) {
		if (L->isLinked()) {
			// Append appendix to L's list.
			int fixup_pos;
			int link = L->pos();
			do {
				fixup_pos = link;
				link = targetAt(fixup_pos);
			} while (link > 0);
			Q_ASSERT(link == kEndOfChain);
			putTargetAt(fixup_pos, appendix->pos());
		} else {
			// L is empty, simply use appendix.
			*L = *appendix;
		}
	}
	appendix->unuse();  // appendix should not be used anymore
}

inline void Assembler::next(Label* L)
{
	Q_ASSERT(L->isLinked());
	int link = targetAt(L->pos());
	if (link == kEndOfChain) {
		L->unuse();
	} else {
		Q_ASSERT(link >= 0);
		L->linkTo(link);
	}
}

inline int Assembler::branchOffset(Label* L, bool jumpEliminationAllowed)
{
	Q_UNUSED(jumpEliminationAllowed)
	int target_pos;
	if (L->isBound()) {
		target_pos = L->pos();
	} else {
		if (L->isLinked()) {
			target_pos = L->pos();  // L's link
		} else {
			target_pos = kEndOfChain;
		}
		L->linkTo(pcOffset());
	}

	// Block the emission of the constant pool, since the branch instruction must
	// be emitted at the pc offset recorded by the label.
	constPoolBlockFor(1);
	return target_pos - (pcOffset() + kPcLoadDelta);
}

} // namespace Arm

#endif // ARM_ASSEM_LABEL_H
