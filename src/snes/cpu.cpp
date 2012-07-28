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
#include "mem.h"
#include "ppu.h"
#include "dsp1.h"
#include "cpu.h"
#include "debug.h"
#include "spu.h"
#include "dma.h"
#include "port.h"
#ifdef USE_SA1
#include "sa1.h"
#endif
//#include "cheats.h"
#include "srtc.h"
#include "sdd1.h"
//#include "spc7110.h"

#include "soundux.h"
#include "missing.h"

#ifdef SUPER_FX
#include "fxemu.h"
#include <QDataStream>

extern struct FxInit_s SuperFX;

void S9xResetSuperFX ()
{
    SuperFX.vFlags = 0; //FX_FLAG_ROM_BUFFER;// | FX_FLAG_ADDRESS_CHECKING;
    FxReset (&SuperFX);
}
#endif

#include "os9x_asm_cpu.h"

void S9xResetCPU ()
{
    Registers.PB = 0;
    Registers.PC = S9xGetWord (0xFFFC);
    Registers.D.W = 0;
    Registers.DB = 0;
    Registers.SH = 1;
    Registers.SL = 0xFF;
    Registers.XH = 0;
    Registers.YH = 0;
    Registers.P.W = 0;

    ICPU.ShiftedPB = 0;
    ICPU.ShiftedDB = 0;
    SetFlags (MemoryFlag | IndexFlag | IRQ | Emulation);
    ClearFlags (Decimal);

    CPU.Flags = CPU.Flags & (DEBUG_MODE_FLAG | TRACE_FLAG);
	CPU.BranchSkip = false;
    CPU.NMIActive = FALSE;
    CPU.IRQActive = FALSE;
    CPU.WaitingForInterrupt = FALSE;
    CPU.InDMA = FALSE;
    CPU.WhichEvent = HBLANK_START_EVENT;
    CPU.PC = NULL;
    CPU.PCBase = NULL;
    CPU.PCAtOpcodeStart = NULL;
    CPU.WaitAddress = NULL;
    CPU.WaitCounter = 0;
    CPU.Cycles = 0;
    CPU.NextEvent = Settings.HBlankStart;
    CPU.V_Counter = 0;
    CPU.MemSpeed = SLOW_ONE_CYCLE;
    CPU.MemSpeedx2 = SLOW_ONE_CYCLE * 2;
    CPU.FastROMSpeed = SLOW_ONE_CYCLE;
    CPU.AutoSaveTimer = 0;
    CPU.SRAMModified = FALSE;
    // CPU.NMITriggerPoint = 4; // Set when ROM image loaded
    CPU.BRKTriggered = FALSE;
    //CPU.TriedInterleavedMode2 = FALSE; // Reset when ROM image loaded
    CPU.NMICycleCount = 0;
    CPU.IRQCycleCount = 0;
    S9xSetPCBase (Registers.PC);

    ICPU.CPUExecuting = TRUE;
}


void S9xReset (void)
{
#ifdef SUPER_FX
    if (Settings.SuperFX)
        S9xResetSuperFX ();
#endif

    ZeroMemory (Memory.FillRAM, 0x8000);
    memset (Memory.VRAM, 0x00, 0x10000);
    memset (Memory.RAM, 0x55, 0x20000);

/*	if(Settings.SPC7110)
		S9xSpc7110Reset();*/
    S9xResetCPU ();
    S9xResetPPU ();
    S9xResetSRTC ();
    if (Settings.SDD1)
        S9xResetSDD1 ();

    S9xResetDMA ();
    S9xResetAPU ();
    S9xResetDSP1 ();
#ifdef USE_SA1
    S9xSA1Init ();
#endif

	if (Settings.C4)
		S9xInitC4 ();

	S9xResetSound(1);

    Settings.Paused = FALSE;
    
       //Init CPU Map & co
	CPU.Memory_Map=(u8*)&(Memory.Map);
	CPU.Memory_WriteMap=(u8*)&(Memory.WriteMap);
	CPU.Memory_MemorySpeed=(u8*)&(Memory.MemorySpeed);
	CPU.Memory_BlockIsRAM=(u8*)&(Memory.BlockIsRAM);
    CPU.Memory_SRAM=Memory.SRAM;
    CPU.Memory_BWRAM=Memory.BWRAM;
//    CPU.Memory_SRAMMask=Memory.SRAMMask;
	
}

void S9xMainLoop (void)
{
	asmMainLoop(&CPU);
	Registers.PC = CPU.PC - CPU.PCBase;
	S9xAPUPackStatus ();
	if (CPU.Flags & SCAN_KEYS_FLAG)
	{
		CPU.Flags &= ~SCAN_KEYS_FLAG;
	}

	if (CPU.BRKTriggered && Settings.SuperFX && !CPU.TriedInterleavedMode2)
	{
	CPU.TriedInterleavedMode2 = TRUE;
	CPU.BRKTriggered = FALSE;
	S9xDeinterleaveMode2 ();
	}
}

void S9xSetIRQ (u32 source)
{
	CPU.IRQActive |= source;
	CPU.Flags |= IRQ_PENDING_FLAG;
	CPU.IRQCycleCount = 3;
	if (CPU.WaitingForInterrupt)
	{
	// Force IRQ to trigger immediately after WAI -
	// Final Fantasy Mystic Quest crashes without this.
	CPU.IRQCycleCount = 0;
	CPU.WaitingForInterrupt = FALSE;
	CPU.PC++;
	}
}

void S9xDoHBlankProcessing ()
{
#ifdef CPU_SHUTDOWN
	CPU.WaitCounter++;
#endif
	switch (CPU.WhichEvent)
	{
	case HBLANK_START_EVENT:
		if (IPPU.HDMA && CPU.V_Counter <= PPU.ScreenHeight)
			IPPU.HDMA = S9xDoHDMA (IPPU.HDMA);
		break;

	case HBLANK_END_EVENT:
		APU_EXECUTE(3); // notaz: run spc700 in sound 'speed hack' mode

		if(Settings.SuperFX)
			S9xSuperFXExec ();

		CPU.Cycles -= Settings.H_Max;
		if (/*IAPU.APUExecuting*/CPU.APU_APUExecuting)
			CPU.APU_Cycles -= Settings.H_Max;
		else
			CPU.APU_Cycles = 0;

		CPU.NextEvent = -1;
		ICPU.Scanline++;

		if (++CPU.V_Counter > (Settings.PAL ? SNES_MAX_PAL_VCOUNTER : SNES_MAX_NTSC_VCOUNTER))
		{
			PPU.OAMAddr = PPU.SavedOAMAddr;
			PPU.OAMFlip = 0;
			CPU.V_Counter = 0;
			CPU.NMIActive = FALSE;
			ICPU.Frame++;
			PPU.HVBeamCounterLatched = 0;
			CPU.Flags |= SCAN_KEYS_FLAG;
			S9xStartHDMA ();
		}

		if (PPU.VTimerEnabled && !PPU.HTimerEnabled &&
			CPU.V_Counter == PPU.IRQVBeamPos)
		{
			S9xSetIRQ (PPU_V_BEAM_IRQ_SOURCE);
		}

		if (CPU.V_Counter == PPU.ScreenHeight + FIRST_VISIBLE_LINE)
		{
			// Start of V-blank
			S9xEndScreenRefresh ();
			PPU.FirstSprite = 0;
			IPPU.HDMA = 0;
			// Bits 7 and 6 of $4212 are computed when read in S9xGetPPU.
			missing.dma_this_frame = 0;
			IPPU.MaxBrightness = PPU.Brightness;
			PPU.ForcedBlanking = (Memory.FillRAM [0x2100] >> 7) & 1;

			Memory.FillRAM[0x4210] = 0x80;
			if (Memory.FillRAM[0x4200] & 0x80)
			{
			CPU.NMIActive = TRUE;
			CPU.Flags |= NMI_FLAG;
			CPU.NMICycleCount = CPU.NMITriggerPoint;
			}

			}

		if (CPU.V_Counter == PPU.ScreenHeight + 3)
			S9xUpdateJoypads ();

		if (CPU.V_Counter == FIRST_VISIBLE_LINE)
		{
			Memory.FillRAM[0x4210] = 0;
			CPU.Flags &= ~NMI_FLAG;
			S9xStartScreenRefresh ();
		}
		if (CPU.V_Counter >= FIRST_VISIBLE_LINE &&
			CPU.V_Counter < PPU.ScreenHeight + FIRST_VISIBLE_LINE)
		{
			RenderLine (CPU.V_Counter - FIRST_VISIBLE_LINE);
		}
		// Use TimerErrorCounter to skip update of SPC700 timers once
		// every 128 updates. Needed because this section of code is called
		// once every emulated 63.5 microseconds, which coresponds to
		// 15.750KHz, but the SPC700 timers need to be updated at multiples
		// of 8KHz, hence the error correction.
	//	IAPU.TimerErrorCounter++;
	//	if (IAPU.TimerErrorCounter >= )
	//	    IAPU.TimerErrorCounter = 0;
	//	else
		{
			if (APU.TimerEnabled [2])
			{
			APU.Timer [2] += 4;
			while (APU.Timer [2] >= APU.TimerTarget [2])
			{
				IAPU.RAM [0xff] = (IAPU.RAM [0xff] + 1) & 0xf;
				APU.Timer [2] -= APU.TimerTarget [2];
#ifdef SPC700_SHUTDOWN
				IAPU.WaitCounter++;
				/*IAPU.APUExecuting*/CPU.APU_APUExecuting= TRUE;
#endif
			}
			}
			if (CPU.V_Counter & 1)
			{
			if (APU.TimerEnabled [0])
			{
				APU.Timer [0]++;
				if (APU.Timer [0] >= APU.TimerTarget [0])
				{
				IAPU.RAM [0xfd] = (IAPU.RAM [0xfd] + 1) & 0xf;
				APU.Timer [0] = 0;
#ifdef SPC700_SHUTDOWN
				IAPU.WaitCounter++;
				/*IAPU.APUExecuting*/CPU.APU_APUExecuting = TRUE;
#endif
				}
			}
			if (APU.TimerEnabled [1])
			{
				APU.Timer [1]++;
				if (APU.Timer [1] >= APU.TimerTarget [1])
				{
				IAPU.RAM [0xfe] = (IAPU.RAM [0xfe] + 1) & 0xf;
				APU.Timer [1] = 0;
#ifdef SPC700_SHUTDOWN
				IAPU.WaitCounter++;
				/*IAPU.APUExecuting*/CPU.APU_APUExecuting = TRUE;
#endif
				}
			}
			}
		}
		break;
	case HTIMER_BEFORE_EVENT:
	case HTIMER_AFTER_EVENT:
		if (PPU.HTimerEnabled &&
			(!PPU.VTimerEnabled || CPU.V_Counter == PPU.IRQVBeamPos))
		{
			S9xSetIRQ (PPU_H_BEAM_IRQ_SOURCE);
		}
		break;
	}
	S9xReschedule ();
}

void snesCpuSl() {
	emsl.begin("cpu");
	u32 oldCpuFlags = CPU.Flags;
	emsl.var("flags", CPU.Flags);
	emsl.var("branchSkip", CPU.BranchSkip);
	emsl.var("nmiActive", CPU.NMIActive);
	emsl.var("irqActive", CPU.IRQActive);
	emsl.var("waitForInt", CPU.WaitingForInterrupt);
	emsl.var("whichEvent", CPU.WhichEvent);
	emsl.var("cycles", CPU.Cycles);
	emsl.var("nextEvent", CPU.NextEvent);
	emsl.var("vCounter", CPU.V_Counter);
	emsl.var("memSpeed", CPU.MemSpeed);
	emsl.var("memSpeedx2", CPU.MemSpeedx2);
	emsl.var("fastRomSpeed", CPU.FastROMSpeed);
	\
	u16 p_w = Registers.P.W;
	u16 a_w = Registers.A.W;
	u16 d_w = Registers.D.W;
	u16 s_w = Registers.S.W;
	u16 x_w = Registers.X.W;
	u16 y_w = Registers.Y.W;
	u16 pc = Registers.PC;
	emsl.var("reg_pb", Registers.PB);
	emsl.var("reg_db", Registers.DB);
	emsl.var("p", p_w);
	emsl.var("a", a_w);
	emsl.var("d", d_w);
	emsl.var("s", s_w);
	emsl.var("x", x_w);
	emsl.var("y", y_w);
	emsl.var("pc", pc);
	Registers.P.W = p_w;
	Registers.A.W = a_w;
	Registers.D.W = d_w;
	Registers.S.W = s_w;
	Registers.X.W = x_w;
	Registers.Y.W = y_w;
	Registers.PC = pc;

	if (!emsl.save) {
		CPU.Flags |= oldCpuFlags & (DEBUG_MODE_FLAG | TRACE_FLAG | SINGLE_STEP_FLAG | FRAME_ADVANCE_FLAG);
		CPU.InDMA = FALSE;
		ICPU.ShiftedPB = Registers.PB << 16;
		ICPU.ShiftedDB = Registers.DB << 16;
	}
	emsl.end();
}
