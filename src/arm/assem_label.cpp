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

#include "assem_label.h"

namespace Arm {

void Assembler::print(Label* L)
{
	if (L->isUnused()) {
		qDebug("unused label");
	} else if (L->isBound()) {
		qDebug("bound label to %d", L->pos());
	} else if (L->isLinked()) {
		Label l = *L;
		qDebug("unbound label");
		while (l.isLinked()) {
			Instr instr = instrAt(l.pos());
			if ((instr & ~kImm24Mask) == 0) {
				qDebug("@ %d value", l.pos());
			} else {
				Q_ASSERT((instr & 7*B25) == 5*B25);  // b, bl, or blx
				Condition cond = Instruction::ConditionField(instr);
				const char* b;
				const char* c;
				if (cond == kSpecialCondition) {
					b = "blx";
					c = "";
				} else {
					if ((instr & B24) != 0)
						b = "bl";
					else
						b = "b";

					switch (cond) {
					case eq: c = "eq"; break;
					case ne: c = "ne"; break;
					case hs: c = "hs"; break;
					case lo: c = "lo"; break;
					case mi: c = "mi"; break;
					case pl: c = "pl"; break;
					case vs: c = "vs"; break;
					case vc: c = "vc"; break;
					case hi: c = "hi"; break;
					case ls: c = "ls"; break;
					case ge: c = "ge"; break;
					case lt: c = "lt"; break;
					case gt: c = "gt"; break;
					case le: c = "le"; break;
					case al: c = ""; break;
					default:
						c = "";
						UNREACHABLE();
					}
				}
				qDebug("@ %d %s%s\n", l.pos(), b, c);
			}
			next(&l);
		}
	} else {
		qDebug("label in inconsistent state (pos = %d)\n", L->pos_);
	}
}

} // namespace Arm
