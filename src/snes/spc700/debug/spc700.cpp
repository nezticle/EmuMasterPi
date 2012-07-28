/*
 * Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
 *
 * (c) Copyright 1996 - 2001 Gary Henderson (gary.henderson@ntlworld.com) and
 *                           Jerremy Koot (jkoot@snes9x.com)
 *
 * Super FX C emulator code 
 * (c) Copyright 1997 - 1999 Ivar (ivar@snes9x.com) and
 *                           Gary Henderson.
 * Super FX assembler emulator code (c) Copyright 1998 zsKnight and _Demo_.
 *
 * DSP1 emulator code (c) Copyright 1998 Ivar, _Demo_ and Gary Henderson.
 * C4 asm and some C emulation code (c) Copyright 2000 zsKnight and _Demo_.
 * C4 C code (c) Copyright 2001 Gary Henderson (gary.henderson@ntlworld.com).
 *
 * DOS port code contains the works of other authors. See headers in
 * individual files.
 *
 * Snes9x homepage: http://www.snes9x.com
 *
 * Permission to use, copy, modify and distribute Snes9x in both binary and
 * source form, for non-commercial purposes, is hereby granted without fee,
 * providing that this license information and copyright notice appear with
 * all copies and any derived work.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event shall the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Snes9x is freeware for PERSONAL USE only. Commercial users should
 * seek permission of the copyright holders first. Commercial use includes
 * charging money for Snes9x or software derived from Snes9x.
 *
 * The copyright holders request that bug fixes and improvements to the code
 * should be forwarded to them so everyone can benefit from the modifications
 * in future versions.
 *
 * Super NES and Super Nintendo Entertainment System are trademarks of
 * Nintendo Co., Limited and its subsidiary companies.
 */
#include "snes9x.h"
#include "memmap.h"
#include "display.h"
#include "cpuexec.h"
#include "apu.h"
#include "spc700.h"

// SPC700/Sound DSP chips have a 24.57MHz crystal on their PCB.

//#if defined(ASM_SPC700)
extern "C" {
uint8 S9xAPUGetByteZ (uint8 address);
uint8 S9xAPUGetByte (uint32 address);
void S9xAPUSetByteZ (uint8, uint8 address);
void S9xAPUSetByte (uint8, uint32 address);
}
/*
#elif defined(NO_INLINE_SET_GET)
uint8 S9xAPUGetByteZ (uint8 address);
uint8 S9xAPUGetByte (uint32 address);
void S9xAPUSetByteZ (uint8, uint8 address);
void S9xAPUSetByte (uint8, uint32 address);

#else
#undef INLINE
#define INLINE inline
#include "apumem.h"
#endif
*/

START_EXTERN_C
extern uint8 Work8;
extern uint16 Work16;
extern uint32 Work32;
extern signed char Int8;
extern short Int16;
extern long Int32;
extern short Int16;
extern uint8 W1;
extern uint8 W2;

END_EXTERN_C

#define OP1 (*(pIAPU->PC + 1))
#define OP2 (*(pIAPU->PC + 2))

#ifdef SPC700_SHUTDOWN
#define APUShutdown() \
    if (Settings.Shutdown && (pIAPU->PC == pIAPU->WaitAddress1 || pIAPU->PC == pIAPU->WaitAddress2)) \
    { \
		if (pIAPU->WaitCounter == 0) \
		{ \
			if (!ICPU.CPUExecuting) \
				CPU.APU_Cycles = CPU.Cycles = CPU.NextEvent; \
			else \
				CPU.APU_APUExecuting = FALSE; \
		} \
		else \
		if (pIAPU->WaitCounter >= 2) \
			pIAPU->WaitCounter = 1; \
	else \
	    pIAPU->WaitCounter--; \
    }
#else
#define APUShutdown()
#endif

#define APUSetZN8(b)\
    pIAPU->_Zero = (b);

#define APUSetZN16(w)\
    pIAPU->_Zero = ((w) != 0) | ((w) >> 8);

void STOP (char *s)
{
    char buffer[100];

#ifdef DEBUGGER
    S9xAPUOPrint (buffer, pIAPU->PC - pIAPU->RAM);
#endif

    sprintf (String, "Sound CPU in unknown state executing %s at %04lX\n%s\n", s, pIAPU->PC - pIAPU->RAM, buffer);
    S9xMessage (S9X_ERROR, S9X_APU_STOPPED, String);
    APU.TimerEnabled[0] = APU.TimerEnabled[1] = APU.TimerEnabled[2] = FALSE;
    CPU.APU_APUExecuting = FALSE;

#ifdef DEBUGGER
    CPU.Flags |= DEBUG_MODE_FLAG;
#else
    S9xExit ();
#endif
}

#define TCALL(n)\
{\
    PushW ((pIAPU->PC - pIAPU->RAM + 1)); \
    pIAPU->PC = pIAPU->RAM + (APU.ExtraRAM [((15 - n) << 1)] + \
	     (APU.ExtraRAM [((15 - n) << 1) + 1] << 8)); \
}

// XXX: HalfCarry - BJ fixed?
#define SBC(a,b)\
Int16 = (short) (a) - (short) (b) + (short) (APUCheckCarry ()) - 1;\
pIAPU->_Carry = Int16 >= 0;\
if ((((a) ^ (b)) & 0x80) && (((a) ^ (uint8) Int16) & 0x80))\
    APUSetOverflow ();\
else \
    APUClearOverflow (); \
APUSetHalfCarry ();\
if(((a) ^ (b) ^ (uint8) Int16) & 0x10)\
    APUClearHalfCarry ();\
(a) = (uint8) Int16;\
APUSetZN8 ((uint8) Int16);

// XXX: HalfCarry - BJ fixed?
#define ADC(a,b)\
Work16 = (a) + (b) + APUCheckCarry();\
pIAPU->_Carry = Work16 >= 0x100; \
if (~((a) ^ (b)) & ((b) ^ (uint8) Work16) & 0x80)\
    APUSetOverflow ();\
else \
    APUClearOverflow (); \
APUClearHalfCarry ();\
/*if(((a) ^ (b) ^ (uint8) Int16) & 0x10)  notaz: Int16!? */\
if(((a) ^ (b) ^ (uint8) Work16) & 0x10)\
    APUSetHalfCarry ();\
(a) = (uint8) Work16;\
APUSetZN8 ((uint8) Work16);

#define CMP(a,b)\
Int16 = (short) (a) - (short) (b);\
pIAPU->_Carry = Int16 >= 0;\
APUSetZN8 ((uint8) Int16);

#define ASL(b)\
    pIAPU->_Carry = ((b) & 0x80) != 0; \
    (b) <<= 1;\
    APUSetZN8 (b);
#define LSR(b)\
    pIAPU->_Carry = (b) & 1;\
    (b) >>= 1;\
    APUSetZN8 (b);
#define ROL(b)\
    Work16 = ((b) << 1) | APUCheckCarry (); \
    pIAPU->_Carry = Work16 >= 0x100; \
    (b) = (uint8) Work16; \
    APUSetZN8 (b);
#define ROR(b)\
    Work16 = (b) | ((uint16) APUCheckCarry () << 8); \
    pIAPU->_Carry = (uint8) Work16 & 1; \
    Work16 >>= 1; \
    (b) = (uint8) Work16; \
    APUSetZN8 (b);

#define Push(b)\
    *(pIAPU->RAM + 0x100 + pIAPU->S) = b;\
    pIAPU->S--;

#define Pop(b)\
    pIAPU->S++;\
    (b) = *(pIAPU->RAM + 0x100 + pIAPU->S);

#ifdef FAST_LSB_WORD_ACCESS
#define PushW(w)\
    *(uint16 *) (pIAPU->RAM + 0xff + pIAPU->S) = w;\
    pIAPU->S -= 2;
#define PopW(w)\
    pIAPU->S += 2;\
    w = *(uint16 *) (pIAPU->RAM + 0xff + pIAPU->S);
#else
#define PushW(w)\
    *(pIAPU->RAM + 0xff + pIAPU->S) = w;\
    *(pIAPU->RAM + 0x100 + pIAPU->S) = (w >> 8);\
    pIAPU->S -= 2;
#define PopW(w)\
    pIAPU->S += 2; \
    (w) = *(pIAPU->RAM + 0xff + pIAPU->S) + (*(pIAPU->RAM + 0x100 + pIAPU->S) << 8);
#endif

#define Relative()\
    Int8 = OP1;\
    Int16 = (int) (pIAPU->PC + 2 - pIAPU->RAM) + Int8;

#define Relative2()\
    Int8 = OP2;\
    Int16 = (int) (pIAPU->PC + 3 - pIAPU->RAM) + Int8;

#ifdef FAST_LSB_WORD_ACCESS
#define IndexedXIndirect()\
    pIAPU->Address = *(uint16 *) (pIAPU->DirectPage + ((OP1 + pIAPU->X) & 0xff));

#define Absolute()\
    pIAPU->Address = *(uint16 *) (pIAPU->PC + 1);

#define AbsoluteX()\
    pIAPU->Address = *(uint16 *) (pIAPU->PC + 1) + pIAPU->X;

#define AbsoluteY()\
    pIAPU->Address = *(uint16 *) (pIAPU->PC + 1) + pIAPU->YA.B.Y;

#define MemBit()\
    pIAPU->Address = *(uint16 *) (pIAPU->PC + 1);\
    pIAPU->Bit = (uint8)(pIAPU->Address >> 13);\
    pIAPU->Address &= 0x1fff;

#define IndirectIndexedY()\
    pIAPU->Address = *(uint16 *) (pIAPU->DirectPage + OP1) + pIAPU->YA.B.Y;
#else
#define IndexedXIndirect()\
    pIAPU->Address = *(pIAPU->DirectPage + ((OP1 + pIAPU->X) & 0xff)) + \
		  (*(pIAPU->DirectPage + ((OP1 + pIAPU->X + 1) & 0xff)) << 8);
#define Absolute()\
    pIAPU->Address = OP1 + (OP2 << 8);

#define AbsoluteX()\
    pIAPU->Address = OP1 + (OP2 << 8) + pIAPU->X;

#define AbsoluteY()\
    pIAPU->Address = OP1 + (OP2 << 8) + pIAPU->YA.B.Y;

#define MemBit()\
    pIAPU->Address = OP1 + (OP2 << 8);\
    pIAPU->Bit = (int8) (pIAPU->Address >> 13);\
    pIAPU->Address &= 0x1fff;

#define IndirectIndexedY()\
    pIAPU->Address = *(pIAPU->DirectPage + OP1) + \
		  (*(pIAPU->DirectPage + OP1 + 1) << 8) + \
		  pIAPU->YA.B.Y;
#endif

void Apu00 ()
{
// NOP
    pIAPU->PC++;
}

void Apu01 () { TCALL (0) }

void Apu11 () { TCALL (1) }

void Apu21 () { TCALL (2) }

void Apu31 () { TCALL (3) }

void Apu41 () { TCALL (4) }

void Apu51 () { TCALL (5) }

void Apu61 () { TCALL (6) }

void Apu71 () { TCALL (7) }

void Apu81 () { TCALL (8) }

void Apu91 () { TCALL (9) }

void ApuA1 () { TCALL (10) }

void ApuB1 () { TCALL (11) }

void ApuC1 () { TCALL (12) }

void ApuD1 () { TCALL (13) }

void ApuE1 () { TCALL (14) }

void ApuF1 () { TCALL (15) }

void Apu3F () // CALL absolute
{
    Absolute ();
    // 0xB6f for Star Fox 2
    PushW ((pIAPU->PC + 3 - pIAPU->RAM));
    pIAPU->PC = pIAPU->RAM + pIAPU->Address;
}

void Apu4F () // PCALL $XX
{
    Work8 = OP1;
    PushW ((pIAPU->PC + 2 - pIAPU->RAM));
    pIAPU->PC = pIAPU->RAM + 0xff00 + Work8;
}

#define SET(b) \
S9xAPUSetByteZ ((uint8) (S9xAPUGetByteZ (OP1 ) | (1 << (b))), OP1); \
pIAPU->PC += 2

void Apu02 ()
{
    SET (0);
}

void Apu22 ()
{
    SET (1);
}

void Apu42 ()
{
    SET (2);
}

void Apu62 ()
{
    SET (3);
}

void Apu82 ()
{
    SET (4);
}

void ApuA2 ()
{
    SET (5);
}

void ApuC2 ()
{
    SET (6);
}

void ApuE2 ()
{
    SET (7);
}

#define CLR(b) \
S9xAPUSetByteZ ((uint8) (S9xAPUGetByteZ (OP1) & ~(1 << (b))), OP1); \
pIAPU->PC += 2;

void Apu12 ()
{
    CLR (0);
}

void Apu32 ()
{
    CLR (1);
}

void Apu52 ()
{
    CLR (2);
}

void Apu72 ()
{
    CLR (3);
}

void Apu92 ()
{
    CLR (4);
}

void ApuB2 ()
{
    CLR (5);
}

void ApuD2 ()
{
    CLR (6);
}

void ApuF2 ()
{
    CLR (7);
}

#define BBS(b) \
Work8 = OP1; \
Relative2 (); \
if (S9xAPUGetByteZ (Work8) & (1 << (b))) \
{ \
    pIAPU->PC = pIAPU->RAM + (uint16) Int16; \
    CPU.APU_Cycles += pIAPU->TwoCycles; \
} \
else \
    pIAPU->PC += 3

void Apu03 ()
{
    BBS (0);
}

void Apu23 ()
{
    BBS (1);
}

void Apu43 ()
{
    BBS (2);
}

void Apu63 ()
{
    BBS (3);
}

void Apu83 ()
{
    BBS (4);
}

void ApuA3 ()
{
    BBS (5);
}

void ApuC3 ()
{
    BBS (6);
}

void ApuE3 ()
{
    BBS (7);
}

#define BBC(b) \
Work8 = OP1; \
Relative2 (); \
if (!(S9xAPUGetByteZ (Work8) & (1 << (b)))) \
{ \
    pIAPU->PC = pIAPU->RAM + (uint16) Int16; \
    CPU.APU_Cycles += pIAPU->TwoCycles; \
} \
else \
    pIAPU->PC += 3

void Apu13 ()
{
    BBC (0);
}

void Apu33 ()
{
    BBC (1);
}

void Apu53 ()
{
    BBC (2);
}

void Apu73 ()
{
    BBC (3);
}

void Apu93 ()
{
    BBC (4);
}

void ApuB3 ()
{
    BBC (5);
}

void ApuD3 ()
{
    BBC (6);
}

void ApuF3 ()
{
    BBC (7);
}

void Apu04 ()
{
// OR A,dp
    pIAPU->YA.B.A |= S9xAPUGetByteZ (OP1);
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC += 2;
}

void Apu05 ()
{
// OR A,abs
    Absolute ();
    pIAPU->YA.B.A |= S9xAPUGetByte (pIAPU->Address);
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC += 3;
}

void Apu06 ()
{
// OR A,(X)
    pIAPU->YA.B.A |= S9xAPUGetByteZ (pIAPU->X);
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC++;
}

void Apu07 ()
{
// OR A,(dp+X)
    IndexedXIndirect ();
    pIAPU->YA.B.A |= S9xAPUGetByte (pIAPU->Address);
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC += 2;
}

void Apu08 ()
{
// OR A,#00
    pIAPU->YA.B.A |= OP1;
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC += 2;
}

void Apu09 ()
{
// OR dp(dest),dp(src)
    Work8 = S9xAPUGetByteZ (OP1);
    Work8 |= S9xAPUGetByteZ (OP2);
    S9xAPUSetByteZ (Work8, OP2);
    APUSetZN8 (Work8);
    pIAPU->PC += 3;
}

void Apu14 ()
{
// OR A,dp+X
    pIAPU->YA.B.A |= S9xAPUGetByteZ (OP1 + pIAPU->X);
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC += 2;
}

void Apu15 ()
{
// OR A,abs+X
    AbsoluteX ();
    pIAPU->YA.B.A |= S9xAPUGetByte (pIAPU->Address);
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC += 3;
}

void Apu16 ()
{
// OR A,abs+Y
    AbsoluteY ();
    pIAPU->YA.B.A |= S9xAPUGetByte (pIAPU->Address);
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC += 3;
}

void Apu17 ()
{
// OR A,(dp)+Y
    IndirectIndexedY ();
    pIAPU->YA.B.A |= S9xAPUGetByte (pIAPU->Address);
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC += 2;
}

void Apu18 ()
{
// OR dp,#00
    Work8 = OP1;
    Work8 |= S9xAPUGetByteZ (OP2);
    S9xAPUSetByteZ (Work8, OP2);
    APUSetZN8 (Work8);
    pIAPU->PC += 3;
}

void Apu19 ()
{
// OR (X),(Y)
    Work8 = S9xAPUGetByteZ (pIAPU->X) | S9xAPUGetByteZ (pIAPU->YA.B.Y);
    APUSetZN8 (Work8);
    S9xAPUSetByteZ (Work8, pIAPU->X);
    pIAPU->PC++;
}

void Apu0A ()
{
// OR1 C,membit
    MemBit ();
    if (!APUCheckCarry ())
    {
	if (S9xAPUGetByte (pIAPU->Address) & (1 << pIAPU->Bit))
	    APUSetCarry ();
    }
    pIAPU->PC += 3;
}

void Apu2A ()
{
// OR1 C,not membit
    MemBit ();
    if (!APUCheckCarry ())
    {
	if (!(S9xAPUGetByte (pIAPU->Address) & (1 << pIAPU->Bit)))
	    APUSetCarry ();
    }
    pIAPU->PC += 3;
}

void Apu4A ()
{
// AND1 C,membit
    MemBit ();
    if (APUCheckCarry ())
    {
	if (!(S9xAPUGetByte (pIAPU->Address) & (1 << pIAPU->Bit)))
	    APUClearCarry ();
    }
    pIAPU->PC += 3;
}

void Apu6A ()
{
// AND1 C, not membit
    MemBit ();
    if (APUCheckCarry ())
    {
	if ((S9xAPUGetByte (pIAPU->Address) & (1 << pIAPU->Bit)))
	    APUClearCarry ();
    }
    pIAPU->PC += 3;
}

void Apu8A ()
{
// EOR1 C, membit
    MemBit ();
    if (APUCheckCarry ())
    {
	if (S9xAPUGetByte (pIAPU->Address) & (1 << pIAPU->Bit))
	    APUClearCarry ();
    }
    else
    {
	if (S9xAPUGetByte (pIAPU->Address) & (1 << pIAPU->Bit))
	    APUSetCarry ();
    }
    pIAPU->PC += 3;
}

void ApuAA ()
{
// MOV1 C,membit
    MemBit ();
    if (S9xAPUGetByte (pIAPU->Address) & (1 << pIAPU->Bit))
	APUSetCarry ();
    else
	APUClearCarry ();
    pIAPU->PC += 3;
}

void ApuCA ()
{
// MOV1 membit,C
    MemBit ();
    if (APUCheckCarry ())
    {
	S9xAPUSetByte (S9xAPUGetByte (pIAPU->Address) | (1 << pIAPU->Bit), pIAPU->Address);
    }
    else
    {
	S9xAPUSetByte (S9xAPUGetByte (pIAPU->Address) & ~(1 << pIAPU->Bit), pIAPU->Address);
    }
    pIAPU->PC += 3;
}

void ApuEA ()
{
// NOT1 membit
    MemBit ();
    S9xAPUSetByte (S9xAPUGetByte (pIAPU->Address) ^ (1 << pIAPU->Bit), pIAPU->Address);
    pIAPU->PC += 3;
}

void Apu0B ()
{
// ASL dp
    Work8 = S9xAPUGetByteZ (OP1);
    ASL (Work8);
    S9xAPUSetByteZ (Work8, OP1);
    pIAPU->PC += 2;
}

void Apu0C ()
{
// ASL abs
    Absolute ();
    Work8 = S9xAPUGetByte (pIAPU->Address);
    ASL (Work8);
    S9xAPUSetByte (Work8, pIAPU->Address);
    pIAPU->PC += 3;
}

void Apu1B ()
{
// ASL dp+X
    Work8 = S9xAPUGetByteZ (OP1 + pIAPU->X);
    ASL (Work8);
    S9xAPUSetByteZ (Work8, OP1 + pIAPU->X);
    pIAPU->PC += 2;
}

void Apu1C ()
{
// ASL A
    ASL (pIAPU->YA.B.A);
    pIAPU->PC++;
}

void Apu0D ()
{
// PUSH PSW
    S9xAPUPackStatus ();
    Push (pIAPU->P);
    pIAPU->PC++;
}

void Apu2D ()
{
// PUSH A
    Push (pIAPU->YA.B.A);
    pIAPU->PC++;
}

void Apu4D ()
{
// PUSH X
    Push (pIAPU->X);
    pIAPU->PC++;
}

void Apu6D ()
{
// PUSH Y
    Push (pIAPU->YA.B.Y);
    pIAPU->PC++;
}

void Apu8E ()
{
// POP PSW
    Pop (pIAPU->P);
    S9xAPUUnpackStatus ();
    if (APUCheckDirectPage ())
	pIAPU->DirectPage = pIAPU->RAM + 0x100;
    else
	pIAPU->DirectPage = pIAPU->RAM;
    pIAPU->PC++;
}

void ApuAE ()
{
// POP A
    Pop (pIAPU->YA.B.A);
    pIAPU->PC++;
}

void ApuCE ()
{
// POP X
    Pop (pIAPU->X);
    pIAPU->PC++;
}

void ApuEE ()
{
// POP Y
    Pop (pIAPU->YA.B.Y);
    pIAPU->PC++;
}

void Apu0E ()
{
// TSET1 abs
    Absolute ();
    Work8 = S9xAPUGetByte (pIAPU->Address);
    S9xAPUSetByte (Work8 | pIAPU->YA.B.A, pIAPU->Address);
    Work8 &= pIAPU->YA.B.A;
    APUSetZN8 (Work8);
    pIAPU->PC += 3;
}

void Apu4E ()
{
// TCLR1 abs
    Absolute ();
    Work8 = S9xAPUGetByte (pIAPU->Address);
    S9xAPUSetByte (Work8 & ~pIAPU->YA.B.A, pIAPU->Address);
    Work8 &= pIAPU->YA.B.A;
    APUSetZN8 (Work8);
    pIAPU->PC += 3;
}

void Apu0F ()
{
// BRK

#if 0
    STOP ("BRK");
#else
    PushW ((pIAPU->PC + 1 - pIAPU->RAM));
    S9xAPUPackStatus ();
    Push (pIAPU->P);
    APUSetBreak ();
    APUClearInterrupt ();
// XXX:Where is the BRK vector ???
    pIAPU->PC = pIAPU->RAM + APU.ExtraRAM[0x20] + (APU.ExtraRAM[0x21] << 8);
#endif
}

void ApuEF ()
{
// SLEEP
    // XXX: sleep
    // STOP ("SLEEP");
    CPU.APU_APUExecuting = FALSE;
    pIAPU->PC++;
}

void ApuFF ()
{
// STOP
    // STOP ("STOP");
    CPU.APU_APUExecuting = FALSE;
    pIAPU->PC++;
}

void Apu10 ()
{
// BPL
    Relative ();
    if (!APUCheckNegative ())
    {
	pIAPU->PC = pIAPU->RAM + (uint16) Int16;
	CPU.APU_Cycles += pIAPU->TwoCycles;
	APUShutdown ();
    }
    else
	pIAPU->PC += 2;
}

void Apu30 ()
{
// BMI
    Relative ();
    if (APUCheckNegative ())
    {
	pIAPU->PC = pIAPU->RAM + (uint16) Int16;
	CPU.APU_Cycles += pIAPU->TwoCycles;
	APUShutdown ();
    }
    else
	pIAPU->PC += 2;
}

void Apu90 ()
{
// BCC
    Relative ();
    if (!APUCheckCarry ())
    {
	pIAPU->PC = pIAPU->RAM + (uint16) Int16;
	CPU.APU_Cycles += pIAPU->TwoCycles;
	APUShutdown ();
    }
    else
	pIAPU->PC += 2;
}

void ApuB0 ()
{
// BCS
    Relative ();
    if (APUCheckCarry ())
    {
	pIAPU->PC = pIAPU->RAM + (uint16) Int16;
	CPU.APU_Cycles += pIAPU->TwoCycles;
	APUShutdown ();
    }
    else
	pIAPU->PC += 2;
}

void ApuD0 ()
{
// BNE
    Relative ();
    if (!APUCheckZero ())
    {
	pIAPU->PC = pIAPU->RAM + (uint16) Int16;
	CPU.APU_Cycles += pIAPU->TwoCycles;
	APUShutdown ();
    }
    else
	pIAPU->PC += 2;
}

void ApuF0 ()
{
// BEQ
    Relative ();
    if (APUCheckZero ())
    {
	pIAPU->PC = pIAPU->RAM + (uint16) Int16;
	CPU.APU_Cycles += pIAPU->TwoCycles;
	APUShutdown ();
    }
    else
	pIAPU->PC += 2;
}

void Apu50 ()
{
// BVC
    Relative ();
    if (!APUCheckOverflow ())
    {
	pIAPU->PC = pIAPU->RAM + (uint16) Int16;
	CPU.APU_Cycles += pIAPU->TwoCycles;
    }
    else
	pIAPU->PC += 2;
}

void Apu70 ()
{
// BVS
    Relative ();
    if (APUCheckOverflow ())
    {
	pIAPU->PC = pIAPU->RAM + (uint16) Int16;
	CPU.APU_Cycles += pIAPU->TwoCycles;
    }
    else
	pIAPU->PC += 2;
}

void Apu2F ()
{
// BRA
    Relative ();
    pIAPU->PC = pIAPU->RAM + (uint16) Int16;
}

void Apu80 ()
{
// SETC
    APUSetCarry ();
    pIAPU->PC++;
}

void ApuED ()
{
// NOTC
    pIAPU->_Carry ^= 1;
    pIAPU->PC++;
}

void Apu40 ()
{
// SETP
    APUSetDirectPage ();
    pIAPU->DirectPage = pIAPU->RAM + 0x100;
    pIAPU->PC++;
}

void Apu1A ()
{
// DECW dp
    Work16 = S9xAPUGetByteZ (OP1) + (S9xAPUGetByteZ (OP1 + 1) << 8);
    Work16--;
    S9xAPUSetByteZ ((uint8) Work16, OP1);
    S9xAPUSetByteZ (Work16 >> 8, OP1 + 1);
    APUSetZN16 (Work16);
    pIAPU->PC += 2;
}

void Apu5A ()
{
// CMPW YA,dp
    Work16 = S9xAPUGetByteZ (OP1) + (S9xAPUGetByteZ (OP1 + 1) << 8);
    Int32 = (long) pIAPU->YA.W - (long) Work16;
    pIAPU->_Carry = Int32 >= 0;
    APUSetZN16 ((uint16) Int32);
    pIAPU->PC += 2;
}

void Apu3A ()
{
// INCW dp
    Work16 = S9xAPUGetByteZ (OP1) + (S9xAPUGetByteZ (OP1 + 1) << 8);
    Work16++;
    S9xAPUSetByteZ ((uint8) Work16, OP1);
    S9xAPUSetByteZ (Work16 >> 8, OP1 + 1);
    APUSetZN16 (Work16);
    pIAPU->PC += 2;
}

// XXX: HalfCarry - BJ Fixed? Or is it between bits 7 and 8 for ADDW/SUBW?
void Apu7A ()
{
// ADDW YA,dp
    Work16 = S9xAPUGetByteZ (OP1) + (S9xAPUGetByteZ (OP1 + 1) << 8);
    Work32 = (uint32) pIAPU->YA.W + Work16;
    pIAPU->_Carry = Work32 >= 0x10000;
    if (~(pIAPU->YA.W ^ Work16) & (Work16 ^ (uint16) Work32) & 0x8000)
	APUSetOverflow ();
    else
	APUClearOverflow ();
    APUClearHalfCarry ();
    if((pIAPU->YA.W ^ Work16 ^ (uint16) Work32) & 0x10)
        APUSetHalfCarry ();
    pIAPU->YA.W = (uint16) Work32;
    APUSetZN16 (pIAPU->YA.W);
    pIAPU->PC += 2;
}

// XXX: BJ: i think the old HalfCarry behavior was wrong...
// XXX: Or is it between bits 7 and 8 for ADDW/SUBW?
void Apu9A ()
{
// SUBW YA,dp
    Work16 = S9xAPUGetByteZ (OP1) + (S9xAPUGetByteZ (OP1 + 1) << 8);
    Int32 = (long) pIAPU->YA.W - (long) Work16;
    APUClearHalfCarry ();
    pIAPU->_Carry = Int32 >= 0;
    if (((pIAPU->YA.W ^ Work16) & 0x8000) &&
	    ((pIAPU->YA.W ^ (uint16) Int32) & 0x8000))
	APUSetOverflow ();
    else
	APUClearOverflow ();
//    if (((pIAPU->YA.W ^ Work16) & 0x0080) &&
//	    ((pIAPU->YA.W ^ (uint16) Int32) & 0x0080))
//	APUSetHalfCarry ();                              // notaz: strange here
    APUSetHalfCarry ();
//    if((pIAPU->YA.W ^ Work16 ^ (uint16) Work32) & 0x10) // notaz: Work32?!
    if((pIAPU->YA.W ^ Work16 ^ (uint16) Int32) & 0x10)
        APUClearHalfCarry ();
    pIAPU->YA.W = (uint16) Int32;
    APUSetZN16 (pIAPU->YA.W);
    pIAPU->PC += 2;
}

void ApuBA ()
{
// MOVW YA,dp
    pIAPU->YA.B.A = S9xAPUGetByteZ (OP1);
    pIAPU->YA.B.Y = S9xAPUGetByteZ (OP1 + 1);
    APUSetZN16 (pIAPU->YA.W);
    pIAPU->PC += 2;
}

void ApuDA ()
{
// MOVW dp,YA
    S9xAPUSetByteZ (pIAPU->YA.B.A, OP1);
    S9xAPUSetByteZ (pIAPU->YA.B.Y, OP1 + 1);
    pIAPU->PC += 2;
}

void Apu64 ()
{
// CMP A,dp
    Work8 = S9xAPUGetByteZ (OP1);
    CMP (pIAPU->YA.B.A, Work8);
    pIAPU->PC += 2;
}

void Apu65 ()
{
// CMP A,abs
    Absolute ();
    Work8 = S9xAPUGetByte (pIAPU->Address);
    CMP (pIAPU->YA.B.A, Work8);
    pIAPU->PC += 3;
}

void Apu66 ()
{
// CMP A,(X)
    Work8 = S9xAPUGetByteZ (pIAPU->X);
    CMP (pIAPU->YA.B.A, Work8);
    pIAPU->PC++;
}

void Apu67 ()
{
// CMP A,(dp+X)
    IndexedXIndirect ();
    Work8 = S9xAPUGetByte (pIAPU->Address);
    CMP (pIAPU->YA.B.A, Work8);
    pIAPU->PC += 2;
}

void Apu68 ()
{
// CMP A,#00
    Work8 = OP1;
    CMP (pIAPU->YA.B.A, Work8);
    pIAPU->PC += 2;
}

void Apu69 ()
{
// CMP dp(dest), dp(src)
    W1 = S9xAPUGetByteZ (OP1);
    Work8 = S9xAPUGetByteZ (OP2);
    CMP (Work8, W1);
    pIAPU->PC += 3;
}

void Apu74 ()
{
// CMP A, dp+X
    Work8 = S9xAPUGetByteZ (OP1 + pIAPU->X);
    CMP (pIAPU->YA.B.A, Work8);
    pIAPU->PC += 2;
}

void Apu75 ()
{
// CMP A,abs+X
    AbsoluteX ();
    Work8 = S9xAPUGetByte (pIAPU->Address);
    CMP (pIAPU->YA.B.A, Work8);
    pIAPU->PC += 3;
}

void Apu76 ()
{
// CMP A, abs+Y
    AbsoluteY ();
    Work8 = S9xAPUGetByte (pIAPU->Address);
    CMP (pIAPU->YA.B.A, Work8);
    pIAPU->PC += 3;
}

void Apu77 ()
{
// CMP A,(dp)+Y
    IndirectIndexedY ();
    Work8 = S9xAPUGetByte (pIAPU->Address);
    CMP (pIAPU->YA.B.A, Work8);
    pIAPU->PC += 2;
}

void Apu78 ()
{
// CMP dp,#00
    Work8 = OP1;
    W1 = S9xAPUGetByteZ (OP2);
    CMP (W1, Work8);
    pIAPU->PC += 3;
}

void Apu79 ()
{
// CMP (X),(Y)
    W1 = S9xAPUGetByteZ (pIAPU->X);
    Work8 = S9xAPUGetByteZ (pIAPU->YA.B.Y);
    CMP (W1, Work8);
    pIAPU->PC++;
}

void Apu1E ()
{
// CMP X,abs
    Absolute ();
    Work8 = S9xAPUGetByte (pIAPU->Address);
    CMP (pIAPU->X, Work8);
    pIAPU->PC += 3;
}

void Apu3E ()
{
// CMP X,dp
    Work8 = S9xAPUGetByteZ (OP1);
    CMP (pIAPU->X, Work8);
    pIAPU->PC += 2;
}

void ApuC8 ()
{
// CMP X,#00
    CMP (pIAPU->X, OP1);
    pIAPU->PC += 2;
}

void Apu5E ()
{
// CMP Y,abs
    Absolute ();
    Work8 = S9xAPUGetByte (pIAPU->Address);
    CMP (pIAPU->YA.B.Y, Work8);
    pIAPU->PC += 3;
}

void Apu7E ()
{
// CMP Y,dp
    Work8 = S9xAPUGetByteZ (OP1);
    CMP (pIAPU->YA.B.Y, Work8);
    pIAPU->PC += 2;
}

void ApuAD ()
{
// CMP Y,#00
    Work8 = OP1;
    CMP (pIAPU->YA.B.Y, Work8);
    pIAPU->PC += 2;
}

void Apu1F ()
{
// JMP (abs+X)
    Absolute ();
    pIAPU->PC = pIAPU->RAM + S9xAPUGetByte (pIAPU->Address + pIAPU->X) +
	(S9xAPUGetByte (pIAPU->Address + pIAPU->X + 1) << 8);
// XXX: HERE:
    // APU.Flags |= TRACE_FLAG;
}

void Apu5F ()
{
// JMP abs
    Absolute ();
    pIAPU->PC = pIAPU->RAM + pIAPU->Address;
}

void Apu20 ()
{
// CLRP
    APUClearDirectPage ();
    pIAPU->DirectPage = pIAPU->RAM;
    pIAPU->PC++;
}

void Apu60 ()
{
// CLRC
    APUClearCarry ();
    pIAPU->PC++;
}

void ApuE0 ()
{
// CLRV
    APUClearHalfCarry ();
    APUClearOverflow ();
    pIAPU->PC++;
}

void Apu24 ()
{
// AND A,dp
    pIAPU->YA.B.A &= S9xAPUGetByteZ (OP1);
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC += 2;
}

void Apu25 ()
{
// AND A,abs
    Absolute ();
    pIAPU->YA.B.A &= S9xAPUGetByte (pIAPU->Address);
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC += 3;
}

void Apu26 ()
{
// AND A,(X)
    pIAPU->YA.B.A &= S9xAPUGetByteZ (pIAPU->X);
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC++;
}

void Apu27 ()
{
// AND A,(dp+X)
    IndexedXIndirect ();
    pIAPU->YA.B.A &= S9xAPUGetByte (pIAPU->Address);
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC += 2;
}

void Apu28 ()
{
// AND A,#00
    pIAPU->YA.B.A &= OP1;
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC += 2;
}

void Apu29 ()
{
// AND dp(dest),dp(src)
    Work8 = S9xAPUGetByteZ (OP1);
    Work8 &= S9xAPUGetByteZ (OP2);
    S9xAPUSetByteZ (Work8, OP2);
    APUSetZN8 (Work8);
    pIAPU->PC += 3;
}

void Apu34 ()
{
// AND A,dp+X
    pIAPU->YA.B.A &= S9xAPUGetByteZ (OP1 + pIAPU->X);
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC += 2;
}

void Apu35 ()
{
// AND A,abs+X
    AbsoluteX ();
    pIAPU->YA.B.A &= S9xAPUGetByte (pIAPU->Address);
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC += 3;
}

void Apu36 ()
{
// AND A,abs+Y
    AbsoluteY ();
    pIAPU->YA.B.A &= S9xAPUGetByte (pIAPU->Address);
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC += 3;
}

void Apu37 ()
{
// AND A,(dp)+Y
    IndirectIndexedY ();
    pIAPU->YA.B.A &= S9xAPUGetByte (pIAPU->Address);
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC += 2;
}

void Apu38 ()
{
// AND dp,#00
    Work8 = OP1;
    Work8 &= S9xAPUGetByteZ (OP2);
    S9xAPUSetByteZ (Work8, OP2);
    APUSetZN8 (Work8);
    pIAPU->PC += 3;
}

void Apu39 ()
{
// AND (X),(Y)
    Work8 = S9xAPUGetByteZ (pIAPU->X) & S9xAPUGetByteZ (pIAPU->YA.B.Y);
    APUSetZN8 (Work8);
    S9xAPUSetByteZ (Work8, pIAPU->X);
    pIAPU->PC++;
}

void Apu2B ()
{
// ROL dp
    Work8 = S9xAPUGetByteZ (OP1);
    ROL (Work8);
    S9xAPUSetByteZ (Work8, OP1);
    pIAPU->PC += 2;
}

void Apu2C ()
{
// ROL abs
    Absolute ();
    Work8 = S9xAPUGetByte (pIAPU->Address);
    ROL (Work8);
    S9xAPUSetByte (Work8, pIAPU->Address);
    pIAPU->PC += 3;
}

void Apu3B ()
{
// ROL dp+X
    Work8 = S9xAPUGetByteZ (OP1 + pIAPU->X);
    ROL (Work8);
    S9xAPUSetByteZ (Work8, OP1 + pIAPU->X);
    pIAPU->PC += 2;
}

void Apu3C ()
{
// ROL A
    ROL (pIAPU->YA.B.A);
    pIAPU->PC++;
}

void Apu2E ()
{
// CBNE dp,rel
    Work8 = OP1;
    Relative2 ();
    
    if (S9xAPUGetByteZ (Work8) != pIAPU->YA.B.A)
    {
	pIAPU->PC = pIAPU->RAM + (uint16) Int16;
	CPU.APU_Cycles += pIAPU->TwoCycles;
	APUShutdown ();
    }
    else
	pIAPU->PC += 3;
}

void ApuDE ()
{
// CBNE dp+X,rel
    Work8 = OP1 + pIAPU->X;
    Relative2 ();

    if (S9xAPUGetByteZ (Work8) != pIAPU->YA.B.A)
    {
	pIAPU->PC = pIAPU->RAM + (uint16) Int16;
	CPU.APU_Cycles += pIAPU->TwoCycles;
	APUShutdown ();
    }
    else
	pIAPU->PC += 3;
}

void Apu3D ()
{
// INC X
    pIAPU->X++;
    APUSetZN8 (pIAPU->X);

#ifdef SPC700_SHUTDOWN
    pIAPU->WaitCounter++;
#endif

    pIAPU->PC++;
}

void ApuFC ()
{
// INC Y
    pIAPU->YA.B.Y++;
    APUSetZN8 (pIAPU->YA.B.Y);

#ifdef SPC700_SHUTDOWN
    pIAPU->WaitCounter++;
#endif

    pIAPU->PC++;
}

void Apu1D ()
{
// DEC X
    pIAPU->X--;
    APUSetZN8 (pIAPU->X);

#ifdef SPC700_SHUTDOWN
    pIAPU->WaitCounter++;
#endif

    pIAPU->PC++;
}

void ApuDC ()
{
// DEC Y
    pIAPU->YA.B.Y--;
    APUSetZN8 (pIAPU->YA.B.Y);

#ifdef SPC700_SHUTDOWN
    pIAPU->WaitCounter++;
#endif

    pIAPU->PC++;
}

void ApuAB ()
{
// INC dp
    Work8 = S9xAPUGetByteZ (OP1) + 1;
    S9xAPUSetByteZ (Work8, OP1);
    APUSetZN8 (Work8);

#ifdef SPC700_SHUTDOWN
    pIAPU->WaitCounter++;
#endif

    pIAPU->PC += 2;
}

void ApuAC ()
{
// INC abs
    Absolute ();
    Work8 = S9xAPUGetByte (pIAPU->Address) + 1;
    S9xAPUSetByte (Work8, pIAPU->Address);
    APUSetZN8 (Work8);

#ifdef SPC700_SHUTDOWN
    pIAPU->WaitCounter++;
#endif

    pIAPU->PC += 3;
}

void ApuBB ()
{
// INC dp+X
    Work8 = S9xAPUGetByteZ (OP1 + pIAPU->X) + 1;
    S9xAPUSetByteZ (Work8, OP1 + pIAPU->X);
    APUSetZN8 (Work8);

#ifdef SPC700_SHUTDOWN
    pIAPU->WaitCounter++;
#endif

    pIAPU->PC += 2;
}

void ApuBC ()
{
// INC A
    pIAPU->YA.B.A++;
    APUSetZN8 (pIAPU->YA.B.A);

#ifdef SPC700_SHUTDOWN
    pIAPU->WaitCounter++;
#endif

    pIAPU->PC++;
}

void Apu8B ()
{
// DEC dp
    Work8 = S9xAPUGetByteZ (OP1) - 1;
    S9xAPUSetByteZ (Work8, OP1);
    APUSetZN8 (Work8);

#ifdef SPC700_SHUTDOWN
    pIAPU->WaitCounter++;
#endif

    pIAPU->PC += 2;
}

void Apu8C ()
{
// DEC abs
    Absolute ();
    Work8 = S9xAPUGetByte (pIAPU->Address) - 1;
    S9xAPUSetByte (Work8, pIAPU->Address);
    APUSetZN8 (Work8);

#ifdef SPC700_SHUTDOWN
    pIAPU->WaitCounter++;
#endif

    pIAPU->PC += 3;
}

void Apu9B ()
{
// DEC dp+X
    Work8 = S9xAPUGetByteZ (OP1 + pIAPU->X) - 1;
    S9xAPUSetByteZ (Work8, OP1 + pIAPU->X);
    APUSetZN8 (Work8);

#ifdef SPC700_SHUTDOWN
    pIAPU->WaitCounter++;
#endif

    pIAPU->PC += 2;
}

void Apu9C ()
{
// DEC A
    pIAPU->YA.B.A--;
    APUSetZN8 (pIAPU->YA.B.A);

#ifdef SPC700_SHUTDOWN
    pIAPU->WaitCounter++;
#endif

    pIAPU->PC++;
}

void Apu44 ()
{
// EOR A,dp
    pIAPU->YA.B.A ^= S9xAPUGetByteZ (OP1);
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC += 2;
}

void Apu45 ()
{
// EOR A,abs
    Absolute ();
    pIAPU->YA.B.A ^= S9xAPUGetByte (pIAPU->Address);
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC += 3;
}

void Apu46 ()
{
// EOR A,(X)
    pIAPU->YA.B.A ^= S9xAPUGetByteZ (pIAPU->X);
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC++;
}

void Apu47 ()
{
// EOR A,(dp+X)
    IndexedXIndirect ();
    pIAPU->YA.B.A ^= S9xAPUGetByte (pIAPU->Address);
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC += 2;
}

void Apu48 ()
{
// EOR A,#00
    pIAPU->YA.B.A ^= OP1;
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC += 2;
}

void Apu49 ()
{
// EOR dp(dest),dp(src)
    Work8 = S9xAPUGetByteZ (OP1);
    Work8 ^= S9xAPUGetByteZ (OP2);
    S9xAPUSetByteZ (Work8, OP2);
    APUSetZN8 (Work8);
    pIAPU->PC += 3;
}

void Apu54 ()
{
// EOR A,dp+X
    pIAPU->YA.B.A ^= S9xAPUGetByteZ (OP1 + pIAPU->X);
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC += 2;
}

void Apu55 ()
{
// EOR A,abs+X
    AbsoluteX ();
    pIAPU->YA.B.A ^= S9xAPUGetByte (pIAPU->Address);
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC += 3;
}

void Apu56 ()
{
// EOR A,abs+Y
    AbsoluteY ();
    pIAPU->YA.B.A ^= S9xAPUGetByte (pIAPU->Address);
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC += 3;
}

void Apu57 ()
{
// EOR A,(dp)+Y
    IndirectIndexedY ();
    pIAPU->YA.B.A ^= S9xAPUGetByte (pIAPU->Address);
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC += 2;
}

void Apu58 ()
{
// EOR dp,#00
    Work8 = OP1;
    Work8 ^= S9xAPUGetByteZ (OP2);
    S9xAPUSetByteZ (Work8, OP2);
    APUSetZN8 (Work8);
    pIAPU->PC += 3;
}

void Apu59 ()
{
// EOR (X),(Y)
    Work8 = S9xAPUGetByteZ (pIAPU->X) ^ S9xAPUGetByteZ (pIAPU->YA.B.Y);
    APUSetZN8 (Work8);
    S9xAPUSetByteZ (Work8, pIAPU->X);
    pIAPU->PC++;
}

void Apu4B ()
{
// LSR dp
    Work8 = S9xAPUGetByteZ (OP1);
    LSR (Work8);
    S9xAPUSetByteZ (Work8, OP1);
    pIAPU->PC += 2;
}

void Apu4C ()
{
// LSR abs
    Absolute ();
    Work8 = S9xAPUGetByte (pIAPU->Address);
    LSR (Work8);
    S9xAPUSetByte (Work8, pIAPU->Address);
    pIAPU->PC += 3;
}

void Apu5B ()
{
// LSR dp+X
    Work8 = S9xAPUGetByteZ (OP1 + pIAPU->X);
    LSR (Work8);
    S9xAPUSetByteZ (Work8, OP1 + pIAPU->X);
    pIAPU->PC += 2;
}

void Apu5C ()
{
// LSR A
    LSR (pIAPU->YA.B.A);
    pIAPU->PC++;
}

void Apu7D ()
{
// MOV A,X
    pIAPU->YA.B.A = pIAPU->X;
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC++;
}

void ApuDD ()
{
// MOV A,Y
    pIAPU->YA.B.A = pIAPU->YA.B.Y;
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC++;
}

void Apu5D ()
{
// MOV X,A
    pIAPU->X = pIAPU->YA.B.A;
    APUSetZN8 (pIAPU->X);
    pIAPU->PC++;
}

void ApuFD ()
{
// MOV Y,A
    pIAPU->YA.B.Y = pIAPU->YA.B.A;
    APUSetZN8 (pIAPU->YA.B.Y);
    pIAPU->PC++;
}

void Apu9D ()
{
//MOV X,SP
    pIAPU->X = pIAPU->S;
    APUSetZN8 (pIAPU->X);
    pIAPU->PC++;
}

void ApuBD ()
{
// MOV SP,X
    pIAPU->S = pIAPU->X;
    pIAPU->PC++;
}

void Apu6B ()
{
// ROR dp
    Work8 = S9xAPUGetByteZ (OP1);
    ROR (Work8);
    S9xAPUSetByteZ (Work8, OP1);
    pIAPU->PC += 2;
}

void Apu6C ()
{
// ROR abs
    Absolute ();
    Work8 = S9xAPUGetByte (pIAPU->Address);
    ROR (Work8);
    S9xAPUSetByte (Work8, pIAPU->Address);
    pIAPU->PC += 3;
}

void Apu7B ()
{
// ROR dp+X
    Work8 = S9xAPUGetByteZ (OP1 + pIAPU->X);
    ROR (Work8);
    S9xAPUSetByteZ (Work8, OP1 + pIAPU->X);
    pIAPU->PC += 2;
}

void Apu7C ()
{
// ROR A
    ROR (pIAPU->YA.B.A);
    pIAPU->PC++;
}

void Apu6E ()
{
// DBNZ dp,rel
    Work8 = OP1;
    Relative2 ();
    W1 = S9xAPUGetByteZ (Work8) - 1;
    S9xAPUSetByteZ (W1, Work8);
    if (W1 != 0)
    {
	pIAPU->PC = pIAPU->RAM + (uint16) Int16;
	CPU.APU_Cycles += pIAPU->TwoCycles;
    }
    else
	pIAPU->PC += 3;
}

void ApuFE ()
{
// DBNZ Y,rel
    Relative ();
    pIAPU->YA.B.Y--;
    if (pIAPU->YA.B.Y != 0)
    {
	pIAPU->PC = pIAPU->RAM + (uint16) Int16;
	CPU.APU_Cycles += pIAPU->TwoCycles;
    }
    else
	pIAPU->PC += 2;
}

void Apu6F ()
{
// RET
    PopW (Work16);
    pIAPU->PC = pIAPU->RAM + Work16;
}

void Apu7F ()
{
// RETI
    // STOP ("RETI");
    Pop (pIAPU->P);
    S9xAPUUnpackStatus ();
    PopW (Work16);
    pIAPU->PC = pIAPU->RAM + Work16;
}

void Apu84 ()
{
// ADC A,dp
    Work8 = S9xAPUGetByteZ (OP1);
    ADC (pIAPU->YA.B.A, Work8);
    pIAPU->PC += 2;
}

void Apu85 ()
{
// ADC A, abs
    Absolute ();
    Work8 = S9xAPUGetByte (pIAPU->Address);
    ADC (pIAPU->YA.B.A, Work8);
    pIAPU->PC += 3;
}

void Apu86 ()
{
// ADC A,(X)
    Work8 = S9xAPUGetByteZ (pIAPU->X);
    ADC (pIAPU->YA.B.A, Work8);
    pIAPU->PC++;
}

void Apu87 ()
{
// ADC A,(dp+X)
    IndexedXIndirect ();
    Work8 = S9xAPUGetByte (pIAPU->Address);
    ADC (pIAPU->YA.B.A, Work8);
    pIAPU->PC += 2;
}

void Apu88 ()
{
// ADC A,#00
    Work8 = OP1;
    ADC (pIAPU->YA.B.A, Work8);
    pIAPU->PC += 2;
}

void Apu89 ()
{
// ADC dp(dest),dp(src)
    Work8 = S9xAPUGetByteZ (OP1);
    W1 = S9xAPUGetByteZ (OP2);
    ADC (W1, Work8);
    S9xAPUSetByteZ (W1, OP2);
    pIAPU->PC += 3;
}

void Apu94 ()
{
// ADC A,dp+X
    Work8 = S9xAPUGetByteZ (OP1 + pIAPU->X);
    ADC (pIAPU->YA.B.A, Work8);
    pIAPU->PC += 2;
}

void Apu95 ()
{
// ADC A, abs+X
    AbsoluteX ();
    Work8 = S9xAPUGetByte (pIAPU->Address);
    ADC (pIAPU->YA.B.A, Work8);
    pIAPU->PC += 3;
}

void Apu96 ()
{
// ADC A, abs+Y
    AbsoluteY ();
    Work8 = S9xAPUGetByte (pIAPU->Address);
    ADC (pIAPU->YA.B.A, Work8);
    pIAPU->PC += 3;
}

void Apu97 ()
{
// ADC A, (dp)+Y
    IndirectIndexedY ();
    Work8 = S9xAPUGetByte (pIAPU->Address);
    ADC (pIAPU->YA.B.A, Work8);
    pIAPU->PC += 2;
}

void Apu98 ()
{
// ADC dp,#00
    Work8 = OP1;
    W1 = S9xAPUGetByteZ (OP2);
    ADC (W1, Work8);
    S9xAPUSetByteZ (W1, OP2);
    pIAPU->PC += 3;
}

void Apu99 ()
{
// ADC (X),(Y)
    W1 = S9xAPUGetByteZ (pIAPU->X);
    Work8 = S9xAPUGetByteZ (pIAPU->YA.B.Y);
    ADC (W1, Work8);
    S9xAPUSetByteZ (W1, pIAPU->X);
    pIAPU->PC++;
}

void Apu8D ()
{
// MOV Y,#00
    pIAPU->YA.B.Y = OP1;
    APUSetZN8 (pIAPU->YA.B.Y);
    pIAPU->PC += 2;
}

void Apu8F ()
{
// MOV dp,#00
    Work8 = OP1;
    S9xAPUSetByteZ (Work8, OP2);
    pIAPU->PC += 3;
}

void Apu9E ()
{
// DIV YA,X
    if (pIAPU->X == 0)
    {
	APUSetOverflow ();
	pIAPU->YA.B.Y = 0xff;
	pIAPU->YA.B.A = 0xff;
    }
    else
    {
	APUClearOverflow ();
	Work8 = pIAPU->YA.W / pIAPU->X;
	pIAPU->YA.B.Y = pIAPU->YA.W % pIAPU->X;
	pIAPU->YA.B.A = Work8;
    }
// XXX How should Overflow, Half Carry, Zero and Negative flags be set??
    // APUSetZN16 (pIAPU->YA.W);
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC++;
}

void Apu9F ()
{
// XCN A
    pIAPU->YA.B.A = (pIAPU->YA.B.A >> 4) | (pIAPU->YA.B.A << 4);
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC++;
}

void ApuA4 ()
{
// SBC A, dp
    Work8 = S9xAPUGetByteZ (OP1);
    SBC (pIAPU->YA.B.A, Work8);
    pIAPU->PC += 2;
}

void ApuA5 ()
{
// SBC A, abs
    Absolute ();
    Work8 = S9xAPUGetByte (pIAPU->Address);
    SBC (pIAPU->YA.B.A, Work8);
    pIAPU->PC += 3;
}

void ApuA6 ()
{
// SBC A, (X)
    Work8 = S9xAPUGetByteZ (pIAPU->X);
    SBC (pIAPU->YA.B.A, Work8);
    pIAPU->PC++;
}

void ApuA7 ()
{
// SBC A,(dp+X)
    IndexedXIndirect ();
    Work8 = S9xAPUGetByte (pIAPU->Address);
    SBC (pIAPU->YA.B.A, Work8);
    pIAPU->PC += 2;
}

void ApuA8 ()
{
// SBC A,#00
    Work8 = OP1;
    SBC (pIAPU->YA.B.A, Work8);
    pIAPU->PC += 2;
}

void ApuA9 ()
{
// SBC dp(dest), dp(src)
    Work8 = S9xAPUGetByteZ (OP1);
    W1 = S9xAPUGetByteZ (OP2);
    SBC (W1, Work8);
    S9xAPUSetByteZ (W1, OP2);
    pIAPU->PC += 3;
}

void ApuB4 ()
{
// SBC A, dp+X
    Work8 = S9xAPUGetByteZ (OP1 + pIAPU->X);
    SBC (pIAPU->YA.B.A, Work8);
    pIAPU->PC += 2;
}

void ApuB5 ()
{
// SBC A,abs+X
    AbsoluteX ();
    Work8 = S9xAPUGetByte (pIAPU->Address);
    SBC (pIAPU->YA.B.A, Work8);
    pIAPU->PC += 3;
}

void ApuB6 ()
{
// SBC A,abs+Y
    AbsoluteY ();
    Work8 = S9xAPUGetByte (pIAPU->Address);
    SBC (pIAPU->YA.B.A, Work8);
    pIAPU->PC += 3;
}

void ApuB7 ()
{
// SBC A,(dp)+Y
    IndirectIndexedY ();
    Work8 = S9xAPUGetByte (pIAPU->Address);
    SBC (pIAPU->YA.B.A, Work8);
    pIAPU->PC += 2;
}

void ApuB8 ()
{
// SBC dp,#00
    Work8 = OP1;
    W1 = S9xAPUGetByteZ (OP2);
    SBC (W1, Work8);
    S9xAPUSetByteZ (W1, OP2);
    pIAPU->PC += 3;
}

void ApuB9 ()
{
// SBC (X),(Y)
    W1 = S9xAPUGetByteZ (pIAPU->X);
    Work8 = S9xAPUGetByteZ (pIAPU->YA.B.Y);
    SBC (W1, Work8);
    S9xAPUSetByteZ (W1, pIAPU->X);
    pIAPU->PC++;
}

void ApuAF ()
{
// MOV (X)+, A
    S9xAPUSetByteZ (pIAPU->YA.B.A, pIAPU->X++);
    pIAPU->PC++;
}

void ApuBE ()
{
// DAS
    if ((pIAPU->YA.B.A & 0x0f) > 9 || !APUCheckHalfCarry())
   {
        pIAPU->YA.B.A -= 6;
   }
    if (pIAPU->YA.B.A > 0x9f || !pIAPU->_Carry)
   {
	pIAPU->YA.B.A -= 0x60;
	APUClearCarry ();
   }
    else { APUSetCarry (); }
    APUSetZN8 (pIAPU->YA.B.A);
   pIAPU->PC++;
}

void ApuBF ()
{
// MOV A,(X)+
    pIAPU->YA.B.A = S9xAPUGetByteZ (pIAPU->X++);
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC++;
}

void ApuC0 ()
{
// DI
    APUClearInterrupt ();
    pIAPU->PC++;
}

void ApuA0 ()
{
// EI
    APUSetInterrupt ();
    pIAPU->PC++;
}

void ApuC4 ()
{
// MOV dp,A
    S9xAPUSetByteZ (pIAPU->YA.B.A, OP1);
    pIAPU->PC += 2;
}

void ApuC5 ()
{
// MOV abs,A
    Absolute ();
    S9xAPUSetByte (pIAPU->YA.B.A, pIAPU->Address);
    pIAPU->PC += 3;
}

void ApuC6 ()
{
// MOV (X), A
    S9xAPUSetByteZ (pIAPU->YA.B.A, pIAPU->X);
    pIAPU->PC++;
}

void ApuC7 ()
{
// MOV (dp+X),A
    IndexedXIndirect ();
    S9xAPUSetByte (pIAPU->YA.B.A, pIAPU->Address);
    pIAPU->PC += 2;
}

void ApuC9 ()
{
// MOV abs,X
    Absolute ();
    S9xAPUSetByte (pIAPU->X, pIAPU->Address);
    pIAPU->PC += 3;
}

void ApuCB ()
{
// MOV dp,Y
    S9xAPUSetByteZ (pIAPU->YA.B.Y, OP1);
    pIAPU->PC += 2;
}

void ApuCC ()
{
// MOV abs,Y
    Absolute ();
    S9xAPUSetByte (pIAPU->YA.B.Y, pIAPU->Address);
    pIAPU->PC += 3;
}

void ApuCD ()
{
// MOV X,#00
    pIAPU->X = OP1;
    APUSetZN8 (pIAPU->X);
    pIAPU->PC += 2;
}

void ApuCF ()
{
// MUL YA
    pIAPU->YA.W = (uint16) pIAPU->YA.B.A * pIAPU->YA.B.Y;
    APUSetZN16 (pIAPU->YA.W);
    pIAPU->PC++;
}

void ApuD4 ()
{
// MOV dp+X, A
    S9xAPUSetByteZ (pIAPU->YA.B.A, OP1 + pIAPU->X);
    pIAPU->PC += 2;
}

void ApuD5 ()
{
// MOV abs+X,A
    AbsoluteX ();
    S9xAPUSetByte (pIAPU->YA.B.A, pIAPU->Address);
    pIAPU->PC += 3;
}

void ApuD6 ()
{
// MOV abs+Y,A
    AbsoluteY ();
    S9xAPUSetByte (pIAPU->YA.B.A, pIAPU->Address);
    pIAPU->PC += 3;
}

void ApuD7 ()
{
// MOV (dp)+Y,A
    IndirectIndexedY ();
    S9xAPUSetByte (pIAPU->YA.B.A, pIAPU->Address);
    pIAPU->PC += 2;
}

void ApuD8 ()
{
// MOV dp,X
    S9xAPUSetByteZ (pIAPU->X, OP1);
    pIAPU->PC += 2;
}

void ApuD9 ()
{
// MOV dp+Y,X
    S9xAPUSetByteZ (pIAPU->X, OP1 + pIAPU->YA.B.Y);
    pIAPU->PC += 2;
}

void ApuDB ()
{
// MOV dp+X,Y
    S9xAPUSetByteZ (pIAPU->YA.B.Y, OP1 + pIAPU->X);
    pIAPU->PC += 2;
}

void ApuDF ()
{
// DAA
    if ((pIAPU->YA.B.A & 0x0f) > 9 || APUCheckHalfCarry())
    {
        if(pIAPU->YA.B.A > 0xf0) APUSetCarry ();
        pIAPU->YA.B.A += 6;
	//APUSetHalfCarry (); Intel procs do this, but this is a Sony proc...
    }
    //else { APUClearHalfCarry (); } ditto as above
    if (pIAPU->YA.B.A > 0x9f || pIAPU->_Carry)
    {
	pIAPU->YA.B.A += 0x60;
	APUSetCarry ();
    }
    else { APUClearCarry (); }
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC++;
}

void ApuE4 ()
{
// MOV A, dp
    pIAPU->YA.B.A = S9xAPUGetByteZ (OP1);
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC += 2;
}

void ApuE5 ()
{
// MOV A,abs
    Absolute ();
    pIAPU->YA.B.A = S9xAPUGetByte (pIAPU->Address);
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC += 3;
}

void ApuE6 ()
{
// MOV A,(X)
    pIAPU->YA.B.A = S9xAPUGetByteZ (pIAPU->X);
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC++;
}

void ApuE7 ()
{
// MOV A,(dp+X)
    IndexedXIndirect ();
    pIAPU->YA.B.A = S9xAPUGetByte (pIAPU->Address);
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC += 2;
}

void ApuE8 ()
{
// MOV A,#00
    pIAPU->YA.B.A = OP1;
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC += 2;
}

void ApuE9 ()
{
// MOV X, abs
    Absolute ();
    pIAPU->X = S9xAPUGetByte (pIAPU->Address);
    APUSetZN8 (pIAPU->X);
    pIAPU->PC += 3;
}

void ApuEB ()
{
// MOV Y,dp
    pIAPU->YA.B.Y = S9xAPUGetByteZ (OP1);
    APUSetZN8 (pIAPU->YA.B.Y);
    pIAPU->PC += 2;
}

void ApuEC ()
{
// MOV Y,abs
    Absolute ();
    pIAPU->YA.B.Y = S9xAPUGetByte (pIAPU->Address);
    APUSetZN8 (pIAPU->YA.B.Y);
    pIAPU->PC += 3;
}

void ApuF4 ()
{
// MOV A, dp+X
    pIAPU->YA.B.A = S9xAPUGetByteZ (OP1 + pIAPU->X);
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC += 2;
}

void ApuF5 ()
{
// MOV A, abs+X
    AbsoluteX ();
    pIAPU->YA.B.A = S9xAPUGetByte (pIAPU->Address);
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC += 3;
}

void ApuF6 ()
{
// MOV A, abs+Y
    AbsoluteY ();
    pIAPU->YA.B.A = S9xAPUGetByte (pIAPU->Address);
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC += 3;
}

void ApuF7 ()
{
// MOV A, (dp)+Y
    IndirectIndexedY ();
    pIAPU->YA.B.A = S9xAPUGetByte (pIAPU->Address);
    APUSetZN8 (pIAPU->YA.B.A);
    pIAPU->PC += 2;
}

void ApuF8 ()
{
// MOV X,dp
    pIAPU->X = S9xAPUGetByteZ (OP1);
    APUSetZN8 (pIAPU->X);
    pIAPU->PC += 2;
}

void ApuF9 ()
{
// MOV X,dp+Y
    pIAPU->X = S9xAPUGetByteZ (OP1 + pIAPU->YA.B.Y);
    APUSetZN8 (pIAPU->X);
    pIAPU->PC += 2;
}

void ApuFA ()
{
// MOV dp(dest),dp(src)
    S9xAPUSetByteZ (S9xAPUGetByteZ (OP1), OP2);
    pIAPU->PC += 3;
}

void ApuFB ()
{
// MOV Y,dp+X
    pIAPU->YA.B.Y = S9xAPUGetByteZ (OP1 + pIAPU->X);
    APUSetZN8 (pIAPU->YA.B.Y);
    pIAPU->PC += 2;
}

//#if defined(ASM_SPC700)
#undef INLINE
#define INLINE extern "C"
#include "apumem.h"
/*
#elif defined(NO_INLINE_SET_GET)
#undef INLINE
#define INLINE
#include "apumem.h"
#endif
*/

void (*S9xApuOpcodes[256]) (void) =
{
	Apu00, Apu01, Apu02, Apu03, Apu04, Apu05, Apu06, Apu07,
	Apu08, Apu09, Apu0A, Apu0B, Apu0C, Apu0D, Apu0E, Apu0F,
	Apu10, Apu11, Apu12, Apu13, Apu14, Apu15, Apu16, Apu17,
	Apu18, Apu19, Apu1A, Apu1B, Apu1C, Apu1D, Apu1E, Apu1F,
	Apu20, Apu21, Apu22, Apu23, Apu24, Apu25, Apu26, Apu27,
	Apu28, Apu29, Apu2A, Apu2B, Apu2C, Apu2D, Apu2E, Apu2F,
	Apu30, Apu31, Apu32, Apu33, Apu34, Apu35, Apu36, Apu37,
	Apu38, Apu39, Apu3A, Apu3B, Apu3C, Apu3D, Apu3E, Apu3F,
	Apu40, Apu41, Apu42, Apu43, Apu44, Apu45, Apu46, Apu47,
	Apu48, Apu49, Apu4A, Apu4B, Apu4C, Apu4D, Apu4E, Apu4F,
	Apu50, Apu51, Apu52, Apu53, Apu54, Apu55, Apu56, Apu57,
	Apu58, Apu59, Apu5A, Apu5B, Apu5C, Apu5D, Apu5E, Apu5F,
	Apu60, Apu61, Apu62, Apu63, Apu64, Apu65, Apu66, Apu67,
	Apu68, Apu69, Apu6A, Apu6B, Apu6C, Apu6D, Apu6E, Apu6F,
	Apu70, Apu71, Apu72, Apu73, Apu74, Apu75, Apu76, Apu77,
	Apu78, Apu79, Apu7A, Apu7B, Apu7C, Apu7D, Apu7E, Apu7F,
	Apu80, Apu81, Apu82, Apu83, Apu84, Apu85, Apu86, Apu87,
	Apu88, Apu89, Apu8A, Apu8B, Apu8C, Apu8D, Apu8E, Apu8F,
	Apu90, Apu91, Apu92, Apu93, Apu94, Apu95, Apu96, Apu97,
	Apu98, Apu99, Apu9A, Apu9B, Apu9C, Apu9D, Apu9E, Apu9F,
	ApuA0, ApuA1, ApuA2, ApuA3, ApuA4, ApuA5, ApuA6, ApuA7,
	ApuA8, ApuA9, ApuAA, ApuAB, ApuAC, ApuAD, ApuAE, ApuAF,
	ApuB0, ApuB1, ApuB2, ApuB3, ApuB4, ApuB5, ApuB6, ApuB7,
	ApuB8, ApuB9, ApuBA, ApuBB, ApuBC, ApuBD, ApuBE, ApuBF,
	ApuC0, ApuC1, ApuC2, ApuC3, ApuC4, ApuC5, ApuC6, ApuC7,
	ApuC8, ApuC9, ApuCA, ApuCB, ApuCC, ApuCD, ApuCE, ApuCF,
	ApuD0, ApuD1, ApuD2, ApuD3, ApuD4, ApuD5, ApuD6, ApuD7,
	ApuD8, ApuD9, ApuDA, ApuDB, ApuDC, ApuDD, ApuDE, ApuDF,
	ApuE0, ApuE1, ApuE2, ApuE3, ApuE4, ApuE5, ApuE6, ApuE7,
	ApuE8, ApuE9, ApuEA, ApuEB, ApuEC, ApuED, ApuEE, ApuEF,
	ApuF0, ApuF1, ApuF2, ApuF3, ApuF4, ApuF5, ApuF6, ApuF7,
	ApuF8, ApuF9, ApuFA, ApuFB, ApuFC, ApuFD, ApuFE, ApuFF
};


struct SIAPU IAPU2;
struct SIAPU *pIAPU;

void APUCompare()
{
	IAPU.icount++;

	if(IAPU.PC != IAPU2.PC) {
		dprintf("!%02X %5i PC %08X vs %08X", IAPU.opcode, IAPU.icount, IAPU.PC, IAPU2.PC);
		exit(1);
	}

	if(IAPU.YA.W != IAPU2.YA.W) {
		dprintf("!%02X %5i YA %04X vs %04X", IAPU.opcode, IAPU.icount, IAPU.YA.W, IAPU2.YA.W);
		dprintf("  (%04X / %02X)", IAPU.ya_prev, IAPU.x_prev);
		exit(1);
	}

	if((IAPU.P&0x7d) != (IAPU2.P&0x7d)) {
		dprintf("!%02X %5i P %02X vs %02X", IAPU.opcode, IAPU.icount, IAPU.P, IAPU2.P);
		exit(1);
	}

	if(IAPU.X != IAPU2.X) {
		dprintf("!%02X %5i X %02X vs %02X", IAPU.opcode, IAPU.icount, IAPU.X, IAPU2.X);
		exit(1);
	}

	if(IAPU.S != IAPU2.S) {
		dprintf("!%02X %5i S %02X vs %02X", IAPU.opcode, IAPU.icount, IAPU.S, IAPU2.S);
		exit(1);
	}

	if((IAPU._Zero == 0) != (IAPU2._Zero == 0)) {
		dprintf("!%02X %5i _Zero %02X vs %02X", IAPU.opcode, IAPU.icount, IAPU._Zero, IAPU2._Zero);
		exit(1);
	}

	if((IAPU._Zero & 0x80) != (IAPU2._Zero & 0x80)) {
		dprintf("!%02X %5i _Zero(n) %02X vs %02X", IAPU.opcode, IAPU.icount, IAPU._Zero, IAPU2._Zero);
		exit(1);
	}

	if(IAPU.memread_addr != IAPU2.memread_addr) {
		dprintf("!%02X %5i memread_addr %04X vs %04X", IAPU.opcode, IAPU.icount, IAPU.memread_addr, IAPU2.memread_addr);
		exit(1);
	}

	if(IAPU.memread_data != IAPU2.memread_data) {
		dprintf("!%02X %5i memread_data %02X@%04X vs %02X@%04X", IAPU.opcode, IAPU.icount, IAPU.memread_data, IAPU.memread_addr, IAPU2.memread_data, IAPU2.memread_addr);
		exit(1);
	}

	if(IAPU.memwrite_addr != IAPU2.memwrite_addr) {
		dprintf("!%02X %5i memwrite_addr %04X vs %04X", IAPU.opcode, IAPU.icount, IAPU.memwrite_addr, IAPU2.memwrite_addr);
		exit(1);
	}

	if(IAPU.memwrite_data != IAPU2.memwrite_data) {
		dprintf("!%02X %5i memwrite_data %02X@%04X vs %02X@%04X", IAPU.opcode, IAPU.icount, IAPU.memwrite_data, IAPU.memwrite_addr, IAPU2.memwrite_data, IAPU2.memwrite_addr);
		exit(1);
	}
}


