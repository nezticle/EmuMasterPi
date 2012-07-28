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

#ifndef ARM_LABEL_H
#define ARM_LABEL_H

#include "constants.h"

namespace Arm {

// -----------------------------------------------------------------------------
// Labels represent pc locations; they are typically jump or call targets.
// After declaration, a label can be freely used to denote known or (yet)
// unknown pc location. Assembler::bind() is used to bind a label to the
// current pc. A label can be bound only once.

class Label BASE_EMBEDDED
{
public:
	Label() { unuse(); }
	~Label() { Q_ASSERT(!isLinked()); }

	void unuse() { pos_ = 0; }

	bool isBound() const { return pos_ <  0; }
	bool isUnused() const { return pos_ == 0; }
	bool isLinked() const { return pos_ >  0; }

	// Returns the position of bound or linked labels. Cannot be used
	// for unused labels.
	int pos() const;

	bool operator ==(const Label &l) const { return pos_ == l.pos_; }
	bool operator !=(const Label &l) const { return pos_ != l.pos_; }
private:
	// pos_ encodes both the binding state (via its sign)
	// and the binding position (via its value) of a label.
	//
	// pos_ <  0  bound label, pos() returns the jump target position
	// pos_ == 0  unused label
	// pos_ >  0  linked label, pos() returns the last reference position
	int pos_;

	void bindTo(int pos);
	void linkTo(int pos);

	friend class Assembler;
	friend class Displacement;
};

inline int Label::pos() const
{
	Q_ASSERT(!isUnused());
	if (pos_ < 0)
		return -pos_ - 1;
	if (pos_ > 0)
		return  pos_ - 1;
	return 0;
}

inline void Label::bindTo(int pos)
{
	pos_ = -pos - 1;
	Q_ASSERT(isBound());
}

inline void Label::linkTo(int pos)
{
	pos_ = pos + 1;
	Q_ASSERT(isLinked());
}

} // namespace Arm

#endif // ARM_LABEL_H
