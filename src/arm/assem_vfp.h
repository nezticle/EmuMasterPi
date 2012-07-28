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

#ifndef ARM_ASSEM_VFP_H
#define ARM_ASSEM_VFP_H

#include "assembler.h"

namespace Arm {

// Support for VFP.

#if defined(CAN_USE_VFP_INSTRUCTIONS)
inline void Assembler::vldr(const DwVfpRegister dst,
							const Register base,
							int offset,
							const Condition cond)
{
	// Ddst = MEM(Rbase + offset).
	// Instruction details available in ARM DDI 0406A, A8-628.
	// cond(31-28) | 1101(27-24)| U001(23-20) | Rbase(19-16) |
	// Vdst(15-12) | 1011(11-8) | offset
	int u = 1;
	if (offset < 0) {
		offset = -offset;
		u = 0;
	}

	Q_ASSERT(offset >= 0);
	if ((offset % 4) == 0 && (offset / 4) < 256) {
		iemit(cond | u*B23 | 0xD1*B20 | base.code()*B16 | dst.code()*B12 |
			  0xB*B8 | ((offset / 4) & 255));
	} else {
		// Larger offsets must be handled by computing the correct address
		// in the ip register.
		Q_ASSERT(!base.is(ip));
		if (u == 1) {
			add(ip, base, Operand(offset));
		} else {
			sub(ip, base, Operand(offset));
		}
		iemit(cond | 0xD1*B20 | ip.code()*B16 | dst.code()*B12 | 0xB*B8);
	}
}


inline void Assembler::vldr(const DwVfpRegister dst,
							const MemOperand& operand,
							const Condition cond)
{
	Q_ASSERT(!operand.rm().isValid());
	Q_ASSERT(operand.am_ == Offset);
	vldr(dst, operand.rn(), operand.offset(), cond);
}


inline void Assembler::vldr(const SwVfpRegister dst,
							const Register base,
							int offset,
							const Condition cond)
{
	// Sdst = MEM(Rbase + offset).
	// Instruction details available in ARM DDI 0406A, A8-628.
	// cond(31-28) | 1101(27-24)| U001(23-20) | Rbase(19-16) |
	// Vdst(15-12) | 1010(11-8) | offset
	int u = 1;
	if (offset < 0) {
		offset = -offset;
		u = 0;
	}
	int sd, d;
	dst.splitCode(&sd, &d);
	Q_ASSERT(offset >= 0);

	if ((offset % 4) == 0 && (offset / 4) < 256) {
		iemit(cond | u*B23 | d*B22 | 0xD1*B20 | base.code()*B16 | sd*B12 |
			  0xA*B8 | ((offset / 4) & 255));
	} else {
		// Larger offsets must be handled by computing the correct address
		// in the ip register.
		Q_ASSERT(!base.is(ip));
		if (u == 1) {
			add(ip, base, Operand(offset));
		} else {
			sub(ip, base, Operand(offset));
		}
		iemit(cond | d*B22 | 0xD1*B20 | ip.code()*B16 | sd*B12 | 0xA*B8);
	}
}


inline void Assembler::vldr(const SwVfpRegister dst,
							const MemOperand& operand,
							const Condition cond)
{
	Q_ASSERT(!operand.rm().isValid());
	Q_ASSERT(operand.am_ == Offset);
	vldr(dst, operand.rn(), operand.offset(), cond);
}


inline void Assembler::vstr(const DwVfpRegister src,
							const Register base,
							int offset,
							const Condition cond)
{
	// MEM(Rbase + offset) = Dsrc.
	// Instruction details available in ARM DDI 0406A, A8-786.
	// cond(31-28) | 1101(27-24)| U000(23-20) | | Rbase(19-16) |
	// Vsrc(15-12) | 1011(11-8) | (offset/4)
	int u = 1;
	if (offset < 0) {
		offset = -offset;
		u = 0;
	}
	Q_ASSERT(offset >= 0);
	if ((offset % 4) == 0 && (offset / 4) < 256) {
		iemit(cond | u*B23 | 0xD0*B20 | base.code()*B16 | src.code()*B12 |
			  0xB*B8 | ((offset / 4) & 255));
	} else {
		// Larger offsets must be handled by computing the correct address
		// in the ip register.
		Q_ASSERT(!base.is(ip));
		if (u == 1) {
			add(ip, base, Operand(offset));
		} else {
			sub(ip, base, Operand(offset));
		}
		iemit(cond | 0xD0*B20 | ip.code()*B16 | src.code()*B12 | 0xB*B8);
	}
}


inline void Assembler::vstr(const DwVfpRegister src,
							const MemOperand& operand,
							const Condition cond)
{
	Q_ASSERT(!operand.rm().isValid());
	Q_ASSERT(operand.am_ == Offset);
	vstr(src, operand.rn(), operand.offset(), cond);
}


inline void Assembler::vstr(const SwVfpRegister src,
							const Register base,
							int offset,
							const Condition cond)
{
	// MEM(Rbase + offset) = SSrc.
	// Instruction details available in ARM DDI 0406A, A8-786.
	// cond(31-28) | 1101(27-24)| U000(23-20) | Rbase(19-16) |
	// Vdst(15-12) | 1010(11-8) | (offset/4)
	int u = 1;
	if (offset < 0) {
		offset = -offset;
		u = 0;
	}
	int sd, d;
	src.splitCode(&sd, &d);
	Q_ASSERT(offset >= 0);
	if ((offset % 4) == 0 && (offset / 4) < 256) {
		iemit(cond | u*B23 | d*B22 | 0xD0*B20 | base.code()*B16 | sd*B12 |
			  0xA*B8 | ((offset / 4) & 255));
	} else {
		// Larger offsets must be handled by computing the correct address
		// in the ip register.
		Q_ASSERT(!base.is(ip));
		if (u == 1) {
			add(ip, base, Operand(offset));
		} else {
			sub(ip, base, Operand(offset));
		}
		iemit(cond | d*B22 | 0xD0*B20 | ip.code()*B16 | sd*B12 | 0xA*B8);
	}
}


inline void Assembler::vstr(const SwVfpRegister src,
							const MemOperand& operand,
							const Condition cond)
{
	Q_ASSERT(!operand.rm().isValid());
	Q_ASSERT(operand.am_ == Offset);
	vldr(src, operand.rn(), operand.offset(), cond);
}


inline void  Assembler::vldm(BlockAddrMode am,
							 Register base,
							 DwVfpRegister first,
							 DwVfpRegister last,
							 Condition cond)
{
	// Instruction details available in ARM DDI 0406A, A8-626.
	// cond(31-28) | 110(27-25)| PUDW1(24-20) | Rbase(19-16) |
	// first(15-12) | 1010(11-8) | (count * 2)
	Q_ASSERT_LE(first.code(), last.code());
	Q_ASSERT(am == ia || am == ia_w || am == db_w);
	Q_ASSERT(!base.is(pc));

	int sd, d;
	first.splitCode(&sd, &d);
	int count = last.code() - first.code() + 1;
	iemit(cond | B27 | B26 | am | d*B22 | B20 | base.code()*B16 | sd*B12 |
		  0xB*B8 | count*2);
}


inline void  Assembler::vstm(BlockAddrMode am,
							 Register base,
							 DwVfpRegister first,
							 DwVfpRegister last,
							 Condition cond)
{
	// Instruction details available in ARM DDI 0406A, A8-784.
	// cond(31-28) | 110(27-25)| PUDW0(24-20) | Rbase(19-16) |
	// first(15-12) | 1011(11-8) | (count * 2)
	Q_ASSERT_LE(first.code(), last.code());
	Q_ASSERT(am == ia || am == ia_w || am == db_w);
	Q_ASSERT(!base.is(pc));

	int sd, d;
	first.splitCode(&sd, &d);
	int count = last.code() - first.code() + 1;
	iemit(cond | B27 | B26 | am | d*B22 | base.code()*B16 | sd*B12 |
		  0xB*B8 | count*2);
}

inline void  Assembler::vldm(BlockAddrMode am,
							 Register base,
							 SwVfpRegister first,
							 SwVfpRegister last,
							 Condition cond)
{
	// Instruction details available in ARM DDI 0406A, A8-626.
	// cond(31-28) | 110(27-25)| PUDW1(24-20) | Rbase(19-16) |
	// first(15-12) | 1010(11-8) | (count/2)
	Q_ASSERT_LE(first.code(), last.code());
	Q_ASSERT(am == ia || am == ia_w || am == db_w);
	Q_ASSERT(!base.is(pc));

	int sd, d;
	first.splitCode(&sd, &d);
	int count = last.code() - first.code() + 1;
	iemit(cond | B27 | B26 | am | d*B22 | B20 | base.code()*B16 | sd*B12 |
		  0xA*B8 | count);
}


inline void  Assembler::vstm(BlockAddrMode am,
							 Register base,
							 SwVfpRegister first,
							 SwVfpRegister last,
							 Condition cond)
{
	// Instruction details available in ARM DDI 0406A, A8-784.
	// cond(31-28) | 110(27-25)| PUDW0(24-20) | Rbase(19-16) |
	// first(15-12) | 1011(11-8) | (count/2)
	Q_ASSERT_LE(first.code(), last.code());
	Q_ASSERT(am == ia || am == ia_w || am == db_w);
	Q_ASSERT(!base.is(pc));

	int sd, d;
	first.splitCode(&sd, &d);
	int count = last.code() - first.code() + 1;
	iemit(cond | B27 | B26 | am | d*B22 | base.code()*B16 | sd*B12 |
		  0xA*B8 | count);
}

static inline void DoubleAsTwoUInt32(double d, u32* lo, u32* hi)
{
	u64 i;
	memcpy(&i, &d, 8);

	*lo = i & 0xffffffff;
	*hi = i >> 32;
}

// Only works for little endian floating point formats.
// We don't support VFP on the mixed endian floating point platform.
static inline bool FitsVMOVDoubleImmediate(double d, u32 *encoding)
{
	// VMOV can accept an immediate of the form:
	//
	//  +/- m * 2^(-n) where 16 <= m <= 31 and 0 <= n <= 7
	//
	// The immediate is encoded using an 8-bit quantity, comprised of two
	// 4-bit fields. For an 8-bit immediate of the form:
	//
	//  [abcdefgh]
	//
	// where a is the MSB and h is the LSB, an immediate 64-bit double can be
	// created of the form:
	//
	//  [aBbbbbbb,bbcdefgh,00000000,00000000,
	//      00000000,00000000,00000000,00000000]
	//
	// where B = ~b.
	//

	u32 lo, hi;
	DoubleAsTwoUInt32(d, &lo, &hi);

	// The most obvious constraint is the long block of zeroes.
	if ((lo != 0) || ((hi & 0xffff) != 0)) {
		return false;
	}

	// Bits 62:55 must be all clear or all set.
	if (((hi & 0x3fc00000) != 0) && ((hi & 0x3fc00000) != 0x3fc00000)) {
		return false;
	}

	// Bit 63 must be NOT bit 62.
	if (((hi ^ (hi << 1)) & (0x40000000)) == 0) {
		return false;
	}

	// Create the encoded immediate in the form:
	//  [00000000,0000abcd,00000000,0000efgh]
	*encoding  = (hi >> 16) & 0xf;      // Low nybble.
	*encoding |= (hi >> 4) & 0x70000;   // Low three bits of the high nybble.
	*encoding |= (hi >> 12) & 0x80000;  // Top bit of the high nybble.

	return true;
}


inline void Assembler::vmov(const DwVfpRegister dst,
							double imm,
							const Condition cond)
{
	// Dd = immediate
	// Instruction details available in ARM DDI 0406B, A8-640.
	u32 enc;
	if (FitsVMOVDoubleImmediate(imm, &enc)) {
		// The double can be encoded in the instruction.
		iemit(cond | 0xE*B24 | 0xB*B20 | dst.code()*B12 | 0xB*B8 | enc);
	} else {
		// Synthesise the double from ARM immediates. This could be implemented
		// using vldr from a constant pool.
		u32 lo, hi;
		DoubleAsTwoUInt32(imm, &lo, &hi);

		if (lo == hi) {
			// If the lo and hi parts of the double are equal, the literal is easier
			// to create. This is the case with 0.0.
			mov(ip, Operand(lo));
			vmov(dst, ip, ip);
		} else {
			// Move the low part of the double into the lower of the corresponsing S
			// registers of D register dst.
			mov(ip, Operand(lo));
			vmov(dst.firstSingle(), ip, cond);

			// Move the high part of the double into the higher of the corresponsing S
			// registers of D register dst.
			mov(ip, Operand(hi));
			vmov(dst.secondSingle(), ip, cond);
		}
	}
}


inline void Assembler::vmov(const SwVfpRegister dst,
							const SwVfpRegister src,
							const Condition cond)
{
	// Sd = Sm
	// Instruction details available in ARM DDI 0406B, A8-642.
	int sd, d, sm, m;
	dst.splitCode(&sd, &d);
	src.splitCode(&sm, &m);
	iemit(cond | 0xE*B24 | d*B22 | 0xB*B20 | sd*B12 | 0xA*B8 | B6 | m*B5 | sm);
}


inline void Assembler::vmov(const DwVfpRegister dst,
							const DwVfpRegister src,
							const Condition cond)
{
	// Dd = Dm
	// Instruction details available in ARM DDI 0406B, A8-642.
	iemit(cond | 0xE*B24 | 0xB*B20 |
		  dst.code()*B12 | 0x5*B9 | B8 | B6 | src.code());
}


inline void Assembler::vmov(const DwVfpRegister dst,
							const Register src1,
							const Register src2,
							const Condition cond)
{
	// Dm = <Rt,Rt2>.
	// Instruction details available in ARM DDI 0406A, A8-646.
	// cond(31-28) | 1100(27-24)| 010(23-21) | op=0(20) | Rt2(19-16) |
	// Rt(15-12) | 1011(11-8) | 00(7-6) | M(5) | 1(4) | Vm
	Q_ASSERT(!src1.is(pc) && !src2.is(pc));
	iemit(cond | 0xC*B24 | B22 | src2.code()*B16 |
		  src1.code()*B12 | 0xB*B8 | B4 | dst.code());
}


inline void Assembler::vmov(const Register dst1,
							const Register dst2,
							const DwVfpRegister src,
							const Condition cond)
{
	// <Rt,Rt2> = Dm.
	// Instruction details available in ARM DDI 0406A, A8-646.
	// cond(31-28) | 1100(27-24)| 010(23-21) | op=1(20) | Rt2(19-16) |
	// Rt(15-12) | 1011(11-8) | 00(7-6) | M(5) | 1(4) | Vm
	Q_ASSERT(!dst1.is(pc) && !dst2.is(pc));
	iemit(cond | 0xC*B24 | B22 | B20 | dst2.code()*B16 |
		  dst1.code()*B12 | 0xB*B8 | B4 | src.code());
}


inline void Assembler::vmov(const SwVfpRegister dst,
							const Register src,
							const Condition cond)
{
	// Sn = Rt.
	// Instruction details available in ARM DDI 0406A, A8-642.
	// cond(31-28) | 1110(27-24)| 000(23-21) | op=0(20) | Vn(19-16) |
	// Rt(15-12) | 1010(11-8) | N(7)=0 | 00(6-5) | 1(4) | 0000(3-0)
	Q_ASSERT(!src.is(pc));
	int sn, n;
	dst.splitCode(&sn, &n);
	iemit(cond | 0xE*B24 | sn*B16 | src.code()*B12 | 0xA*B8 | n*B7 | B4);
}


inline void Assembler::vmov(const Register dst,
							const SwVfpRegister src,
							const Condition cond)
{
	// Rt = Sn.
	// Instruction details available in ARM DDI 0406A, A8-642.
	// cond(31-28) | 1110(27-24)| 000(23-21) | op=1(20) | Vn(19-16) |
	// Rt(15-12) | 1010(11-8) | N(7)=0 | 00(6-5) | 1(4) | 0000(3-0)
	Q_ASSERT(!dst.is(pc));
	int sn, n;
	src.splitCode(&sn, &n);
	iemit(cond | 0xE*B24 | B20 | sn*B16 | dst.code()*B12 | 0xA*B8 | n*B7 | B4);
}


// Type of data to read from or write to VFP register.
// Used as specifier in generic vcvt instruction.
enum VFPType { S32, U32, F32, F64 };


static inline bool IsSignedVFPType(VFPType type)
{
	switch (type) {
	case S32:
		return true;
	case U32:
		return false;
	default:
		UNREACHABLE();
		return false;
	}
}


static inline bool IsIntegerVFPType(VFPType type)
{
	switch (type) {
	case S32:
	case U32:
		return true;
	case F32:
	case F64:
		return false;
	default:
		UNREACHABLE();
		return false;
	}
}


static inline bool IsDoubleVFPType(VFPType type)
{
	switch (type) {
	case F32:
		return false;
	case F64:
		return true;
	default:
		UNREACHABLE();
		return false;
	}
}


// Split five bit reg_code based on size of reg_type.
//  32-bit register codes are Vm:M
//  64-bit register codes are M:Vm
// where Vm is four bits, and M is a single bit.
static inline void SplitRegCode(VFPType reg_type,
								int reg_code,
								int* vm,
								int* m)
{
	Q_ASSERT((reg_code >= 0) && (reg_code <= 31));
	if (IsIntegerVFPType(reg_type) || !IsDoubleVFPType(reg_type)) {
		// 32 bit type.
		*m  = reg_code & 0x1;
		*vm = reg_code >> 1;
	} else {
		// 64 bit type.
		*m  = (reg_code & 0x10) >> 4;
		*vm = reg_code & 0x0F;
	}
}


// Encode vcvt.src_type.dst_type instruction.
static inline Instr EncodeVCVT(const VFPType dst_type,
							   const int dst_code,
							   const VFPType src_type,
							   const int src_code,
							   VFPConversionMode mode,
							   const Condition cond)
{
	Q_ASSERT(src_type != dst_type);
	int D, Vd, M, Vm;
	SplitRegCode(src_type, src_code, &Vm, &M);
	SplitRegCode(dst_type, dst_code, &Vd, &D);

	if (IsIntegerVFPType(dst_type) || IsIntegerVFPType(src_type)) {
		// Conversion between IEEE floating point and 32-bit integer.
		// Instruction details available in ARM DDI 0406B, A8.6.295.
		// cond(31-28) | 11101(27-23)| D(22) | 11(21-20) | 1(19) | opc2(18-16) |
		// Vd(15-12) | 101(11-9) | sz(8) | op(7) | 1(6) | M(5) | 0(4) | Vm(3-0)
		Q_ASSERT(!IsIntegerVFPType(dst_type) || !IsIntegerVFPType(src_type));

		int sz, opc2, op;

		if (IsIntegerVFPType(dst_type)) {
			opc2 = IsSignedVFPType(dst_type) ? 0x5 : 0x4;
			sz = IsDoubleVFPType(src_type) ? 0x1 : 0x0;
			op = mode;
		} else {
			Q_ASSERT(IsIntegerVFPType(src_type));
			opc2 = 0x0;
			sz = IsDoubleVFPType(dst_type) ? 0x1 : 0x0;
			op = IsSignedVFPType(src_type) ? 0x1 : 0x0;
		}

		return (cond | 0xE*B24 | B23 | D*B22 | 0x3*B20 | B19 | opc2*B16 |
				Vd*B12 | 0x5*B9 | sz*B8 | op*B7 | B6 | M*B5 | Vm);
	} else {
		// Conversion between IEEE double and single precision.
		// Instruction details available in ARM DDI 0406B, A8.6.298.
		// cond(31-28) | 11101(27-23)| D(22) | 11(21-20) | 0111(19-16) |
		// Vd(15-12) | 101(11-9) | sz(8) | 1(7) | 1(6) | M(5) | 0(4) | Vm(3-0)
		int sz = IsDoubleVFPType(src_type) ? 0x1 : 0x0;
		return (cond | 0xE*B24 | B23 | D*B22 | 0x3*B20 | 0x7*B16 |
				Vd*B12 | 0x5*B9 | sz*B8 | B7 | B6 | M*B5 | Vm);
	}
}


inline void Assembler::vcvt_f64_s32(const DwVfpRegister dst,
									const SwVfpRegister src,
									VFPConversionMode mode,
									const Condition cond)
{
	iemit(EncodeVCVT(F64, dst.code(), S32, src.code(), mode, cond));
}


inline void Assembler::vcvt_f32_s32(const SwVfpRegister dst,
									const SwVfpRegister src,
									VFPConversionMode mode,
									const Condition cond)
{
	iemit(EncodeVCVT(F32, dst.code(), S32, src.code(), mode, cond));
}


inline void Assembler::vcvt_f64_u32(const DwVfpRegister dst,
									const SwVfpRegister src,
									VFPConversionMode mode,
									const Condition cond)
{
	iemit(EncodeVCVT(F64, dst.code(), U32, src.code(), mode, cond));
}


inline void Assembler::vcvt_s32_f64(const SwVfpRegister dst,
									const DwVfpRegister src,
									VFPConversionMode mode,
									const Condition cond)
{
	iemit(EncodeVCVT(S32, dst.code(), F64, src.code(), mode, cond));
}


inline void Assembler::vcvt_u32_f64(const SwVfpRegister dst,
									const DwVfpRegister src,
									VFPConversionMode mode,
									const Condition cond)
{
	iemit(EncodeVCVT(U32, dst.code(), F64, src.code(), mode, cond));
}


inline void Assembler::vcvt_f64_f32(const DwVfpRegister dst,
									const SwVfpRegister src,
									VFPConversionMode mode,
									const Condition cond)
{
	iemit(EncodeVCVT(F64, dst.code(), F32, src.code(), mode, cond));
}


inline void Assembler::vcvt_f32_f64(const SwVfpRegister dst,
									const DwVfpRegister src,
									VFPConversionMode mode,
									const Condition cond)
{
	iemit(EncodeVCVT(F32, dst.code(), F64, src.code(), mode, cond));
}


inline void Assembler::vneg(const DwVfpRegister dst,
							const DwVfpRegister src,
							const Condition cond)
{
	iemit(cond | 0xE*B24 | 0xB*B20 | B16 | dst.code()*B12 |
		  0x5*B9 | B8 | B6 | src.code());
}


inline void Assembler::vabs(const DwVfpRegister dst,
							const DwVfpRegister src,
							const Condition cond)
{
	iemit(cond | 0xE*B24 | 0xB*B20 | dst.code()*B12 |
		  0x5*B9 | B8 | 0x3*B6 | src.code());
}


inline void Assembler::vadd(const DwVfpRegister dst,
							const DwVfpRegister src1,
							const DwVfpRegister src2,
							const Condition cond)
{
	// Dd = vadd(Dn, Dm) double precision floating point addition.
	// Dd = D:Vd; Dm=M:Vm; Dn=N:Vm.
	// Instruction details available in ARM DDI 0406A, A8-536.
	// cond(31-28) | 11100(27-23)| D=?(22) | 11(21-20) | Vn(19-16) |
	// Vd(15-12) | 101(11-9) | sz(8)=1 | N(7)=0 | 0(6) | M=?(5) | 0(4) | Vm(3-0)
	iemit(cond | 0xE*B24 | 0x3*B20 | src1.code()*B16 |
		  dst.code()*B12 | 0x5*B9 | B8 | src2.code());
}


inline void Assembler::vsub(const DwVfpRegister dst,
							const DwVfpRegister src1,
							const DwVfpRegister src2,
							const Condition cond)
{
	// Dd = vsub(Dn, Dm) double precision floating point subtraction.
	// Dd = D:Vd; Dm=M:Vm; Dn=N:Vm.
	// Instruction details available in ARM DDI 0406A, A8-784.
	// cond(31-28) | 11100(27-23)| D=?(22) | 11(21-20) | Vn(19-16) |
	// Vd(15-12) | 101(11-9) | sz(8)=1 | N(7)=0 | 1(6) | M=?(5) | 0(4) | Vm(3-0)
	iemit(cond | 0xE*B24 | 0x3*B20 | src1.code()*B16 |
		  dst.code()*B12 | 0x5*B9 | B8 | B6 | src2.code());
}


inline void Assembler::vmul(const DwVfpRegister dst,
							const DwVfpRegister src1,
							const DwVfpRegister src2,
							const Condition cond)
{
	// Dd = vmul(Dn, Dm) double precision floating point multiplication.
	// Dd = D:Vd; Dm=M:Vm; Dn=N:Vm.
	// Instruction details available in ARM DDI 0406A, A8-784.
	// cond(31-28) | 11100(27-23)| D=?(22) | 10(21-20) | Vn(19-16) |
	// Vd(15-12) | 101(11-9) | sz(8)=1 | N(7)=0 | 0(6) | M=?(5) | 0(4) | Vm(3-0)
	iemit(cond | 0xE*B24 | 0x2*B20 | src1.code()*B16 |
		  dst.code()*B12 | 0x5*B9 | B8 | src2.code());
}


inline void Assembler::vdiv(const DwVfpRegister dst,
							const DwVfpRegister src1,
							const DwVfpRegister src2,
							const Condition cond)
{
	// Dd = vdiv(Dn, Dm) double precision floating point division.
	// Dd = D:Vd; Dm=M:Vm; Dn=N:Vm.
	// Instruction details available in ARM DDI 0406A, A8-584.
	// cond(31-28) | 11101(27-23)| D=?(22) | 00(21-20) | Vn(19-16) |
	// Vd(15-12) | 101(11-9) | sz(8)=1 | N(7)=? | 0(6) | M=?(5) | 0(4) | Vm(3-0)
	iemit(cond | 0xE*B24 | B23 | src1.code()*B16 |
		  dst.code()*B12 | 0x5*B9 | B8 | src2.code());
}


inline void Assembler::vcmp(const DwVfpRegister src1,
							const DwVfpRegister src2,
							const Condition cond)
{
	// vcmp(Dd, Dm) double precision floating point comparison.
	// Instruction details available in ARM DDI 0406A, A8-570.
	// cond(31-28) | 11101 (27-23)| D=?(22) | 11 (21-20) | 0100 (19-16) |
	// Vd(15-12) | 101(11-9) | sz(8)=1 | E(7)=0 | 1(6) | M(5)=? | 0(4) | Vm(3-0)
	iemit(cond | 0xE*B24 |B23 | 0x3*B20 | B18 |
		  src1.code()*B12 | 0x5*B9 | B8 | B6 | src2.code());
}


inline void Assembler::vcmp(const DwVfpRegister src1,
							const double src2,
							const Condition cond)
{
	// vcmp(Dd, Dm) double precision floating point comparison.
	// Instruction details available in ARM DDI 0406A, A8-570.
	// cond(31-28) | 11101 (27-23)| D=?(22) | 11 (21-20) | 0101 (19-16) |
	// Vd(15-12) | 101(11-9) | sz(8)=1 | E(7)=0 | 1(6) | M(5)=? | 0(4) | 0000(3-0)
	Q_ASSERT(src2 == 0.0);
	Q_UNUSED(src2)
	iemit(cond | 0xE*B24 |B23 | 0x3*B20 | B18 | B16 |
		  src1.code()*B12 | 0x5*B9 | B8 | B6);
}


inline void Assembler::vmsr(Register dst, Condition cond)
{
	// Instruction details available in ARM DDI 0406A, A8-652.
	// cond(31-28) | 1110 (27-24) | 1110(23-20)| 0001 (19-16) |
	// Rt(15-12) | 1010 (11-8) | 0(7) | 00 (6-5) | 1(4) | 0000(3-0)
	iemit(cond | 0xE*B24 | 0xE*B20 |  B16 |
		  dst.code()*B12 | 0xA*B8 | B4);
}


inline void Assembler::vmrs(Register dst, Condition cond)
{
	// Instruction details available in ARM DDI 0406A, A8-652.
	// cond(31-28) | 1110 (27-24) | 1111(23-20)| 0001 (19-16) |
	// Rt(15-12) | 1010 (11-8) | 0(7) | 00 (6-5) | 1(4) | 0000(3-0)
	iemit(cond | 0xE*B24 | 0xF*B20 |  B16 |
		  dst.code()*B12 | 0xA*B8 | B4);
}


inline void Assembler::vsqrt(const DwVfpRegister dst,
							 const DwVfpRegister src,
							 const Condition cond)
{
	// cond(31-28) | 11101 (27-23)| D=?(22) | 11 (21-20) | 0001 (19-16) |
	// Vd(15-12) | 101(11-9) | sz(8)=1 | 11 (7-6) | M(5)=? | 0(4) | Vm(3-0)
	iemit(cond | 0xE*B24 | B23 | 0x3*B20 | B16 |
		  dst.code()*B12 | 0x5*B9 | B8 | 3*B6 | src.code());
}

#endif // defined(CAN_USE_VFP_INSTRUCTIONS)

} // namespace Arm

#endif // ARM_ASSEM_VFP_H
