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

#include "cpubase.h"
#include "mapper.h"

NesCpuBase *nesCpu = 0;

void NesCpuBase::reset()
{
	u8 lo = nesCpuRead(ResetVectorAddress  );
	u8 hi = nesCpuRead(ResetVectorAddress+1);
	m_stateRegs.pc = lo|(hi<<8);
	m_stateRegs.a = 0;
	m_stateRegs.y = 0;
	m_stateRegs.x = 0;
	m_stateRegs.s = 0xff;
	m_stateRegs.p = IrqDisable;
}

void NesCpuBase::sl()
{
	emsl.begin("cpu");
	emsl.var("signals", m_stateSignals);
	emsl.var("interrupts", m_stateInterrupts);
	emsl.array("regs", &m_stateRegs, sizeof(NesCpuBaseRegisters));
	emsl.end();
}

const u8 NesCpuBase::cyclesTable[256] =
{
/* 0x00 */7, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6,
/* 0x10 */2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
/* 0x20 */6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 4, 4, 6, 6,
/* 0x30 */2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
/* 0x40 */6, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 3, 4, 6, 6,
/* 0x50 */2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
/* 0x60 */6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 6, 6,
/* 0x70 */2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
/* 0x80 */2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
/* 0x90 */2, 6, 2, 6, 4, 4, 4, 4, 2, 5, 2, 5, 5, 5, 5, 5,
/* 0xa0 */2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
/* 0xb0 */2, 5, 2, 5, 4, 4, 4, 4, 2, 4, 2, 4, 4, 4, 4, 4,
/* 0xc0 */2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
/* 0xd0 */2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
/* 0xe0 */2, 6, 3, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
/* 0xf0 */2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7
};

const u8 NesCpuBase::sizeTable[256] =
{
/* 0x00 */1, 2, 1, 2, 2, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 3,
/* 0x10 */2, 2, 1, 2, 2, 2, 2, 2, 1, 3, 1, 3, 3, 3, 3, 3,
/* 0x20 */3, 2, 1, 2, 2, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 3,
/* 0x30 */2, 2, 1, 2, 2, 2, 2, 2, 1, 3, 1, 3, 3, 3, 3, 3,
/* 0x40 */1, 2, 1, 2, 2, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 3,
/* 0x50 */2, 2, 1, 2, 2, 2, 2, 2, 1, 3, 1, 3, 3, 3, 3, 3,
/* 0x60 */1, 2, 1, 2, 2, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 3,
/* 0x70 */2, 2, 1, 2, 2, 2, 2, 2, 1, 3, 1, 3, 3, 3, 3, 3,
/* 0x80 */2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 3,
/* 0x90 */2, 2, 1, 2, 2, 2, 2, 2, 1, 3, 1, 3, 3, 3, 3, 3,
/* 0xa0 */2, 2, 2, 2, 2, 2, 2, 3, 1, 2, 1, 2, 3, 3, 3, 4,
/* 0xb0 */2, 2, 1, 2, 2, 2, 2, 4, 1, 3, 1, 3, 3, 3, 3, 4,
/* 0xc0 */2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 3,
/* 0xd0 */2, 2, 1, 2, 2, 2, 2, 2, 1, 3, 1, 3, 3, 3, 3, 3,
/* 0xe0 */2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 3,
/* 0xf0 */2, 2, 1, 2, 2, 2, 2, 2, 1, 3, 1, 3, 3, 3, 3, 3
};

const u8 NesCpuBase::addressingModeTable[256] =
{
/* 0x00 */Impli, IndiX, Impli, IndiX, ZeroP, ZeroP, ZeroP, ZeroP,
/* 0x08 */Impli, Immed, Accum, Immed, Absol, Absol, Absol, Absol,
/* 0x10 */Relat, IndiY, Impli, IndiY, ZerPX, ZerPX, ZerPX, ZerPX,
/* 0x18 */Impli, AbsoY, Impli, AbsoY, AbsoX, AbsoX, AbsoX, AbsoX,
/* 0x20 */Absol, IndiX, Impli, IndiX, ZeroP, ZeroP, ZeroP, ZeroP,
/* 0x28 */Impli, Immed, Accum, Immed, Absol, Absol, Absol, Absol,
/* 0x30 */Relat, IndiY, Impli, IndiY, ZerPX, ZerPX, ZerPX, ZerPX,
/* 0x38 */Impli, AbsoY, Impli, AbsoY, AbsoX, AbsoX, AbsoX, AbsoX,
/* 0x40 */Impli, IndiX, Impli, IndiX, ZeroP, ZeroP, ZeroP, ZeroP,
/* 0x48 */Impli, Immed, Accum, Immed, Absol, Absol, Absol, Absol,
/* 0x50 */Relat, IndiY, Impli, IndiY, ZerPX, ZerPX, ZerPX, ZerPX,
/* 0x58 */Impli, AbsoY, Impli, AbsoY, AbsoX, AbsoX, AbsoX, AbsoX,
/* 0x60 */Impli, IndiX, Impli, IndiX, ZeroP, ZeroP, ZeroP, ZeroP,
/* 0x68 */Impli, Immed, Accum, Immed, Indir, Absol, Absol, Absol,
/* 0x70 */Relat, IndiY, Impli, IndiY, ZerPX, ZerPX, ZerPX, ZerPX,
/* 0x78 */Impli, AbsoY, Impli, AbsoY, AbsoX, AbsoX, AbsoX, AbsoX,
/* 0x80 */Immed, IndiX, Immed, IndiX, ZeroP, ZeroP, ZeroP, ZeroP,
/* 0x88 */Impli, Immed, Impli, Immed, Absol, Absol, Absol, Absol,
/* 0x90 */Relat, IndiY, Impli, IndiY, ZerPX, ZerPX, ZerPY, ZerPY,
/* 0x98 */Impli, AbsoY, Impli, AbsoY, AbsoX, AbsoX, AbsoY, AbsoY,
/* 0xa0 */Immed, IndiX, Immed, IndiX, ZeroP, ZeroP, ZeroP, ZeroP,
/* 0xa8 */Impli, Immed, Impli, Immed, Absol, Absol, Absol, Absol,
/* 0xb0 */Relat, IndiY, Impli, IndiY, ZerPX, ZerPX, ZerPY, ZerPY,
/* 0xb8 */Impli, AbsoY, Impli, AbsoY, AbsoX, AbsoX, AbsoY, AbsoY,
/* 0xc0 */Immed, IndiX, Immed, IndiX, ZeroP, ZeroP, ZeroP, ZeroP,
/* 0xc8 */Impli, Immed, Impli, Immed, Absol, Absol, Absol, Absol,
/* 0xd0 */Relat, IndiY, Impli, IndiY, ZerPX, ZerPX, ZerPX, ZerPX,
/* 0xd8 */Impli, AbsoY, Impli, AbsoY, AbsoX, AbsoX, AbsoX, AbsoX,
/* 0xe0 */Immed, IndiX, Immed, IndiX, ZeroP, ZeroP, ZeroP, ZeroP,
/* 0xe8 */Impli, Immed, Impli, Immed, Absol, Absol, Absol, Absol,
/* 0xf0 */Relat, IndiY, Impli, IndiY, ZerPX, ZerPX, ZerPX, ZerPX,
/* 0xf8 */Impli, AbsoY, Impli, AbsoY, AbsoX, AbsoX, AbsoX, AbsoX
};

const char *NesCpuBase::nameTable[256] =
{
/* 0x00 */"BRK", "ORA", "KIL", "SLO", "DOP", "ORA", "ASL", "SLO",
/* 0x08 */"PHP", "ORA", "ASL", "AAC", "TOP", "ORA", "ASL", "SLO",
/* 0x10 */"BPL", "ORA", "KIL", "SLO", "DOP", "ORA", "ASL", "SLO",
/* 0x18 */"CLC", "ORA", "NOP", "SLO", "TOP", "ORA", "ASL", "SLO",
/* 0x20 */"JSR", "AND", "KIL", "RLA", "BIT", "AND", "ROL", "RLA",
/* 0x28 */"PLP", "AND", "ROL", "AAC", "BIT", "AND", "ROL", "RLA",
/* 0x30 */"BMI", "AND", "KIL", "RLA", "DOP", "AND", "ROL", "RLA",
/* 0x38 */"SEC", "AND", "NOP", "RLA", "TOP", "AND", "ROL", "RLA",
/* 0x40 */"RTI", "EOR", "KIL", "SRE", "DOP", "EOR", "LSR", "SRE",
/* 0x48 */"PHA", "EOR", "LSR", "ASR", "JMP", "EOR", "LSR", "SRE",
/* 0x50 */"BVC", "EOR", "KIL", "SRE", "DOP", "EOR", "LSR", "SRE",
/* 0x58 */"CLI", "EOR", "NOP", "SRE", "TOP", "EOR", "LSR", "SRE",
/* 0x60 */"RTS", "ADC", "KIL", "RRA", "DOP", "ADC", "ROR", "RRA",
/* 0x68 */"PLA", "ADC", "ROR", "ARR", "JMP", "ADC", "ROR", "RRA",
/* 0x70 */"BVS", "ADC", "KIL", "RRA", "DOP", "ADC", "ROR", "RRA",
/* 0x78 */"SEI", "ADC", "NOP", "RRA", "TOP", "ADC", "ROR", "RRA",
/* 0x80 */"DOP", "STA", "DOP", "AAX", "STY", "STA", "STX", "AAX",
/* 0x88 */"DEY", "DOP", "TXA", "XAA", "STY", "STA", "STX", "AAX",
/* 0x90 */"BCC", "STA", "KIL", "AXA", "STY", "STA", "STX", "AAX",
/* 0x98 */"TYA", "STA", "TXS", "XAS", "SYA", "STA", "SXA", "AXA",
/* 0xa0 */"LDY", "LDA", "LDX", "LAX", "LDY", "LDA", "LDX", "LAX",
/* 0xa8 */"TAY", "LDA", "TAX", "ATX", "LDY", "LDA", "LDX", "LAX",
/* 0xb0 */"BCS", "LDA", "KIL", "LAX", "LDY", "LDA", "LDX", "LAX",
/* 0xb8 */"CLV", "LDA", "TSX", "LAR", "LDY", "LDA", "LDX", "LAX",
/* 0xc0 */"CPY", "CMP", "DOP", "DCP", "CPY", "CMP", "DEC", "DCP",
/* 0xc8 */"INY", "CMP", "DEX", "AXS", "CPY", "CMP", "DEC", "DCP",
/* 0xd0 */"BNE", "CMP", "KIL", "DCP", "DOP", "CMP", "DEC", "DCP",
/* 0xd8 */"CLD", "CMP", "NOP", "DCP", "TOP", "CMP", "DEC", "DCP",
/* 0xe0 */"CPX", "SBC", "DOP", "ISC", "CPX", "SBC", "INC", "ISC",
/* 0xe8 */"INX", "SBC", "NOP", "SBC", "CPX", "SBC", "INC", "ISC",
/* 0xf0 */"BEQ", "SBC", "KIL", "ISC", "DOP", "SBC", "INC", "ISC",
/* 0xf8 */"SED", "SBC", "NOP", "ISC", "TOP", "SBC", "INC", "ISC"
};
