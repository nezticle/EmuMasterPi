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

#ifndef ARM_ASSEM_COPROCESSOR_H
#define ARM_ASSEM_COPROCESSOR_H

#include "assembler.h"
#include "utils.h"

namespace Arm {

// Coprocessor instructions.
inline void Assembler::cdp(Coprocessor coproc,
						   int opcode_1,
						   CRegister crd,
						   CRegister crn,
						   CRegister crm,
						   int opcode_2,
						   Condition cond)
{
	Q_ASSERT(isUint4(opcode_1) && isUint3(opcode_2));
	iemit(cond | B27 | B26 | B25 | (opcode_1 & 15)*B20 | crn.code()*B16 |
		 crd.code()*B12 | coproc*B8 | (opcode_2 & 7)*B5 | crm.code());
}


inline void Assembler::cdp2(Coprocessor coproc,
							int opcode_1,
							CRegister crd,
							CRegister crn,
							CRegister crm,
							int opcode_2)
{  // v5 and above
	cdp(coproc, opcode_1, crd, crn, crm, opcode_2, kSpecialCondition);
}


inline void Assembler::mcr(Coprocessor coproc,
						   int opcode_1,
						   Register rd,
						   CRegister crn,
						   CRegister crm,
						   int opcode_2,
						   Condition cond)
{
	Q_ASSERT(isUint3(opcode_1) && isUint3(opcode_2));
	iemit(cond | B27 | B26 | B25 | (opcode_1 & 7)*B21 | crn.code()*B16 |
		 rd.code()*B12 | coproc*B8 | (opcode_2 & 7)*B5 | B4 | crm.code());
}


inline void Assembler::mcr2(Coprocessor coproc,
							int opcode_1,
							Register rd,
							CRegister crn,
							CRegister crm,
							int opcode_2)
{  // v5 and above
	mcr(coproc, opcode_1, rd, crn, crm, opcode_2, kSpecialCondition);
}


inline void Assembler::mrc(Coprocessor coproc,
						   int opcode_1,
						   Register rd,
						   CRegister crn,
						   CRegister crm,
						   int opcode_2,
						   Condition cond)
{
	Q_ASSERT(isUint3(opcode_1) && isUint3(opcode_2));
	iemit(cond | B27 | B26 | B25 | (opcode_1 & 7)*B21 | L | crn.code()*B16 |
		 rd.code()*B12 | coproc*B8 | (opcode_2 & 7)*B5 | B4 | crm.code());
}


inline void Assembler::mrc2(Coprocessor coproc,
							int opcode_1,
							Register rd,
							CRegister crn,
							CRegister crm,
							int opcode_2)
{  // v5 and above
	mrc(coproc, opcode_1, rd, crn, crm, opcode_2, kSpecialCondition);
}


inline void Assembler::ldc(Coprocessor coproc,
						   CRegister crd,
						   const MemOperand& src,
						   LFlag l,
						   Condition cond)
{
	addrmod5(cond | B27 | B26 | l | L | coproc*B8, crd, src);
}


inline void Assembler::ldc(Coprocessor coproc,
						   CRegister crd,
						   Register rn,
						   int option,
						   LFlag l,
						   Condition cond)
{
	// Unindexed addressing.
	Q_ASSERT(isUint8(option));
	iemit(cond | B27 | B26 | U | l | L | rn.code()*B16 | crd.code()*B12 |
		 coproc*B8 | (option & 255));
}


inline void Assembler::ldc2(Coprocessor coproc,
							CRegister crd,
							const MemOperand& src,
							LFlag l)
{  // v5 and above
	ldc(coproc, crd, src, l, kSpecialCondition);
}


inline void Assembler::ldc2(Coprocessor coproc,
							CRegister crd,
							Register rn,
							int option,
							LFlag l)
{  // v5 and above
	ldc(coproc, crd, rn, option, l, kSpecialCondition);
}

} // namespace Arm

#endif // ARM_ASSEM_COPROCESSOR_H
