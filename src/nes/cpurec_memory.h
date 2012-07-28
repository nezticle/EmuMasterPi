/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef NESCPUREC_MEMORY_H
#define NESCPUREC_MEMORY_H

#include "cpurec_p.h"

inline void NesCpuTranslator::mRead8(Register addr, Register dst, RegList preserve)
{
	Q_ASSERT(!(preserve & ~0xf)); // allow preserving only r0-r3
	Q_ASSERT(!dst.isValid() || !(preserve & dst.bit())); // preserve will not work for the result

	RegList preserved = preserve | ip.bit();
	__ mrs(ip, CPSR);
	__ stm(db_w, sp, preserved);

	if (addr != r0)
		__ mov(r0, addr);
	mCallCFunction(offsetof(NesCpuRecData,cpuRead));
	if (dst.isValid() && dst != r0)
		__ mov(dst, r0);

	__ ldm(ia_w, sp, preserved);
	__ msr(CPSR_f, Operand(ip));
}

inline void NesCpuTranslator::mWrite8(Register addr, Register src, RegList preserve)
{
	Q_ASSERT(!(preserve & ~0xf)); // allow preserving only r0-r3

	mStoreCurrentCycles();

	RegList preserved = preserve | ip.bit();
	__ mrs(ip, CPSR);
	__ stm(db_w, sp, preserved);

	// move addr and src to r0 and r1
	Register data = src;
	if (data == r0 || addr == r1) {
		if (data == r0 && addr == r1) {
			__ Swap(r0, r1);
			addr = r0;
			data = r1;
		} else if (data == r0) {
			__ mov(r1, data);
			data = r1;
		} else { // if (addr == r1)
			__ mov(r0, addr);
			addr = r0;
		}
	}
	if (data != r1)
		__ mov(r1, data);
	if (addr != r0)
		__ mov(r0, addr);

	mCallCFunction(offsetof(NesCpuRecData,cpuWrite));

	__ ldm(ia_w, sp, preserved);
	__ msr(CPSR_f, Operand(ip));

	// write could trigger an alert
	m_checkAlertAfterInstruction = true;
}

inline void NesCpuTranslator::mRead8CallCFunc(u16 addr,
											  Register dst,
											  RegList preserve,
											  int functionOffset)
{
	Q_ASSERT(!(preserve & ~0xf)); // allow preserving only r0-r3
	Q_ASSERT(!dst.isValid() || !(preserve & dst.bit())); // preserve will not work for the result

	RegList preserved = preserve | ip.bit();
	__ mrs(ip, CPSR);
	__ stm(db_w, sp, preserved);

	__ mov(r0, Operand(addr));
	mCallCFunction(functionOffset);
	if (dst.isValid() && dst != r0)
		__ mov(dst, r0);

	__ ldm(ia_w, sp, preserved);
	__ msr(CPSR_f, Operand(ip));
}

inline void NesCpuTranslator::mWrite8CallCFunc(u16 addr,
											   Register src,
											   RegList preserve,
											   int functionOffset,
											   bool cheatsProcessing)
{
	Q_ASSERT(!(preserve & ~0xf)); // allow preserving only r0-r3

	RegList preserved = preserve | ip.bit();
	__ mrs(ip, CPSR);
	__ stm(db_w, sp, preserved);

	if (src != r1)
		__ mov(r1, src);
	__ mov(r0, Operand(addr));
	mCallCFunction(functionOffset);

	if (cheatsProcessing)
		mCallCFunction(offsetof(NesCpuRecData,processCheats));

	__ ldm(ia_w, sp, preserved);
	__ msr(CPSR_f, Operand(ip));
}

inline void NesCpuTranslator::mReadRam8(u16 addr, Register dst)
{
	Q_ASSERT(addr < 0x800);
	__ ldr(dst, MemOperand(mDataBase, offsetof(NesCpuRecData,ram)));
	__ ldrb(dst, MemOperand(dst, addr));
}

inline void NesCpuTranslator::mReadRam8(Register addr, Register dst)
{
	Q_ASSERT(addr != dst);
	__ ldr(dst, MemOperand(mDataBase, offsetof(NesCpuRecData,ram)));
	__ ldrb(dst, MemOperand(dst, addr));
}

inline void NesCpuTranslator::mWriteRam8(u16 addr, Register src)
{
	Q_ASSERT(addr < 0x800);
	// addr is encoded at most in 12 bits, so we can use ip register here
	__ ldr(ip, MemOperand(mDataBase, offsetof(NesCpuRecData,ram)));
	__ strb(src, MemOperand(ip, addr));
}

inline void NesCpuTranslator::mWriteRam8(Register addr, Register src)
{
	Q_ASSERT(addr != src);
	__ ldr(ip, MemOperand(mDataBase, offsetof(NesCpuRecData,ram)));
	__ strb(src, MemOperand(ip, addr));
}

inline void NesCpuTranslator::mReadZp8(u8 addr, Register dst)
{
	mReadRam8(addr, dst);
}

inline void NesCpuTranslator::mReadZp8(Register addr, Register dst)
{
	mReadRam8(addr, dst);
}

inline void NesCpuTranslator::mWriteZp8(u8 addr, Register src)
{
	mWriteRam8(addr, src);
}

inline void NesCpuTranslator::mWriteZp8(Register addr, Register src)
{
	mWriteRam8(addr, src);
}

inline void NesCpuTranslator::mReadZp16(u8 addr, Register dst)
{
	mReadRam8(addr, dst);
	mReadRam8((addr+1)&0xff, ip);
	__ orr(dst, dst, Operand(ip, LSL, 8));
}

inline void NesCpuTranslator::mReadZp16(Register addr, Register dst, Register scratch)
{
	mReadRam8(addr, dst);
	__ add(ip, addr, Operand(1));
	__ and_(ip, ip, Operand(0xff));
	mReadRam8(ip, scratch);
	__ orr(dst, dst, Operand(scratch, LSL, 8));
}

inline void NesCpuTranslator::mReadDirect8(u16 addr, Register dst)
{
	// ip may be modified here because addr can exceed 12-bit offset
	Q_ASSERT(dst != ip);
	__ ldr(dst, MemOperand(mDataBase, offsetof(NesCpuRecData,banks)));
	__ ldr(dst, MemOperand(dst, (addr>>13) * kPointerSize));
	__ ldrb(dst, MemOperand(dst, addr & 0x1fff));
}

void NesCpuTranslator::mReadDirect8(Register addr, Register dst)
{
	Q_ASSERT(dst != ip && addr != ip && addr != dst);
	__ ldr(dst, MemOperand(mDataBase, offsetof(NesCpuRecData,banks)));
	__ mov(ip, Operand(addr, LSR, 13));
	__ ldr(dst, MemOperand(dst, ip, LSL, 2));
	__ mov(ip, Operand(0x1fff));
	__ And(ip, addr, Operand(ip));
	__ ldrb(dst, MemOperand(dst, ip));
}

inline void NesCpuTranslator::mRead8(u16 addr, Register dst, RegList preserve)
{
	Q_ASSERT(!(preserve & ~0xf)); // allow preserving only r0-r3
	switch (addr >> 13) {
	case 0:	// 0x0000-0x1fff
		if (dst.isValid())
			mReadRam8(addr & 0x07ff, dst);
		break;
	case 1:	// 0x2000-0x3fff
		mRead8CallCFunc(addr & 7, dst, preserve,
						offsetof(NesCpuRecData,ppuReadReg));
		break;
	case 2:	// 0x4000-0x5fff
		if (addr < 0x4100)
			mRead8CallCFunc(addr, dst, preserve,
							offsetof(NesCpuRecData,cpuRead40xx));
		else
			mRead8CallCFunc(addr, dst, preserve,
							offsetof(NesCpuRecData,cpuReadLow));
		break;
	case 3:	// 0x6000-0x7fff
		mRead8CallCFunc(addr, dst, preserve,
						offsetof(NesCpuRecData,cpuReadLow));
		break;
	case 4:	// 0x8000-0x9fff
	case 5:	// 0xa000-0xbfff
	case 6:	// 0xc000-0xdfff
	case 7:	// 0xe000-0xffff
		if (dst.isValid()) {
			// check if in same bank - then only read data once if possible
			if (nesCpuPageByAddr(currentPc()) == nesCpuPageByAddr(addr)) {
				u8 data = nesCpuReadDirect(addr);
				__ mov(dst, Operand(data));
			} else {
				mReadDirect8(addr, dst);
			}
		}
		break;
	}
}

inline void NesCpuTranslator::mWrite8(u16 addr, Register src, RegList preserve)
{
	Q_ASSERT(!(preserve & ~0xf)); // allow preserving only r0-r3
	switch (addr >> 13) {
	case 0: // 0x0000-0x1fff
		mWriteRam8(addr & 0x07ff, src);
		break;
	case 1:	// 0x2000-0x3fff
		mWrite8CallCFunc(addr & 7, src, preserve,
						 offsetof(NesCpuRecData,ppuWriteReg));
		if ((addr & 7) == 0) // check alert for write to 0x2000 address
			m_checkAlertAfterInstruction = true;
		break;
	case 2:	// 0x4000-0x5fff
		if (addr < 0x4100) {
			mStoreCurrentCycles();
			mWrite8CallCFunc(addr, src, preserve,
							 offsetof(NesCpuRecData,cpuWrite40xx));
		} else {
			mWrite8CallCFunc(addr, src, preserve,
							 offsetof(NesCpuRecData,cpuWriteLow));
		}
		m_checkAlertAfterInstruction = true; // check alert for DMA and APU, etc.
		break;
	case 3:	// 0x6000-0x7fff
		mWrite8CallCFunc(addr, src, preserve,
						 offsetof(NesCpuRecData,cpuWriteLow));
		m_checkAlertAfterInstruction = true; // check alert for DMA and APU, etc.
		break;
	case 4: // 0x8000-0x9fff
	case 5:	// 0xa000-0xbfff
	case 6:	// 0xc000-0xdfff
	case 7:	// 0xe000-0xffff
		if (nesMapper->hasWriteHigh()) {
			// do cheat processing here also
			mWrite8CallCFunc(addr, src, preserve,
							 offsetof(NesCpuRecData,cpuWriteHigh),
							 true);
			m_checkAlertAfterInstruction = true;
		}
		break;
	}
}

#endif // NESCPUREC_MEMORY_H
