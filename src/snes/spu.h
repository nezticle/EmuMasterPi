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
#ifndef SNESSPU_H
#define SNESSPU_H

#include "spc700.h"
#include "port.h"
#include <base/emu.h>

extern void snesSpuSl();

struct SIAPU
{
	u8  *DirectPage;        // 0x00
	u32 Address;            // 0x04  c core only
	u8  *WaitAddress1;      // 0x08
	u8  *WaitAddress2;      // 0x0C
	u32 WaitCounter;        // 0x10
	u8  *ShadowRAM;         // 0x14
	u8  *CachedSamples;     // 0x18
	u8  _Carry;             // 0x1C  c core only
	u8  _Overflow;          // 0x1D  c core only
	u8  Bit;                // 0x1E  c core only
	u8  pad0;
	u32 TimerErrorCounter;  // 0x20
	u32 Scanline;           // 0x24
	s32  OneCycle;           // 0x28
	s32  TwoCycles;          // 0x2C
    // notaz: reordered and moved everything here, for faster context load/save
	u32 *asmJumpTab;        // 0x30
	u8  *PC;                // 0x34
    YAndA  YA;                 // 0x38  0x0000YYAA
	u8  P;                  // 0x3C  flags: NODBHIZC
	u8  pad1;
	u8  pad2;
	u8  _Zero;              // 0x3F  Z=0, when this!=0; also stores neg flag in &0x80
	u8  X;                  // 0x40
	u8  S;                  // 0x41  stack pointer, default: 0xff
	u16 pad3;
	u8  *RAM;               // 0x44

	u8  *ExtraRAM;          // 0x48  shortcut to APU.ExtraRAM
};

struct SAPU
{
	s32  Cycles;
    bool8  ShowROM;
	u8  Flags;
	u8  KeyedChannels;
	u8  OutPorts [4];
	u8  DSP [0x80];
	u8  ExtraRAM [64];
	u16 Timer [3];
	u16 TimerTarget [3];
    bool8  TimerEnabled [3];
    bool8  TimerValueWritten [3];
};

EXTERN_C struct SAPU APU;
EXTERN_C struct SIAPU IAPU;

STATIC inline void S9xAPUUnpackStatus()
{
    IAPU._Zero     =((IAPU.P & Zero) == 0) | (IAPU.P & Negative);

#ifndef ASM_SPC700
	IAPU._Carry    = (IAPU.P & Carry);
    IAPU._Overflow = (IAPU.P & Overflow);
#endif
}

STATIC inline void S9xAPUPackStatus()
{
#ifdef ASM_SPC700
    IAPU.P &= ~(Zero | Negative);
    if(!IAPU._Zero)       IAPU.P |= Zero;
    if(IAPU._Zero & 0x80) IAPU.P |= Negative;
#else
    IAPU.P &= ~(Zero | Negative | Carry | Overflow);
    if(IAPU._Carry)       IAPU.P |= Carry;
    if(!IAPU._Zero)       IAPU.P |= Zero;
    if(IAPU._Overflow)    IAPU.P |= Overflow;
    if(IAPU._Zero & 0x80) IAPU.P |= Negative;
#endif
}

START_EXTERN_C
void S9xResetAPU (void);
bool8 S9xInitAPU ();
void S9xDeinitAPU ();
void S9xDecacheSamples ();
int S9xTraceAPU ();
int S9xAPUOPrint (char *buffer, u16 Address);
void S9xSetAPUControl (u8 byte);
void S9xSetAPUDSP (u8 byte);
u8 S9xGetAPUDSP ();
void S9xSetAPUTimer (u16 Address, u8 byte);
void S9xOpenCloseSoundTracingFile (bool8);
void S9xPrintAPUState ();
extern s32 S9xAPUCycles [256];	// Scaled cycle lengths
extern s32 S9xAPUCycleLengths [256];	// Raw data.
extern void (*S9xApuOpcodes [256]) (void);
extern void (*S9xApuOpcodesReal [256]) (void);
END_EXTERN_C


#define APU_VOL_LEFT 0x00
#define APU_VOL_RIGHT 0x01
#define APU_P_LOW 0x02
#define APU_P_HIGH 0x03
#define APU_SRCN 0x04
#define APU_ADSR1 0x05
#define APU_ADSR2 0x06
#define APU_GAIN 0x07
#define APU_ENVX 0x08
#define APU_OUTX 0x09

#define APU_MVOL_LEFT 0x0c
#define APU_MVOL_RIGHT 0x1c
#define APU_EVOL_LEFT 0x2c
#define APU_EVOL_RIGHT 0x3c
#define APU_KON 0x4c
#define APU_KOFF 0x5c
#define APU_FLG 0x6c
#define APU_ENDX 0x7c

#define APU_EFB 0x0d
#define APU_PMON 0x2d
#define APU_NON 0x3d
#define APU_EON 0x4d
#define APU_DIR 0x5d
#define APU_ESA 0x6d
#define APU_EDL 0x7d

#define APU_C0 0x0f
#define APU_C1 0x1f
#define APU_C2 0x2f
#define APU_C3 0x3f
#define APU_C4 0x4f
#define APU_C5 0x5f
#define APU_C6 0x6f
#define APU_C7 0x7f

#define APU_SOFT_RESET 0x80
#define APU_MUTE 0x40
#define APU_ECHO_DISABLED 0x20

#define FREQUENCY_MASK 0x3fff

#endif // SNESSPU_H
