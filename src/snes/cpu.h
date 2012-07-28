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
#ifndef SNESCPU_H
#define SNESCPU_H
#include "ppu.h"
#include "mem.h"
#include "65c816.h"
#include <base/emu.h>

extern void snesCpuSl();

#define DO_HBLANK_CHECK() \
    if (CPU.Cycles >= CPU.NextEvent) \
	S9xDoHBlankProcessing ();

struct SOpcodes {
#ifdef __WIN32__
	void (__cdecl *S9xOpcode)( void);
#else
	void (*S9xOpcode)( void);
#endif
};

struct SICPU
{
#ifndef CPU_ASM
	u8  *Speed;
	u8  _Carry;
	u8  _Zero;
	u8  _Negative;
	u8  _Overflow;
    struct SOpcodes *S9xOpcodes;
#endif
    
	bool8  CPUExecuting;
	u32 ShiftedPB;
	u32 ShiftedDB;
	u32 Frame;
	u32 Scanline;
	u32 FrameAdvanceCount;
};

void S9xMainLoop();
void S9xReset();
void S9xSetIRQ(u32);
extern struct SICPU ICPU;

START_EXTERN_C
void S9xDoHBlankProcessing();
END_EXTERN_C

STATIC inline void S9xClearIRQ(u32 M) {
    CPU.IRQActive &= ~M;
    if (!CPU.IRQActive)
	CPU.Flags &= ~IRQ_PENDING_FLAG;
}

STATIC inline void S9xReschedule ()
{
	u8 which;
    long max;
    
    if (CPU.WhichEvent == HBLANK_START_EVENT ||
	CPU.WhichEvent == HTIMER_AFTER_EVENT)
    {
	which = HBLANK_END_EVENT;
	max = Settings.H_Max;
    }
    else
    {
	which = HBLANK_START_EVENT;
	max = Settings.HBlankStart;
    }

    if (PPU.HTimerEnabled &&
        (long) PPU.HTimerPosition < max &&
	(long) PPU.HTimerPosition > CPU.NextEvent &&
	(!PPU.VTimerEnabled ||
	 (PPU.VTimerEnabled && CPU.V_Counter == PPU.IRQVBeamPos)))
    {
	which = (long) PPU.HTimerPosition < Settings.HBlankStart ?
			HTIMER_BEFORE_EVENT : HTIMER_AFTER_EVENT;
	max = PPU.HTimerPosition;
    }
    CPU.NextEvent = max;
    CPU.WhichEvent = which;
}

#endif // SNESCPU_H
