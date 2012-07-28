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
#include "cpu.h"
#include "missing.h"
#include "spu.h"
#include "dma.h"
#include "gfx.h"
#include "display.h"
#include "sa1.h"
//#include "netplay.h"
#include "sdd1.h"
#include "srtc.h"

#include "port.h"

#ifdef SUPER_FX
#include "fxemu.h"
#include "fxinst.h"
extern struct FxInit_s SuperFX;
extern struct FxRegs_s GSU;
#else
EXTERN_C void S9xSuperFXWriteReg (u8, u32);
EXTERN_C u8 S9xSuperFXReadReg (u32);
#endif

#include <QDataStream>

void S9xUpdateHTimer ()
{
    if (PPU.HTimerEnabled)
    {
#ifdef DEBUGGER
	missing.hirq_pos = PPU.IRQHBeamPos;
#endif
	PPU.HTimerPosition = PPU.IRQHBeamPos * Settings.H_Max / SNES_HCOUNTER_MAX;
	if (PPU.HTimerPosition == Settings.H_Max ||
	    PPU.HTimerPosition == Settings.HBlankStart)
	{
	    PPU.HTimerPosition--;
	}

	if (!PPU.VTimerEnabled || CPU.V_Counter == PPU.IRQVBeamPos)
	{
	    if (PPU.HTimerPosition < CPU.Cycles)
	    {
		// Missed the IRQ on this line already
		if (CPU.WhichEvent == HBLANK_END_EVENT ||
		    CPU.WhichEvent == HTIMER_AFTER_EVENT)
		{
		    CPU.WhichEvent = HBLANK_END_EVENT;
		    CPU.NextEvent = Settings.H_Max;
		}
		else
		{
		    CPU.WhichEvent = HBLANK_START_EVENT;
		    CPU.NextEvent = Settings.HBlankStart;
		}
	    }
	    else
	    {
		if (CPU.WhichEvent == HTIMER_BEFORE_EVENT ||
		    CPU.WhichEvent == HBLANK_START_EVENT)
		{
		    if (PPU.HTimerPosition > Settings.HBlankStart)
		    {
			// HTimer was to trigger before h-blank start,
			// now triggers after start of h-blank
			CPU.NextEvent = Settings.HBlankStart;
			CPU.WhichEvent = HBLANK_START_EVENT;
		    }
		    else
		    {
			CPU.NextEvent = PPU.HTimerPosition;
			CPU.WhichEvent = HTIMER_BEFORE_EVENT;
		    }
		}
		else
		{
		    CPU.WhichEvent = HTIMER_AFTER_EVENT;
		    CPU.NextEvent = PPU.HTimerPosition;
		}
	    }
	}
    }
}



void S9xFixColourBrightness ()
{
    IPPU.XB = mul_brightness [PPU.Brightness];
    if (Settings.SixteenBit)
    {
		for (int i = 0; i < 256; i++)
		{
	    	IPPU.Red [i]   = IPPU.XB [PPU.CGDATA [i] & 0x1f];
		    IPPU.Green [i] = IPPU.XB [(PPU.CGDATA [i] >> 5) & 0x1f];
		    IPPU.Blue [i]  = IPPU.XB [(PPU.CGDATA [i] >> 10) & 0x1f];
	    IPPU.ScreenColors [i] = BUILD_PIXEL (IPPU.Red [i], IPPU.Green [i],
						 IPPU.Blue [i]);
		}
    }  
}

/**********************************************************************************************/
/* S9xSetPPU()                                                                                   */
/* This function sets a PPU Register to a specific byte                                       */
/**********************************************************************************************/
void S9xSetPPU (u8 Byte, u16 Address)
{
    if (Address <= 0x2183)
    {
    switch (Address)
    {
    case 0x2100:
	// Brightness and screen blank bit
	if (Byte != Memory.FillRAM [0x2100])
	{
	    FLUSH_REDRAW ();
	    if (PPU.Brightness != (Byte & 0xF))
	    {
		IPPU.ColorsChanged = TRUE;
		IPPU.DirectColourMapsNeedRebuild = TRUE;
		PPU.Brightness = Byte & 0xF;
		S9xFixColourBrightness ();
		if (PPU.Brightness > IPPU.MaxBrightness)
		    IPPU.MaxBrightness = PPU.Brightness;
	    }
	    if ((Memory.FillRAM[0x2100] & 0x80) != (Byte & 0x80))
	    {
		IPPU.ColorsChanged = TRUE;
		PPU.ForcedBlanking = (Byte >> 7) & 1;
	    }
	}
	break;

    case 0x2101:
	// Sprite (OBJ) tile address
	if (Byte != Memory.FillRAM [0x2101])
	{
	    FLUSH_REDRAW ();
	    PPU.OBJNameBase   = (Byte & 3) << 14;
	    PPU.OBJNameSelect = ((Byte >> 3) & 3) << 13;
	    PPU.OBJSizeSelect = (Byte >> 5) & 7;
	    IPPU.OBJChanged = TRUE;
	}
	break;

    case 0x2102:
	// Sprite write address (low)
				PPU.OAMAddr = Byte;
				PPU.OAMFlip = 2;
				PPU.OAMReadFlip = 0;
				PPU.SavedOAMAddr = PPU.OAMAddr;
				if (PPU.OAMPriorityRotation)
				{
					PPU.FirstSprite = PPU.OAMAddr & 0x7f;
#ifdef DEBUGGER
		missing.sprite_priority_rotation = 1;
#endif
	}
	break;

    case 0x2103:
	// Sprite register write address (high), sprite priority rotation
	// bit.
				if ((PPU.OAMPriorityRotation = (Byte & 0x80) == 0 ? 0 : 1))
				{
					PPU.FirstSprite = PPU.OAMAddr & 0x7f;
#ifdef DEBUGGER
			missing.sprite_priority_rotation = 1;
#endif
		}
				// Only update the sprite write address top bit if the low byte has
				// been written to first.
				if (PPU.OAMFlip & 2)
				{
					PPU.OAMAddr &= 0x00FF;
					PPU.OAMAddr |= (Byte & 1) << 8;
				}
	PPU.OAMFlip = 0;
	PPU.OAMReadFlip = 0;
	PPU.SavedOAMAddr = PPU.OAMAddr;
	break;

    case 0x2104:
	// Sprite register write
	REGISTER_2104(Byte);
	break;

    case 0x2105:
	// Screen mode (0 - 7), background tile sizes and background 3
	// priority
	if (Byte != Memory.FillRAM [0x2105])
	{
	    FLUSH_REDRAW ();
	    PPU.BG[0].BGSize = (Byte >> 4) & 1;
	    PPU.BG[1].BGSize = (Byte >> 5) & 1;
	    PPU.BG[2].BGSize = (Byte >> 6) & 1;
	    PPU.BG[3].BGSize = (Byte >> 7) & 1;
	    PPU.BGMode = Byte & 7;
				// BJ: BG3Priority only takes effect if BGMode==1 and the bit is set
				PPU.BG3Priority  = ((Byte & 0x0f) == 0x09);
#ifdef DEBUGGER
	    missing.modes[PPU.BGMode] = 1;
#endif
	}
	break;

    case 0x2106:
	// Mosaic pixel size and enable
	if (Byte != Memory.FillRAM [0x2106])
	{
	    FLUSH_REDRAW ();
#ifdef DEBUGGER
	    if ((Byte & 0xf0) && (Byte & 0x0f))
		missing.mosaic = 1;
#endif
	    PPU.Mosaic = (Byte >> 4) + 1;
	    PPU.BGMosaic [0] = (Byte & 1) && PPU.Mosaic > 1;
	    PPU.BGMosaic [1] = (Byte & 2) && PPU.Mosaic > 1;
	    PPU.BGMosaic [2] = (Byte & 4) && PPU.Mosaic > 1;
	    PPU.BGMosaic [3] = (Byte & 8) && PPU.Mosaic > 1;
	}
	break;
    case 0x2107:		// [BG0SC]
	if (Byte != Memory.FillRAM [0x2107])
	{
	    FLUSH_REDRAW ();
	    PPU.BG[0].SCSize = Byte & 3;
	    PPU.BG[0].SCBase = (Byte & 0x7c) << 8;
	}
	break;

    case 0x2108:		// [BG1SC]
	if (Byte != Memory.FillRAM [0x2108])
	{
	    FLUSH_REDRAW ();
	    PPU.BG[1].SCSize = Byte & 3;
	    PPU.BG[1].SCBase = (Byte & 0x7c) << 8;
	}
	break;

    case 0x2109:		// [BG2SC]
	if (Byte != Memory.FillRAM [0x2109])
	{
	    FLUSH_REDRAW ();
	    PPU.BG[2].SCSize = Byte & 3;
	    PPU.BG[2].SCBase = (Byte & 0x7c) << 8;
	}
	break;

    case 0x210A:		// [BG3SC]
	if (Byte != Memory.FillRAM [0x210a])
	{
	    FLUSH_REDRAW ();
	    PPU.BG[3].SCSize = Byte & 3;
	    PPU.BG[3].SCBase = (Byte & 0x7c) << 8;
	}
	break;

    case 0x210B:		// [BG01NBA]
	if (Byte != Memory.FillRAM [0x210b])
	{
	    FLUSH_REDRAW ();
	    PPU.BG[0].NameBase    = (Byte & 7) << 12;
	    PPU.BG[1].NameBase    = ((Byte >> 4) & 7) << 12;
	}
	break;

    case 0x210C:		// [BG23NBA]
	if (Byte != Memory.FillRAM [0x210c])
	{
	    FLUSH_REDRAW ();
	    PPU.BG[2].NameBase    = (Byte & 7) << 12;
	    PPU.BG[3].NameBase    = ((Byte >> 4) & 7) << 12;
	}
	break;

    case 0x210D:
				PPU.BG[0].HOffset =
					((PPU.BG[0].HOffset >> 8) & 0xff) | ((u16) Byte << 8);
				break;

			case 0x210E :
				PPU.BG[0].VOffset =
					((PPU.BG[0].VOffset >> 8) & 0xff) | ((u16) Byte << 8);
				break;
			case 0x210F :
				PPU.BG[1].HOffset =
					((PPU.BG[1].HOffset >> 8) & 0xff) | ((u16) Byte << 8);
				break;

			case 0x2110 :
				PPU.BG[1].VOffset =
					((PPU.BG[1].VOffset >> 8) & 0xff) | ((u16) Byte << 8);
				break;

			case 0x2111 :
				PPU.BG[2].HOffset =
					((PPU.BG[2].HOffset >> 8) & 0xff) | ((u16) Byte << 8);
				break;

			case 0x2112 :
				PPU.BG[2].VOffset =
					((PPU.BG[2].VOffset >> 8) & 0xff) | ((u16) Byte << 8);
				break;

			case 0x2113 :
				PPU.BG[3].HOffset =
					((PPU.BG[3].HOffset >> 8) & 0xff) | ((u16) Byte << 8);
				break;

    case 0x2114:
				PPU.BG[3].VOffset =
					((PPU.BG[3].VOffset >> 8) & 0xff) | ((u16) Byte << 8);
	break;

    case 0x2115:
	// VRAM byte/word access flag and increment
	PPU.VMA.High = (Byte & 0x80) == 0 ? FALSE : TRUE;
	switch (Byte & 3)
	{
	case 0:
	    PPU.VMA.Increment = 1;
	    break;
	case 1:
	    PPU.VMA.Increment = 32;
	    break;
	case 2:
	    PPU.VMA.Increment = 128;
	    break;
	case 3:
	    PPU.VMA.Increment = 128;
	    break;
	}
#ifdef DEBUGGER
	if ((Byte & 3) != 0)
	    missing.vram_inc = Byte & 3;
#endif
	if (Byte & 0x0c)
	{
		static u16 IncCount [4] = { 0, 32, 64, 128 };
		static u16 Shift [4] = { 0, 5, 6, 7 };
#ifdef DEBUGGER
					missing.vram_full_graphic_inc =
						(Byte & 0x0c) >> 2;
#endif
	    PPU.VMA.Increment = 1;
		u8 i = (Byte & 0x0c) >> 2;
	    PPU.VMA.FullGraphicCount = IncCount [i];
	    PPU.VMA.Mask1 = IncCount [i] * 8 - 1;
	    PPU.VMA.Shift = Shift [i];
	}
	else
	    PPU.VMA.FullGraphicCount = 0;
	break;

    case 0x2116:
	// VRAM read/write address (low)
	PPU.VMA.Address &= 0xFF00;
	PPU.VMA.Address |= Byte;
	IPPU.FirstVRAMRead = TRUE;
	break;

    case 0x2117:
	// VRAM read/write address (high)
	PPU.VMA.Address &= 0x00FF;
	PPU.VMA.Address |= Byte << 8;
	IPPU.FirstVRAMRead = TRUE;
	break;

    case 0x2118:
	// VRAM write data (low)
	IPPU.FirstVRAMRead = TRUE;
	REGISTER_2118(Byte);
	break;

    case 0x2119:
	// VRAM write data (high)
	IPPU.FirstVRAMRead = TRUE;
	REGISTER_2119(Byte);
	break;

    case 0x211a:
	// Mode 7 outside rotation area display mode and flipping
	if (Byte != Memory.FillRAM [0x211a])
	{
	    FLUSH_REDRAW ();
	    PPU.Mode7Repeat = Byte >> 6;
	    PPU.Mode7VFlip = (Byte & 2) >> 1;
	    PPU.Mode7HFlip = Byte & 1;
	}
	break;
    case 0x211b:
	// Mode 7 matrix A (low & high)
	PPU.MatrixA = ((PPU.MatrixA >> 8) & 0xff) | (Byte << 8);
	PPU.Need16x8Mulitply = TRUE;
	break;
    case 0x211c:
	// Mode 7 matrix B (low & high)
	PPU.MatrixB = ((PPU.MatrixB >> 8) & 0xff) | (Byte << 8);
	PPU.Need16x8Mulitply = TRUE;
	break;
    case 0x211d:
	// Mode 7 matrix C (low & high)
	PPU.MatrixC = ((PPU.MatrixC >> 8) & 0xff) | (Byte << 8);
	break;
    case 0x211e:
	// Mode 7 matrix D (low & high)
	PPU.MatrixD = ((PPU.MatrixD >> 8) & 0xff) | (Byte << 8);
	break;
    case 0x211f:
	// Mode 7 centre of rotation X (low & high)
	PPU.CentreX = ((PPU.CentreX >> 8) & 0xff) | (Byte << 8);
	break;
    case 0x2120:
	// Mode 7 centre of rotation Y (low & high)
	PPU.CentreY = ((PPU.CentreY >> 8) & 0xff) | (Byte << 8);
	break;

    case 0x2121:
	// CG-RAM address
	PPU.CGFLIP = 0;
	PPU.CGFLIPRead = 0;
	PPU.CGADD = Byte;
	break;

    case 0x2122:
	REGISTER_2122(Byte);
	break;

    case 0x2123:
	// Window 1 and 2 enable for backgrounds 1 and 2
	if (Byte != Memory.FillRAM [0x2123])
	{
					if (Settings.os9x_hack&PPU_IGNORE_WINDOW) return;
	    FLUSH_REDRAW ();
	    PPU.ClipWindow1Enable [0] = !!(Byte & 0x02);
	    PPU.ClipWindow1Enable [1] = !!(Byte & 0x20);
	    PPU.ClipWindow2Enable [0] = !!(Byte & 0x08);
	    PPU.ClipWindow2Enable [1] = !!(Byte & 0x80);
	    PPU.ClipWindow1Inside [0] = !(Byte & 0x01);
	    PPU.ClipWindow1Inside [1] = !(Byte & 0x10);
	    PPU.ClipWindow2Inside [0] = !(Byte & 0x04);
	    PPU.ClipWindow2Inside [1] = !(Byte & 0x40);
	    PPU.RecomputeClipWindows = TRUE;
#ifdef DEBUGGER
	    if (Byte & 0x80)
		missing.window2[1] = 1;
	    if (Byte & 0x20)
		missing.window1[1] = 1;
	    if (Byte & 0x08)
		missing.window2[0] = 1;
	    if (Byte & 0x02)
		missing.window1[0] = 1;
#endif
	}
	break;
    case 0x2124:
	// Window 1 and 2 enable for backgrounds 3 and 4
	if (Byte != Memory.FillRAM [0x2124])
	{
					if (Settings.os9x_hack&PPU_IGNORE_WINDOW) return;
	    FLUSH_REDRAW ();
	    PPU.ClipWindow1Enable [2] = !!(Byte & 0x02);
	    PPU.ClipWindow1Enable [3] = !!(Byte & 0x20);
	    PPU.ClipWindow2Enable [2] = !!(Byte & 0x08);
	    PPU.ClipWindow2Enable [3] = !!(Byte & 0x80);
	    PPU.ClipWindow1Inside [2] = !(Byte & 0x01);
	    PPU.ClipWindow1Inside [3] = !(Byte & 0x10);
	    PPU.ClipWindow2Inside [2] = !(Byte & 0x04);
	    PPU.ClipWindow2Inside [3] = !(Byte & 0x40);
	    PPU.RecomputeClipWindows = TRUE;
#ifdef DEBUGGER
	    if (Byte & 0x80)
		missing.window2[3] = 1;
	    if (Byte & 0x20)
		missing.window1[3] = 1;
	    if (Byte & 0x08)
		missing.window2[2] = 1;
	    if (Byte & 0x02)
		missing.window1[2] = 1;
#endif
	}
	break;
    case 0x2125:
	// Window 1 and 2 enable for objects and colour window
	if (Byte != Memory.FillRAM [0x2125])
	{
					if (Settings.os9x_hack&PPU_IGNORE_WINDOW) return;
	    FLUSH_REDRAW ();
	    PPU.ClipWindow1Enable [4] = !!(Byte & 0x02);
	    PPU.ClipWindow1Enable [5] = !!(Byte & 0x20);
	    PPU.ClipWindow2Enable [4] = !!(Byte & 0x08);
	    PPU.ClipWindow2Enable [5] = !!(Byte & 0x80);
	    PPU.ClipWindow1Inside [4] = !(Byte & 0x01);
	    PPU.ClipWindow1Inside [5] = !(Byte & 0x10);
	    PPU.ClipWindow2Inside [4] = !(Byte & 0x04);
	    PPU.ClipWindow2Inside [5] = !(Byte & 0x40);
	    PPU.RecomputeClipWindows = TRUE;
#ifdef DEBUGGER
	    if (Byte & 0x80)
		missing.window2[5] = 1;
	    if (Byte & 0x20)
		missing.window1[5] = 1;
	    if (Byte & 0x08)
		missing.window2[4] = 1;
	    if (Byte & 0x02)
		missing.window1[4] = 1;
#endif
	}
	break;
    case 0x2126:
	// Window 1 left position
	if (Byte != Memory.FillRAM [0x2126])
	{
					if (Settings.os9x_hack&PPU_IGNORE_WINDOW) return;
	    FLUSH_REDRAW ();
	    PPU.Window1Left = Byte;
	    PPU.RecomputeClipWindows = TRUE;
	}
	break;
    case 0x2127:
	// Window 1 right position
	if (Byte != Memory.FillRAM [0x2127])
	{
					if (Settings.os9x_hack&PPU_IGNORE_WINDOW) return;
	    FLUSH_REDRAW ();
	    PPU.Window1Right = Byte;
	    PPU.RecomputeClipWindows = TRUE;
	}
	break;
    case 0x2128:
	// Window 2 left position
	if (Byte != Memory.FillRAM [0x2128])
	{
					if (Settings.os9x_hack&PPU_IGNORE_WINDOW) return;
	    FLUSH_REDRAW ();
	    PPU.Window2Left = Byte;
	    PPU.RecomputeClipWindows = TRUE;
	}
	break;
    case 0x2129:
	// Window 2 right position
	if (Byte != Memory.FillRAM [0x2129])
	{
					if (Settings.os9x_hack&PPU_IGNORE_WINDOW) return;
	    FLUSH_REDRAW ();
	    PPU.Window2Right = Byte;
	    PPU.RecomputeClipWindows = TRUE;
	}
	break;
    case 0x212a:
	// Windows 1 & 2 overlap logic for backgrounds 1 - 4
	if (Byte != Memory.FillRAM [0x212a])
	{
					if (Settings.os9x_hack&PPU_IGNORE_WINDOW) return;
	    FLUSH_REDRAW ();
	    PPU.ClipWindowOverlapLogic [0] = (Byte & 0x03);
	    PPU.ClipWindowOverlapLogic [1] = (Byte & 0x0c) >> 2;
	    PPU.ClipWindowOverlapLogic [2] = (Byte & 0x30) >> 4;
	    PPU.ClipWindowOverlapLogic [3] = (Byte & 0xc0) >> 6;
	    PPU.RecomputeClipWindows = TRUE;
	}
	break;
    case 0x212b:
	// Windows 1 & 2 overlap logic for objects and colour window
	if (Byte != Memory.FillRAM [0x212b])
	{
					if (Settings.os9x_hack&PPU_IGNORE_WINDOW) return;
	    FLUSH_REDRAW ();
	    PPU.ClipWindowOverlapLogic [4] = Byte & 0x03;
	    PPU.ClipWindowOverlapLogic [5] = (Byte & 0x0c) >> 2;
	    PPU.RecomputeClipWindows = TRUE;
	}
	break;
    case 0x212c:
	// Main screen designation (backgrounds 1 - 4 and objects)
	if (Byte != Memory.FillRAM [0x212c])
	{
	    FLUSH_REDRAW ();
	    PPU.RecomputeClipWindows = TRUE;
	    Memory.FillRAM [Address] = Byte;
	    return;
	}
	break;
    case 0x212d:
	// Sub-screen designation (backgrounds 1 - 4 and objects)
	if (Byte != Memory.FillRAM [0x212d])
	{
	    FLUSH_REDRAW ();
#ifdef DEBUGGER
	    if (Byte & 0x1f)
		missing.subscreen = 1;
#endif
	    PPU.RecomputeClipWindows = TRUE;
	    Memory.FillRAM [Address] = Byte;
	    return;
	}
	break;
    case 0x212e:
	// Window mask designation for main screen ?
	if (Byte != Memory.FillRAM [0x212e])
	{
	    FLUSH_REDRAW ();
	    PPU.RecomputeClipWindows = TRUE;
	}
	break;
    case 0x212f:
	// Window mask designation for sub-screen ?
	if (Byte != Memory.FillRAM [0x212f])
	{
	    FLUSH_REDRAW ();
	    PPU.RecomputeClipWindows = TRUE;
	}
	break;
    case 0x2130:
	// Fixed colour addition or screen addition
	if (Byte != Memory.FillRAM [0x2130])
	{
					if (Settings.os9x_hack&PPU_IGNORE_ADDSUB) return;
	    FLUSH_REDRAW ();
	    PPU.RecomputeClipWindows = TRUE;
#ifdef DEBUGGER
					if ((Byte & 1) && (PPU.BGMode == 3 || PPU.BGMode == 4
						|| PPU.BGMode == 7))
		missing.direct = 1;
#endif
	}
	break;
    case 0x2131:
	// Colour addition or subtraction select
	if (Byte != Memory.FillRAM[0x2131])
	{
					if (Settings.os9x_hack&PPU_IGNORE_ADDSUB) return;
	    FLUSH_REDRAW ();
	    // Backgrounds 1 - 4, objects and backdrop colour add/sub enable
#ifdef DEBUGGER
	    if (Byte & 0x80)
	    {
		// Subtract
		if (Memory.FillRAM[0x2130] & 0x02)
		    missing.subscreen_sub = 1;
		else
		    missing.fixed_colour_sub = 1;
	    }
	    else
	    {
		// Addition
		if (Memory.FillRAM[0x2130] & 0x02)
		    missing.subscreen_add = 1;
		else
		    missing.fixed_colour_add = 1;
	    }
#endif
	    Memory.FillRAM[0x2131] = Byte;
	}
	break;
    case 0x2132:
	if (Byte != Memory.FillRAM [0x2132])
				{
					int new_fixedcol;
					//FLUSH_REDRAW ();
					// Colour data for fixed colour addition/subtraction
					if (Byte & 0x80) {
						//PPU.FixedColourBlue = Byte & 0x1f;
						new_fixedcol=(Byte & 0x1f);
						if (new_fixedcol!=PPU.FixedColourBlue) {if (!(Settings.os9x_hack&PPU_IGNORE_FIXEDCOLCHANGES)) FLUSH_REDRAW();PPU.FixedColourBlue=new_fixedcol;}
					}
					if (Byte & 0x40) {
						//PPU.FixedColourGreen = Byte & 0x1f;
						new_fixedcol=(Byte & 0x1f);
						if (new_fixedcol!=PPU.FixedColourGreen) {if (!(Settings.os9x_hack&PPU_IGNORE_FIXEDCOLCHANGES)) FLUSH_REDRAW();PPU.FixedColourGreen=new_fixedcol;}
					}
					if (Byte & 0x20) {
						//PPU.FixedColourRed = Byte & 0x1f;
						new_fixedcol=(Byte & 0x1f);
						if (new_fixedcol!=PPU.FixedColourRed) {if (!(Settings.os9x_hack&PPU_IGNORE_FIXEDCOLCHANGES)) FLUSH_REDRAW();PPU.FixedColourRed=new_fixedcol;}
					}

					/*FLUSH_REDRAW ();
					// Colour data for fixed colour addition/subtraction
					if (Byte & 0x80)
						PPU.FixedColourBlue = Byte & 0x1f;
					if (Byte & 0x40)
						PPU.FixedColourGreen = Byte & 0x1f;
					if (Byte & 0x20)
					PPU.FixedColourRed = Byte & 0x1f;*/

				}
	break;
    case 0x2133:
	// Screen settings
	if (Byte != Memory.FillRAM [0x2133])
	{
#ifdef DEBUGGER
	    if (Byte & 0x40)
		missing.mode7_bgmode = 1;
	    if (Byte & 0x08)
		missing.pseudo_512 = 1;
#endif
	    if (Byte & 0x04)
	    {
		PPU.ScreenHeight = SNES_HEIGHT_EXTENDED;
#ifdef DEBUGGER
		missing.lines_239 = 1;
#endif
	    }
	    else
	        PPU.ScreenHeight = SNES_HEIGHT;
#ifdef DEBUGGER
	    if (Byte & 0x02)
		missing.sprite_double_height = 1;

	    if (Byte & 1)
		missing.interlace = 1;
#endif
	}
	break;
    case 0x2134:
    case 0x2135:
    case 0x2136:
	// Matrix 16bit x 8bit multiply result (read-only)
	return;

    case 0x2137:
	// Software latch for horizontal and vertical timers (read-only)
	return;
    case 0x2138:
	// OAM read data (read-only)
	return;
    case 0x2139:
    case 0x213a:
	// VRAM read data (read-only)
	return;
    case 0x213b:
	// CG-RAM read data (read-only)
	return;
    case 0x213c:
    case 0x213d:
	// Horizontal and vertical (low/high) read counter (read-only)
	return;
    case 0x213e:
	// PPU status (time over and range over)
	return;
    case 0x213f:
	// NTSC/PAL select and field (read-only)
	return;
			case 0x2140 :
			case 0x2141 :
			case 0x2142 :
			case 0x2143 :
			case 0x2144 :
			case 0x2145 :
			case 0x2146 :
			case 0x2147 :
			case 0x2148 :
			case 0x2149 :
			case 0x214a :
			case 0x214b :
			case 0x214c :
			case 0x214d :
			case 0x214e :
			case 0x214f :
			case 0x2150 :
			case 0x2151 :
			case 0x2152 :
			case 0x2153 :
			case 0x2154 :
			case 0x2155 :
			case 0x2156 :
			case 0x2157 :
			case 0x2158 :
			case 0x2159 :
			case 0x215a :
			case 0x215b :
			case 0x215c :
			case 0x215d :
			case 0x215e :
			case 0x215f :
			case 0x2160 :
			case 0x2161 :
			case 0x2162 :
			case 0x2163 :
			case 0x2164 :
			case 0x2165 :
			case 0x2166 :
			case 0x2167 :
			case 0x2168 :
			case 0x2169 :
			case 0x216a :
			case 0x216b :
			case 0x216c :
			case 0x216d :
			case 0x216e :
			case 0x216f :
			case 0x2170 :
			case 0x2171 :
			case 0x2172 :
			case 0x2173 :
			case 0x2174 :
			case 0x2175 :
			case 0x2176 :
			case 0x2177 :
			case 0x2178 :
			case 0x2179 :
			case 0x217a :
			case 0x217b :
			case 0x217c :
			case 0x217d :
			case 0x217e :
			case 0x217f :
#ifdef SPCTOOL
	_SPCInPB (Address & 3, Byte);
#else	
//	CPU.Flags |= DEBUG_MODE_FLAG;
	Memory.FillRAM [Address] = Byte;
	IAPU.RAM [(Address & 3) + 0xf4] = Byte;
#ifdef SPC700_SHUTDOWN
	/*IAPU.APUExecuting*/CPU.APU_APUExecuting = Settings.APUEnabled;
	IAPU.WaitCounter++;
#endif
#endif // SPCTOOL
	break;
    case 0x2180:
	REGISTER_2180(Byte);
	break;
    case 0x2181:
	PPU.WRAM &= 0x1FF00;
	PPU.WRAM |= Byte;
	break;
    case 0x2182:
	PPU.WRAM &= 0x100FF;
	PPU.WRAM |= Byte << 8;
	break;
    case 0x2183:
	PPU.WRAM &= 0x0FFFF;
	PPU.WRAM |= Byte << 16;
	PPU.WRAM &= 0x1FFFF;
	break;
    }
    }
    else
    {
#ifdef USE_SA1
	if (Settings.SA1)
	{
	    if (Address >= 0x2200 && Address <0x23ff)
		S9xSetSA1 (Byte, Address);
	    else
		Memory.FillRAM [Address] = Byte;
	    return;
	}
	else
#endif
	// Dai Kaijyu Monogatari II
	if (Address == 0x2801 && Settings.SRTC)
	    S9xSetSRTC (Byte, Address);
			else if (Address < 0x3000 || Address >= 0x3000 + 768)
			{
#ifdef DEBUGGER
				missing.unknownppu_write = Address;
				if (Settings.TraceUnknownRegisters)
				{
					sprintf(
						String,
						"Unknown register write: $%02X->$%04X\n",
						Byte,
						Address);
					S9xMessage(S9X_TRACE, S9X_PPU_TRACE, String);
				}
#endif
	}
	else
	{
	    if (!Settings.SuperFX)
		return;
	    
	    switch (Address)
	    {
	    case 0x3030:
		if ((Memory.FillRAM [0x3030] ^ Byte) & FLG_G)
		{
		    Memory.FillRAM [Address] = Byte;
		    // Go flag has been changed
		    if (Byte & FLG_G)
			S9xSuperFXExec ();
		    else
			FxFlushCache ();
		}
		else
		    Memory.FillRAM [Address] = Byte;
		break;

	    case 0x3031:
		Memory.FillRAM [Address] = Byte;
		break;
	    case 0x3033:
		Memory.FillRAM [Address] = Byte;
		break;
	    case 0x3034:
		Memory.FillRAM [Address] = Byte & 0x7f;
		break;
	    case 0x3036:
		Memory.FillRAM [Address] = Byte & 0x7f;
		break;
	    case 0x3037:
		Memory.FillRAM [Address] = Byte;
		break;
	    case 0x3038:
		Memory.FillRAM [Address] = Byte;
		break;
	    case 0x3039:
		Memory.FillRAM [Address] = Byte;
		break;
	    case 0x303a:
		Memory.FillRAM [Address] = Byte;
		break;
	    case 0x303b:
		break;
	    case 0x303f:
		Memory.FillRAM [Address] = Byte;
		break;
	    case 0x301f:
		Memory.FillRAM [Address] = Byte;
		Memory.FillRAM [0x3000 + GSU_SFR] |= FLG_G;
		S9xSuperFXExec ();
		return;

	    default:
		Memory.FillRAM[Address] = Byte;
		if (Address >= 0x3100)
		{
		    FxCacheWriteAccess (Address);
		}
		break;
	    }
	    return;
	}
    }
    Memory.FillRAM[Address] = Byte;
}

/**********************************************************************************************/
/* S9xGetPPU()                                                                                   */
/* This function retrieves a PPU Register                                                     */
/**********************************************************************************************/
u8 S9xGetPPU (u16 Address)
{
	u8 byte = 0;

    if (Address <= 0x2190)
    {
	switch (Address)
	{
	case 0x2100:
	case 0x2101:
	    return (Memory.FillRAM[Address]);
	case 0x2102:
#ifdef DEBUGGER
	    missing.oam_address_read = 1;
#endif
		return (u8)(PPU.OAMAddr);
	case 0x2103:
	    return (((PPU.OAMAddr >> 8) & 1) | (PPU.OAMPriorityRotation << 7));
	case 0x2104:
	case 0x2105:
	case 0x2106:
	case 0x2107:
	case 0x2108:
	case 0x2109:
	case 0x210a:
	case 0x210b:
	case 0x210c:
	    return (Memory.FillRAM[Address]);
	case 0x210d:
	case 0x210e:
	case 0x210f:
	case 0x2110:
	case 0x2111:
	case 0x2112:
	case 0x2113:
	case 0x2114:
#ifdef DEBUGGER
	    missing.bg_offset_read = 1;
#endif
	    return (Memory.FillRAM[Address]);
	case 0x2115:
	    return (Memory.FillRAM[Address]);
	case 0x2116:
		return (u8)(PPU.VMA.Address);
	case 0x2117:
	    return (PPU.VMA.Address >> 8);
	case 0x2118:
	case 0x2119:
	case 0x211a:
	    return (Memory.FillRAM[Address]);
	case 0x211b:
	case 0x211c:
	case 0x211d:
	case 0x211e:
	case 0x211f:
	case 0x2120:
#ifdef DEBUGGER
	    missing.matrix_read = 1;
#endif
	    return (Memory.FillRAM[Address]);
	case 0x2121:
	    return (PPU.CGADD);
	case 0x2122:
	case 0x2123:
	case 0x2124:
	case 0x2125:
	case 0x2126:
	case 0x2127:
	case 0x2128:
	case 0x2129:
	case 0x212a:
	case 0x212b:
	case 0x212c:
	case 0x212d:
	case 0x212e:
	case 0x212f:
	case 0x2130:
	case 0x2131:
	case 0x2132:
	case 0x2133:
	    return (Memory.FillRAM[Address]);

	case 0x2134:
	case 0x2135:
	case 0x2136:
	    // 16bit x 8bit multiply read result.
	    if (PPU.Need16x8Mulitply)
	    {
		s32 r = (s32) PPU.MatrixA * (s32) (PPU.MatrixB >> 8);

		Memory.FillRAM[0x2134] = (u8) r;
		Memory.FillRAM[0x2135] = (u8)(r >> 8);
		Memory.FillRAM[0x2136] = (u8)(r >> 16);
		PPU.Need16x8Mulitply = FALSE;
	    }
#ifdef DEBUGGER
	    missing.matrix_multiply = 1;
#endif
	    return (Memory.FillRAM[Address]);
	case 0x2137:
	    // Latch h and v counters
#ifdef DEBUGGER
	    missing.h_v_latch = 1;
#endif
#if 0
#ifdef CPU_SHUTDOWN
	    CPU.WaitAddress = CPU.PCAtOpcodeStart;
#endif
#endif
	    PPU.HVBeamCounterLatched = 1;
				PPU.VBeamPosLatched = (u16)
				CPU.V_Counter;
				PPU.HBeamPosLatched = (u16) ((CPU.Cycles * SNES_HCOUNTER_MAX) / Settings.H_Max);

				if (SNESGameFixes.NeedInit0x2137)
					PPU.VBeamFlip = 0; //jyam sword world sfc2 & godzill
				return (0);
			case 0x2138 :
				// Read OAM (sprite) control data
				if (!PPU.OAMReadFlip)
		{
			byte = PPU.OAMData [PPU.OAMAddr << 1];
		}
		else
		{
					byte = PPU.OAMData[(PPU.OAMAddr << 1) + 1];
					if (++PPU.OAMAddr >= 0x110)
						PPU.OAMAddr = 0;
				}
				PPU.OAMReadFlip ^= 1;
#ifdef DEBUGGER
	    missing.oam_read = 1;
#endif
		return (byte);

	case 0x2139:
	    // Read vram low byte
#ifdef DEBUGGER
	    missing.vram_read = 1;
#endif
	    if (IPPU.FirstVRAMRead)
		byte = Memory.VRAM[PPU.VMA.Address << 1];
				else if (PPU.VMA.FullGraphicCount)
				{
					u32 addr = PPU.VMA.Address - 1;
					u32 rem = addr & PPU.VMA.Mask1;
					u32 address =
						(addr & ~PPU.VMA.Mask1)
							+ (rem >> PPU.VMA.Shift)
							+ ((rem & (PPU.VMA.FullGraphicCount - 1)) << 3);
		byte = Memory.VRAM [((address << 1) - 2) & 0xFFFF];
	    }
	    else
		byte = Memory.VRAM[((PPU.VMA.Address << 1) - 2) & 0xffff];

	    if (!PPU.VMA.High)
	    {
		PPU.VMA.Address += PPU.VMA.Increment;
		IPPU.FirstVRAMRead = FALSE;
	    }
	    break;
	case 0x213A:
	    // Read vram high byte
#ifdef DEBUGGER
	    missing.vram_read = 1;
#endif
	    if (IPPU.FirstVRAMRead)
		byte = Memory.VRAM[((PPU.VMA.Address << 1) + 1) & 0xffff];
				else if (PPU.VMA.FullGraphicCount)
				{
					u32 addr = PPU.VMA.Address - 1;
					u32 rem = addr & PPU.VMA.Mask1;
					u32 address =
						(addr & ~PPU.VMA.Mask1)
							+ (rem >> PPU.VMA.Shift)
							+ ((rem & (PPU.VMA.FullGraphicCount - 1)) << 3);
		byte = Memory.VRAM [((address << 1) - 1) & 0xFFFF];
	    }
	    else
		byte = Memory.VRAM[((PPU.VMA.Address << 1) - 1) & 0xFFFF];
	    if (PPU.VMA.High)
	    {
		PPU.VMA.Address += PPU.VMA.Increment;
		IPPU.FirstVRAMRead = FALSE;
	    }
	    break;

	case 0x213B:
	    // Read palette data
#ifdef DEBUGGER
	    missing.cgram_read = 1;
#endif
	    if (PPU.CGFLIPRead)
		byte = PPU.CGDATA [PPU.CGADD++] >> 8;
	    else
		byte = PPU.CGDATA [PPU.CGADD] & 0xff;

	    PPU.CGFLIPRead ^= 1;
	    return (byte);
	    
	case 0x213C:
	    // Horizontal counter value 0-339
#ifdef DEBUGGER
	    missing.h_counter_read = 1;
#endif
	    if (PPU.HBeamFlip)
		byte = PPU.HBeamPosLatched >> 8;
	    else
		byte = (u8)PPU.HBeamPosLatched;
	    PPU.HBeamFlip ^= 1;
	    break;
	case 0x213D:
	    // Vertical counter value 0-262
#ifdef DEBUGGER
	    missing.v_counter_read = 1;
#endif
	    if (PPU.VBeamFlip)
		byte = PPU.VBeamPosLatched >> 8;
	    else
		byte = (u8)PPU.VBeamPosLatched;
	    PPU.VBeamFlip ^= 1;
	    break;
	case 0x213E:
	    // PPU time and range over flags
	    return (SNESGameFixes._0x213E_ReturnValue);

	case 0x213F:
	    // NTSC/PAL and which field flags
	    PPU.VBeamFlip = PPU.HBeamFlip = 0;
				return ((Settings.PAL ? 0x10 : 0) | (Memory.FillRAM[0x213f] & 0xc0));

			case 0x2140 :
			case 0x2141 :
			case 0x2142 :
			case 0x2143 :
			case 0x2144 :
			case 0x2145 :
			case 0x2146 :
			case 0x2147 :
			case 0x2148 :
			case 0x2149 :
			case 0x214a :
			case 0x214b :
			case 0x214c :
			case 0x214d :
			case 0x214e :
			case 0x214f :
			case 0x2150 :
			case 0x2151 :
			case 0x2152 :
			case 0x2153 :
			case 0x2154 :
			case 0x2155 :
			case 0x2156 :
			case 0x2157 :
			case 0x2158 :
			case 0x2159 :
			case 0x215a :
			case 0x215b :
			case 0x215c :
			case 0x215d :
			case 0x215e :
			case 0x215f :
			case 0x2160 :
			case 0x2161 :
			case 0x2162 :
			case 0x2163 :
			case 0x2164 :
			case 0x2165 :
			case 0x2166 :
			case 0x2167 :
			case 0x2168 :
			case 0x2169 :
			case 0x216a :
			case 0x216b :
			case 0x216c :
			case 0x216d :
			case 0x216e :
			case 0x216f :
			case 0x2170 :
			case 0x2171 :
			case 0x2172 :
			case 0x2173 :
			case 0x2174 :
			case 0x2175 :
			case 0x2176 :
			case 0x2177 :
			case 0x2178 :
			case 0x2179 :
			case 0x217a :
			case 0x217b :
			case 0x217c :
			case 0x217d :
			case 0x217e :
			case 0x217f :
#ifdef SPCTOOL
		return ((u8) _SPCOutP [Address & 3]);
#else
    //	CPU.Flags |= DEBUG_MODE_FLAG;
#ifdef SPC700_SHUTDOWN	
	    /*IAPU.APUExecuting*/CPU.APU_APUExecuting = Settings.APUEnabled;
	    IAPU.WaitCounter++;
#endif
	    if (Settings.APUEnabled)
	    {
#ifdef CPU_SHUTDOWN
//		CPU.WaitAddress = CPU.PCAtOpcodeStart;
#endif
					if(SNESGameFixes.APU_OutPorts_ReturnValueFix
						&& Address >= 0x2140
						&& Address <= 0x2143
						&& !CPU.V_Counter)
					{
						return (u8) ((Address & 1) ?
							((rand() & 0xff00) >> 8) : (rand() & 0xff));
			}

			return (APU.OutPorts [Address & 3]);
		}

		switch (Settings.SoundSkipMethod)
		{
			case 0:
			case 1:
			CPU.BranchSkip = TRUE;
			break;
			case 2:
			break;
			case 3:
			CPU.BranchSkip = TRUE;
			break;
		}

				if ((Address & 3) < 2)
		{
			int r = rand ();
			if (r & 2)
			{
				if (r & 4)
				return (((Address & 3) == 1) ? 0xaa : 0xbb);
				else
				return ((r >> 3) & 0xff);
			}
		}
		else
		{
			int r = rand ();
			if (r & 2)
				return ((r >> 3) & 0xff);
	    }
	    return (Memory.FillRAM[Address]);
#endif // SPCTOOL

	case 0x2180:
	    // Read WRAM
#ifdef DEBUGGER
	    missing.wram_read = 1;
#endif
	    byte = Memory.RAM [PPU.WRAM++];
	    PPU.WRAM &= 0x1FFFF;
	    break;
	case 0x2181:
	case 0x2182:
	case 0x2183:
	    return (Memory.FillRAM [Address]);
	case 0x2190:
	    return (1);
	}
    }
    else
    {
#ifdef USE_SA1
	if (Settings.SA1)
	    return (S9xGetSA1 (Address));
#endif

	if (Address <= 0x2fff || Address >= 0x3000 + 768)
	{
	    switch (Address)
	    {
	    case 0x21c2:
	        return (0x20);
	    case 0x21c3:
	        return (0);
	    case 0x2800:
		// For Dai Kaijyu Monogatari II
		if (Settings.SRTC)
		    return (S9xGetSRTC (Address));
		/*FALL*/
			    
	    default:
#ifdef DEBUGGER
	        missing.unknownppu_read = Address;
	        if (Settings.TraceUnknownRegisters)
		{
		    sprintf (String, "Unknown register read: $%04X\n", Address);
		    S9xMessage (S9X_TRACE, S9X_PPU_TRACE, String);
		}
#endif
		// XXX:
	        return (0); //Memory.FillRAM[Address]);
	    }
	}
	
	if (!Settings.SuperFX)
	    return (0x30);
#ifdef SUPER_FX
	byte = Memory.FillRAM [Address];

//if (Address != 0x3030 && Address != 0x3031)
//printf ("%04x\n", Address);
#ifdef CPU_SHUTDOWN
	if (Address == 0x3030)
	{
	    CPU.WaitAddress = CPU.PCAtOpcodeStart;
	}
	else
#endif
	if (Address == 0x3031)
	{
		S9xClearIRQ(GSU_IRQ_SOURCE);
	    Memory.FillRAM [0x3031] = byte & 0x7f;
	}
	return (byte);
#endif
    }

    return (byte);
}

/**********************************************************************************************/
/* S9xSetCPU()                                                                                   */
/* This function sets a CPU/DMA Register to a specific byte                                   */
/**********************************************************************************************/
void S9xSetCPU (u8 byte, u16 Address)
{
    int d;
    
    if (Address < 0x4200)
    {
#ifdef VAR_CYCLES
	CPU.Cycles += ONE_CYCLE;
#endif
	switch (Address)
	{
	case 0x4016:
	    if ((byte & 1) && !(Memory.FillRAM [Address] & 1))
	    {
		PPU.Joypad1ButtonReadPos = 0;
		PPU.Joypad2ButtonReadPos = 0;
		PPU.Joypad3ButtonReadPos = 0;
	    }
	    break;
	case 0x4017:
	    break;
	default:
#ifdef DEBUGGER
	    missing.unknowncpu_write = Address;
	    if (Settings.TraceUnknownRegisters)
	    {
		sprintf (String, "Unknown register register write: $%02X->$%04X\n",
			 byte, Address);
		S9xMessage (S9X_TRACE, S9X_PPU_TRACE, String);
	    }
#endif
	    break;
	}
    }
    else
    switch (Address)
    {
    case 0x4200:
	// NMI, V & H IRQ and joypad reading enable flags
	if ((byte & 0x20) && 
	    (!SNESGameFixes.umiharakawaseFix || PPU.IRQVBeamPos < 209))
	{
	    if (!PPU.VTimerEnabled)
	    {
#ifdef DEBUGGER
		missing.virq = 1;
		missing.virq_pos = PPU.IRQVBeamPos;
#endif
		PPU.VTimerEnabled = TRUE;
		if (PPU.HTimerEnabled)
		    S9xUpdateHTimer ();
		else
		if (PPU.IRQVBeamPos == CPU.V_Counter)
		    S9xSetIRQ (PPU_V_BEAM_IRQ_SOURCE);
	    }
	}
	else
	{
	    PPU.VTimerEnabled = FALSE;
#ifndef RC_OPTIMIZED
					if (SNESGameFixes.umiharakawaseFix)
						byte &= ~0x20;
#endif
	}

	if (byte & 0x10)
	{
	    if (!PPU.HTimerEnabled)
	    {
#ifdef DEBUGGER
		missing.hirq = 1;
		missing.hirq_pos = PPU.IRQHBeamPos;
#endif
		PPU.HTimerEnabled = TRUE;
		S9xUpdateHTimer ();
	    }
	}
	else
	{
	    // No need to check for HTimer being disabled as the scanline
	    // event trigger code won't trigger an H-IRQ unless its enabled.
	    PPU.HTimerEnabled = FALSE;
	    PPU.HTimerPosition = Settings.H_Max + 1;
	}

#ifndef RC_OPTIMIZED
				if (!Settings.DaffyDuck)
					S9xClearIRQ(PPU_V_BEAM_IRQ_SOURCE | PPU_H_BEAM_IRQ_SOURCE);

				if ((byte & 0x80)
					&& !(Memory.FillRAM[0x4200] & 0x80)
					&& CPU.V_Counter >= PPU.ScreenHeight + FIRST_VISIBLE_LINE
					&& CPU.V_Counter <= PPU.ScreenHeight + (SNESGameFixes.alienVSpredetorFix ? 25 : 15)
					&& //jyam 15->25 alien vs predetor
				// Panic Bomberman clears the NMI pending flag @ scanline 230 before enabling
				// NMIs again. The NMI routine crashes the CPU if it is called without the NMI
				// pending flag being set...
				 (Memory.FillRAM[0x4210] & 0x80) && !CPU.NMIActive)
				{
					CPU.Flags |= NMI_FLAG;
					CPU.NMIActive = TRUE;
					CPU.NMICycleCount = CPU.NMITriggerPoint;
				}
#endif
	break;
    case 0x4201:
	// I/O port output 
    case 0x4202:
	// Multiplier (for multply)
	break;
    case 0x4203:
	{
	    // Multiplicand
		u32 res = Memory.FillRAM[0x4202] * byte;

		Memory.FillRAM[0x4216] = (u8) res;
		Memory.FillRAM[0x4217] = (u8) (res >> 8);
	    break;
	}
    case 0x4204:
    case 0x4205:
	// Low and high muliplier (for divide)
	break;
    case 0x4206:
	{
	    // Divisor
					u16 a =
						Memory.FillRAM[0x4204] + (Memory.FillRAM[0x4205] << 8);
		u16 div = byte ? a / byte : 0xffff;
		u16 rem = byte ? a % byte : a;

		Memory.FillRAM[0x4214] = (u8)div;
	    Memory.FillRAM[0x4215] = div >> 8;
		Memory.FillRAM[0x4216] = (u8)rem;
	    Memory.FillRAM[0x4217] = rem >> 8;
	    break;
	}
    case 0x4207:
	d = PPU.IRQHBeamPos;
	PPU.IRQHBeamPos = (PPU.IRQHBeamPos & 0xFF00) | byte;

	if (PPU.HTimerEnabled && PPU.IRQHBeamPos != d)
	    S9xUpdateHTimer ();
	break;

    case 0x4208:
	d = PPU.IRQHBeamPos;
	PPU.IRQHBeamPos = (PPU.IRQHBeamPos & 0xFF) | ((byte & 1) << 8);

	if (PPU.HTimerEnabled && PPU.IRQHBeamPos != d)
	    S9xUpdateHTimer ();

	break;

    case 0x4209:
	d = PPU.IRQVBeamPos;
	PPU.IRQVBeamPos = (PPU.IRQVBeamPos & 0xFF00) | byte;
#ifdef DEBUGGER
	missing.virq_pos = PPU.IRQVBeamPos;
#endif
	if (PPU.VTimerEnabled && PPU.IRQVBeamPos != d)
	{
	    if (PPU.HTimerEnabled)
		S9xUpdateHTimer ();
	    else
	    {
		if (PPU.IRQVBeamPos == CPU.V_Counter)
		    S9xSetIRQ (PPU_V_BEAM_IRQ_SOURCE);
	    }
	}
	break;

    case 0x420A:
	d = PPU.IRQVBeamPos;
	PPU.IRQVBeamPos = (PPU.IRQVBeamPos & 0xFF) | ((byte & 1) << 8);
#ifdef DEBUGGER
	missing.virq_pos = PPU.IRQVBeamPos;
#endif
	if (PPU.VTimerEnabled && PPU.IRQVBeamPos != d)
	{
	    if (PPU.HTimerEnabled)
		S9xUpdateHTimer ();
	    else
	    {
		if (PPU.IRQVBeamPos == CPU.V_Counter)
		    S9xSetIRQ (PPU_V_BEAM_IRQ_SOURCE);
	    }
	}
	break;

    case 0x420B:
#ifdef DEBUGGER
	missing.dma_this_frame = byte;
	missing.dma_channels = byte;
#endif
	if ((byte & 0x01) != 0)
	    S9xDoDMA (0);
	if ((byte & 0x02) != 0)
	    S9xDoDMA (1);
	if ((byte & 0x04) != 0)
	    S9xDoDMA (2);
	if ((byte & 0x08) != 0)
	    S9xDoDMA (3);
	if ((byte & 0x10) != 0)
	    S9xDoDMA (4);
	if ((byte & 0x20) != 0)
	    S9xDoDMA (5);
	if ((byte & 0x40) != 0)
	    S9xDoDMA (6);
	if ((byte & 0x80) != 0)
	    S9xDoDMA (7);
	break;
    case 0x420C:
#ifdef DEBUGGER
	missing.hdma_this_frame |= byte;
	missing.hdma_channels |= byte; 
#endif
	if (Settings.DisableHDMA)
	    byte = 0;
	Memory.FillRAM[0x420c] = byte;
	IPPU.HDMA = byte;
	break;

    case 0x420d:
	// Cycle speed 0 - 2.68Mhz, 1 - 3.58Mhz (banks 0x80 +)
	if ((byte & 1) != (Memory.FillRAM [0x420d] & 1))
	{
	    if (byte & 1)
	    {
		CPU.FastROMSpeed = ONE_CYCLE;
#ifdef DEBUGGER
		missing.fast_rom = 1;
#endif
	    }
	    else
		CPU.FastROMSpeed = SLOW_ONE_CYCLE;

	    Memory.FixROMSpeed ();
	}
	/* FALL */
    case 0x420e:
    case 0x420f:
	// --->>> Unknown
	break;
    case 0x4210:
	// NMI ocurred flag (reset on read or write)
	Memory.FillRAM[0x4210] = 0;
	return;
    case 0x4211:
	// IRQ ocurred flag (reset on read or write)
	S9xClearIRQ (PPU_V_BEAM_IRQ_SOURCE | PPU_H_BEAM_IRQ_SOURCE);
	break;
    case 0x4212:
	// v-blank, h-blank and joypad being scanned flags (read-only)
    case 0x4213:
	// I/O Port (read-only)
    case 0x4214:
    case 0x4215:
	// Quotent of divide (read-only)
    case 0x4216:
    case 0x4217:
	// Multiply product (read-only)
	return;
    case 0x4218:
    case 0x4219:
    case 0x421a:
    case 0x421b:
    case 0x421c:
    case 0x421d:
    case 0x421e:
    case 0x421f:
	// Joypad values (read-only)
	return;

    case 0x4300:
    case 0x4310:
    case 0x4320:
    case 0x4330:
    case 0x4340:
    case 0x4350:
    case 0x4360:
    case 0x4370:
	d = (Address >> 4) & 0x7;
	DMA[d].TransferDirection = (byte & 128) != 0 ? 1 : 0;
	DMA[d].HDMAIndirectAddressing = (byte & 64) != 0 ? 1 : 0;
	DMA[d].AAddressDecrement = (byte & 16) != 0 ? 1 : 0;
	DMA[d].AAddressFixed = (byte & 8) != 0 ? 1 : 0;
	DMA[d].TransferMode = (byte & 7);
	break;

    case 0x4301:
    case 0x4311:
    case 0x4321:
    case 0x4331:
    case 0x4341:
    case 0x4351:
    case 0x4361:
    case 0x4371:
	DMA[((Address >> 4) & 0x7)].BAddress = byte;
	break;

    case 0x4302:
    case 0x4312:
    case 0x4322:
    case 0x4332:
    case 0x4342:
    case 0x4352:
    case 0x4362:
    case 0x4372:
	d = (Address >> 4) & 0x7;
	DMA[d].AAddress &= 0xFF00;
	DMA[d].AAddress |= byte;
	break;

    case 0x4303:
    case 0x4313:
    case 0x4323:
    case 0x4333:
    case 0x4343:
    case 0x4353:
    case 0x4363:
    case 0x4373:
	d = (Address >> 4) & 0x7;
	DMA[d].AAddress &= 0xFF;
	DMA[d].AAddress |= byte << 8;
	break;

    case 0x4304:
    case 0x4314:
    case 0x4324:
    case 0x4334:
    case 0x4344:
    case 0x4354:
    case 0x4364:
    case 0x4374:
	DMA[((Address >> 4) & 0x7)].ABank = byte;
	break;

    case 0x4305:
    case 0x4315:
    case 0x4325:
    case 0x4335:
    case 0x4345:
    case 0x4355:
    case 0x4365:
    case 0x4375:
	d = (Address >> 4) & 0x7;
	DMA[d].TransferBytes &= 0xFF00;
	DMA[d].TransferBytes |= byte;
	DMA[d].IndirectAddress &= 0xff00;
	DMA[d].IndirectAddress |= byte;
	break;

    case 0x4306:
    case 0x4316:
    case 0x4326:
    case 0x4336:
    case 0x4346:
    case 0x4356:
    case 0x4366:
    case 0x4376:
	d = (Address >> 4) & 0x7;
	DMA[d].TransferBytes &= 0xFF;
	DMA[d].TransferBytes |= byte << 8;
	DMA[d].IndirectAddress &= 0xff;
	DMA[d].IndirectAddress |= byte << 8;
	break;

    case 0x4307:
    case 0x4317:
    case 0x4327:
    case 0x4337:
    case 0x4347:
    case 0x4357:
    case 0x4367:
    case 0x4377:
	DMA[d = ((Address >> 4) & 0x7)].IndirectBank = byte;
	break;

    case 0x4308:
    case 0x4318:
    case 0x4328:
    case 0x4338:
    case 0x4348:
    case 0x4358:
    case 0x4368:
    case 0x4378:
	d = (Address >> 4) & 7;
	DMA[d].Address &= 0xff00;
	DMA[d].Address |= byte;
	break;

    case 0x4309:
    case 0x4319:
    case 0x4329:
    case 0x4339:
    case 0x4349:
    case 0x4359:
    case 0x4369:
    case 0x4379:
	d = (Address >> 4) & 0x7;
	DMA[d].Address &= 0xff;
	DMA[d].Address |= byte << 8;
	break;

    case 0x430A:
    case 0x431A:
    case 0x432A:
    case 0x433A:
    case 0x434A:
    case 0x435A:
    case 0x436A:
    case 0x437A:
	d = (Address >> 4) & 0x7;
	DMA[d].LineCount = byte & 0x7f;
	DMA[d].Repeat = !(byte & 0x80);
	break;

    case 0x4800:
    case 0x4801:
    case 0x4802:
    case 0x4803:
//printf ("%02x->%04x\n", byte, Address);
	break;

    case 0x4804:
    case 0x4805:
    case 0x4806:
    case 0x4807:
//printf ("%02x->%04x\n", byte, Address);

	S9xSetSDD1MemoryMap (Address - 0x4804, byte & 7);
	break;
    default:
#ifdef DEBUGGER
	missing.unknowncpu_write = Address;
	if (Settings.TraceUnknownRegisters)
	{
					sprintf(
						String,
						"Unknown register write: $%02X->$%04X\n",
						byte,
						Address);
	    S9xMessage (S9X_TRACE, S9X_PPU_TRACE, String);
	}
#endif
	break;
    }
    Memory.FillRAM [Address] = byte;
}

/**********************************************************************************************/
/* S9xGetCPU()                                                                                   */
/* This function retrieves a CPU/DMA Register                                                 */
/**********************************************************************************************/
u8 S9xGetCPU (u16 Address)
{
	u8 byte;

    if (Address < 0x4200)
    {
#ifdef VAR_CYCLES
	CPU.Cycles += ONE_CYCLE;
#endif
	switch (Address)
	{
	// Secret of the Evermore
	case 0x4000:
	case 0x4001:
	    return (0x40);

	case 0x4016:
	{
	    if (Memory.FillRAM [0x4016] & 1)
	    {
						if ((!Settings.SwapJoypads
							&& IPPU.Controller == SNES_MOUSE_SWAPPED)
							|| (Settings.SwapJoypads
								&& IPPU.Controller == SNES_MOUSE))
		{
		    if (++PPU.MouseSpeed [0] > 2)
			PPU.MouseSpeed [0] = 0;
		}
		return (0);
	    }
		
	    int ind = Settings.SwapJoypads ? 1 : 0;
	    byte = IPPU.Joypads[ind] >> (PPU.Joypad1ButtonReadPos ^ 15);
	    PPU.Joypad1ButtonReadPos++;
	    return (byte & 1);
	}
	case 0x4017:
        {
	    if (Memory.FillRAM [0x4016] & 1)
	    {
		// MultiPlayer5 adaptor is only allowed to be plugged into port 2
		switch (IPPU.Controller)
		{
		case SNES_MULTIPLAYER5:
		    return (2);
		case SNES_MOUSE_SWAPPED:
								if (Settings.SwapJoypads
									&& ++PPU.MouseSpeed[0] > 2)
			PPU.MouseSpeed [0] = 0;
		    break;
		    
		case SNES_MOUSE:
								if (!Settings.SwapJoypads
									&& ++PPU.MouseSpeed[0] > 2)
			PPU.MouseSpeed [0] = 0;
		    break;
		}
		return (0x00);
	    }

	    int ind = Settings.SwapJoypads ? 0 : 1;

	    if (IPPU.Controller == SNES_MULTIPLAYER5)
	    {
		if (Memory.FillRAM [0x4201] & 0x80)
		{
							byte =
								((IPPU.Joypads[ind]
									>> (PPU.Joypad2ButtonReadPos ^ 15))
									& 1)
									| (((IPPU.Joypads[2]
										>> (PPU.Joypad2ButtonReadPos ^ 15))
										& 1)
										<< 1);
		    PPU.Joypad2ButtonReadPos++;
		    return (byte);
		}
		else
		{
							byte =
								((IPPU.Joypads[3]
									>> (PPU.Joypad3ButtonReadPos ^ 15))
									& 1)
									| (((IPPU.Joypads[4]
										>> (PPU.Joypad3ButtonReadPos ^ 15))
										& 1)
										<< 1);
							PPU.Joypad3ButtonReadPos++;
							return (byte);
						}
					}
					return (
						(IPPU.Joypads[ind]
							>> (PPU.Joypad2ButtonReadPos++ ^ 15))
							& 1);
        }
	default:
#ifdef DEBUGGER
	    missing.unknowncpu_read = Address;
	    if (Settings.TraceUnknownRegisters)
	    {
		sprintf (String, "Unknown register read: $%04X\n", Address);
		S9xMessage (S9X_TRACE, S9X_PPU_TRACE, String);
	    }
#endif
	    break;
	}
	return (Memory.FillRAM [Address]);
    }
    else
    switch (Address)
    {
	// BS Dynami Tracer! needs to be able to check if NMIs are enabled
	// already, otherwise the game locks up.
    case 0x4200:
	// NMI, h & v timers and joypad reading enable
	if (SNESGameFixes.Old_Read0x4200)
	{
#ifdef CPU_SHUTDOWN
           CPU.WaitAddress = CPU.PCAtOpcodeStart;
#endif
           return (REGISTER_4212());
	}
    case 0x4201:
	// I/O port (output - write only?)
    case 0x4202:
    case 0x4203:
	// Multiplier and multiplicand (write)
    case 0x4204:
    case 0x4205:
    case 0x4206:
	// Divisor and dividend (write)
	return (Memory.FillRAM[Address]);
    case 0x4207:
	return (u8)(PPU.IRQHBeamPos);
    case 0x4208:
	return (PPU.IRQHBeamPos >> 8);
    case 0x4209:
	return (u8)(PPU.IRQVBeamPos);
    case 0x420a:
	return (PPU.IRQVBeamPos >> 8);
    case 0x420b:
	// General purpose DMA enable
	// Super Formation Soccer 95 della Serie A UCC Xaqua requires this
	// register should not always return zero.
	// .. But Aero 2 waits until this register goes zero..
	// Just keep toggling the value for now in the hope that it breaks
	// the game out of its wait loop...
	Memory.FillRAM [0x420b] = !Memory.FillRAM [0x420b];
	return (Memory.FillRAM [0x420b]);
    case 0x420c:
	// H-DMA enable
	return (IPPU.HDMA);
    case 0x420d:
	// Cycle speed 0 - 2.68Mhz, 1 - 3.58Mhz (banks 0x80 +)
	return (Memory.FillRAM[Address]);
    case 0x420e:
    case 0x420f:
	// --->>> Unknown
	return (Memory.FillRAM[Address]);
    case 0x4210:
#ifdef CPU_SHUTDOWN
	CPU.WaitAddress = CPU.PCAtOpcodeStart;
#endif	
	byte = Memory.FillRAM[0x4210];
	Memory.FillRAM[0x4210] = 0;
	return (byte);
    case 0x4211:
				byte =
					(CPU.IRQActive
						& (PPU_V_BEAM_IRQ_SOURCE | PPU_H_BEAM_IRQ_SOURCE))
						? 0x80
						: 0;
	// Super Robot Wars Ex ROM bug requires this.
	byte |= CPU.Cycles >= Settings.HBlankStart ? 0x40 : 0;
	S9xClearIRQ(PPU_V_BEAM_IRQ_SOURCE | PPU_H_BEAM_IRQ_SOURCE);
	return (byte);
    case 0x4212:
	// V-blank, h-blank and joypads being read flags (read-only)
#ifdef CPU_SHUTDOWN
	CPU.WaitAddress = CPU.PCAtOpcodeStart;
#endif
	return (REGISTER_4212());
    case 0x4213:
	// I/O port input
    case 0x4214:
    case 0x4215:
	// Quotient of divide result
    case 0x4216:
    case 0x4217:
	// Multiplcation result (for multiply) or remainder of
	// divison.
	return (Memory.FillRAM[Address]);
    case 0x4218:
    case 0x4219:
    case 0x421a:
    case 0x421b:
    case 0x421c:
    case 0x421d:
    case 0x421e:
    case 0x421f:
	// Joypads 1-4 button and direction state.
	return (Memory.FillRAM [Address]);

    case 0x4300:
    case 0x4310:
    case 0x4320:
    case 0x4330:
    case 0x4340:
    case 0x4350:
    case 0x4360:
    case 0x4370:
	// DMA direction, address type, fixed flag,
	return (Memory.FillRAM[Address]);

    case 0x4301:
    case 0x4311:
    case 0x4321:
    case 0x4331:
    case 0x4341:
    case 0x4351:
    case 0x4361:
    case 0x4371:
	return (Memory.FillRAM[Address]);

    case 0x4302:
    case 0x4312:
    case 0x4322:
    case 0x4332:
    case 0x4342:
    case 0x4352:
    case 0x4362:
    case 0x4372:
	return (Memory.FillRAM[Address]);

    case 0x4303:
    case 0x4313:
    case 0x4323:
    case 0x4333:
    case 0x4343:
    case 0x4353:
    case 0x4363:
    case 0x4373:
	return (Memory.FillRAM[Address]);

    case 0x4304:
    case 0x4314:
    case 0x4324:
    case 0x4334:
    case 0x4344:
    case 0x4354:
    case 0x4364:
    case 0x4374:
	return (Memory.FillRAM[Address]);

    case 0x4305:
    case 0x4315:
    case 0x4325:
    case 0x4335:
    case 0x4345:
    case 0x4355:
    case 0x4365:
    case 0x4375:
	return (Memory.FillRAM[Address]);

    case 0x4306:
    case 0x4316:
    case 0x4326:
    case 0x4336:
    case 0x4346:
    case 0x4356:
    case 0x4366:
    case 0x4376:
	return (Memory.FillRAM[Address]);

    case 0x4307:
    case 0x4317:
    case 0x4327:
    case 0x4337:
    case 0x4347:
    case 0x4357:
    case 0x4367:
    case 0x4377:
	return (DMA[(Address >> 4) & 7].IndirectBank);

    case 0x4308:
    case 0x4318:
    case 0x4328:
    case 0x4338:
    case 0x4348:
    case 0x4358:
    case 0x4368:
    case 0x4378:
	return (Memory.FillRAM[Address]);

    case 0x4309:
    case 0x4319:
    case 0x4329:
    case 0x4339:
    case 0x4349:
    case 0x4359:
    case 0x4369:
    case 0x4379:
	return (Memory.FillRAM[Address]);

    case 0x430A:
    case 0x431A:
    case 0x432A:
    case 0x433A:
    case 0x434A:
    case 0x435A:
    case 0x436A:
    case 0x437A:
    {
	int d = (Address & 0x70) >> 4;
	if (IPPU.HDMA & (1 << d))
	{
	    return (DMA[d].LineCount);
	}
	return (Memory.FillRAM[Address]);
    }
    default:
#ifdef DEBUGGER
	missing.unknowncpu_read = Address;
	if (Settings.TraceUnknownRegisters)
	{
	    sprintf (String, "Unknown register read: $%04X\n", Address);
	    S9xMessage (S9X_TRACE, S9X_PPU_TRACE, String);
	}
	    
#endif
	break;
    }
    return (Memory.FillRAM[Address]);
}

void S9xResetPPU ()
{
    PPU.BGMode = 0;
    PPU.BG3Priority = 0;
    PPU.Brightness = 0;
    PPU.VMA.High = 0;
    PPU.VMA.Increment = 1;
    PPU.VMA.Address = 0;
    PPU.VMA.FullGraphicCount = 0;
    PPU.VMA.Shift = 0;

	for (u8 B = 0; B != 4; B++)
    {
	PPU.BG[B].SCBase = 0;
	PPU.BG[B].VOffset = 0;
	PPU.BG[B].HOffset = 0;
	PPU.BG[B].BGSize = 0;
	PPU.BG[B].NameBase = 0;
	PPU.BG[B].SCSize = 0;

	PPU.ClipCounts[B] = 0;
	PPU.ClipWindowOverlapLogic [B] = CLIP_OR;
	PPU.ClipWindow1Enable[B] = FALSE;
	PPU.ClipWindow2Enable[B] = FALSE;
	PPU.ClipWindow1Inside[B] = TRUE;
	PPU.ClipWindow2Inside[B] = TRUE;
    }

    PPU.ClipCounts[4] = 0;
    PPU.ClipCounts[5] = 0;
    PPU.ClipWindowOverlapLogic[4] = PPU.ClipWindowOverlapLogic[5] = CLIP_OR;
    PPU.ClipWindow1Enable[4] = PPU.ClipWindow1Enable[5] = FALSE;
    PPU.ClipWindow2Enable[4] = PPU.ClipWindow2Enable[5] = FALSE;
    PPU.ClipWindow1Inside[4] = PPU.ClipWindow1Inside[5] = TRUE;
    PPU.ClipWindow2Inside[4] = PPU.ClipWindow2Inside[5] = TRUE;
    
    PPU.CGFLIP = 0;
    int c;
    for (c = 0; c < 256; c++)
    {
	IPPU.Red [c] = (c & 7) << 2;
	IPPU.Green [c] = ((c >> 3) & 7) << 2;
	IPPU.Blue [c] = ((c >> 6) & 2) << 3;
		PPU.CGDATA[c] =
			IPPU.Red[c] | (IPPU.Green[c] << 5) | (IPPU.Blue[c] << 10);
    }

    PPU.FirstSprite = 0;
    PPU.LastSprite = 127;
    for (int Sprite = 0; Sprite < 128; Sprite++)
    {
	PPU.OBJ[Sprite].HPos = 0;
	PPU.OBJ[Sprite].VPos = 0;
	PPU.OBJ[Sprite].VFlip = 0;
	PPU.OBJ[Sprite].HFlip = 0;
	PPU.OBJ[Sprite].Priority = 0;
	PPU.OBJ[Sprite].Palette = 0;
	PPU.OBJ[Sprite].Name = 0;
	PPU.OBJ[Sprite].Size = 0;
    }
    PPU.OAMPriorityRotation = 0;

    PPU.OAMFlip = 0;
    PPU.OAMTileAddress = 0;
    PPU.OAMAddr = 0;
    PPU.IRQVBeamPos = 0;
    PPU.IRQHBeamPos = 0;
    PPU.VBeamPosLatched = 0;
    PPU.HBeamPosLatched = 0;

    PPU.HBeamFlip = 0;
    PPU.VBeamFlip = 0;
    PPU.HVBeamCounterLatched = 0;

    PPU.MatrixA = PPU.MatrixB = PPU.MatrixC = PPU.MatrixD = 0;
    PPU.CentreX = PPU.CentreY = 0;
    PPU.Joypad1ButtonReadPos = 0;
    PPU.Joypad2ButtonReadPos = 0;
    PPU.Joypad3ButtonReadPos = 0;

    PPU.CGADD = 0;
    PPU.FixedColourRed = PPU.FixedColourGreen = PPU.FixedColourBlue = 0;
    PPU.SavedOAMAddr = 0;
    PPU.ScreenHeight = SNES_HEIGHT;
    PPU.WRAM = 0;
    PPU.BG_Forced = 0;
    PPU.ForcedBlanking = TRUE;
    PPU.OBJThroughMain = FALSE;
    PPU.OBJThroughSub = FALSE;
    PPU.OBJSizeSelect = 0;
    PPU.OBJNameSelect = 0;
    PPU.OBJNameBase = 0;
    PPU.OBJAddition = FALSE;
    PPU.OAMReadFlip = 0;
    ZeroMemory (PPU.OAMData, 512 + 32);
    
    PPU.VTimerEnabled = FALSE;
    PPU.HTimerEnabled = FALSE;
    PPU.HTimerPosition = Settings.H_Max + 1;
    PPU.Mosaic = 0;
    PPU.BGMosaic [0] = PPU.BGMosaic [1] = FALSE;
    PPU.BGMosaic [2] = PPU.BGMosaic [3] = FALSE;
    PPU.Mode7HFlip = FALSE;
    PPU.Mode7VFlip = FALSE;
    PPU.Mode7Repeat = 0;
    PPU.Window1Left = 1;
    PPU.Window1Right = 0;
    PPU.Window2Left = 1;
    PPU.Window2Right = 0;
    PPU.RecomputeClipWindows = TRUE;
    PPU.CGFLIPRead = 0;
    PPU.Need16x8Mulitply = FALSE;
    PPU.MouseSpeed[0] = PPU.MouseSpeed[1] = 0;

    IPPU.ColorsChanged = TRUE;
    IPPU.HDMA = 0;
    IPPU.HDMAStarted = FALSE;
    IPPU.MaxBrightness = 0;
    IPPU.LatchedBlanking = 0;
    IPPU.OBJChanged = TRUE;
    IPPU.RenderThisFrame = TRUE;
    IPPU.DirectColourMapsNeedRebuild = TRUE;
    IPPU.FrameCount = 0;
    IPPU.RenderedFramesCount = 0;
    IPPU.DisplayedRenderedFrameCount = 0;
	ZeroMemory(IPPU.TileCached[TILE_2BIT], MAX_2BIT_TILES);
	ZeroMemory(IPPU.TileCached[TILE_4BIT], MAX_4BIT_TILES);
	ZeroMemory(IPPU.TileCached[TILE_8BIT], MAX_8BIT_TILES);
	IPPU.FirstVRAMRead = FALSE;
	IPPU.LatchedInterlace = FALSE;
	IPPU.DoubleWidthPixels = FALSE;
    IPPU.RenderedScreenWidth = SNES_WIDTH;
    IPPU.RenderedScreenHeight = SNES_HEIGHT;
    IPPU.XB = NULL;
    for (c = 0; c < 256; c++)
		IPPU.ScreenColors [c] = c;
    S9xFixColourBrightness ();
    IPPU.PreviousLine = IPPU.CurrentLine = 0;
    IPPU.Joypads[0] = IPPU.Joypads[1] = IPPU.Joypads[2] = 0;
    IPPU.Joypads[3] = IPPU.Joypads[4] = 0;
    IPPU.SuperScope = 0;
    IPPU.Mouse[0] = IPPU.Mouse[1] = 0;
    IPPU.PrevMouseX[0] = IPPU.PrevMouseX[1] = 256 / 2;
    IPPU.PrevMouseY[0] = IPPU.PrevMouseY[1] = 224 / 2;

    if (Settings.ControllerOption == 0)
	IPPU.Controller = SNES_MAX_CONTROLLER_OPTIONS - 1;
    else
	IPPU.Controller = Settings.ControllerOption - 1;
    S9xNextController ();

    for (c = 0; c < 2; c++)
	memset (&IPPU.Clip [c], 0, sizeof (struct ClipData));

    if (Settings.MouseMaster)
    {
	S9xProcessMouse (0);
	S9xProcessMouse (1);
    }
    for (c = 0; c < 0x8000; c += 0x100)
	memset (&Memory.FillRAM [c], c >> 8, 0x100);

    ZeroMemory (&Memory.FillRAM [0x2100], 0x100);
    ZeroMemory (&Memory.FillRAM [0x4200], 0x100);
    ZeroMemory (&Memory.FillRAM [0x4000], 0x100);
    // For BS Suttehakkun 2...
    ZeroMemory (&Memory.FillRAM [0x1000], 0x1000);
}

void S9xProcessMouse (int which1)
{
    int x, y;
	u32 buttons;
    
	if ((IPPU.Controller == SNES_MOUSE
		|| IPPU.Controller == SNES_MOUSE_SWAPPED)
		&& S9xReadMousePosition(which1, x, y, buttons))
	{
		int delta_x, delta_y;
#define MOUSE_SIGNATURE 0x1
		IPPU.Mouse[which1] =
			MOUSE_SIGNATURE
				| (PPU.MouseSpeed[which1] << 4)
				| ((buttons & 1) << 6)
				| ((buttons & 2) << 6);

	delta_x = x - IPPU.PrevMouseX[which1];
	delta_y = y - IPPU.PrevMouseY[which1];

	if (delta_x > 63)
	{
	    delta_x = 63;
	    IPPU.PrevMouseX[which1] += 63;
	}
		else if (delta_x < -63)
	{
	    delta_x = -63;
	    IPPU.PrevMouseX[which1] -= 63;
	}
	else
	    IPPU.PrevMouseX[which1] = x;

	if (delta_y > 63)
	{
	    delta_y = 63;
	    IPPU.PrevMouseY[which1] += 63;
	}
		else if (delta_y < -63)
	{
	    delta_y = -63;
	    IPPU.PrevMouseY[which1] -= 63;
	}
	else
	    IPPU.PrevMouseY[which1] = y;

	if (delta_x < 0)
	{
	    delta_x = -delta_x;
	    IPPU.Mouse [which1] |= (delta_x | 0x80) << 16;
	}
	else
	    IPPU.Mouse [which1] |= delta_x << 16;

	if (delta_y < 0)
	{
	    delta_y = -delta_y;
	    IPPU.Mouse [which1] |= (delta_y | 0x80) << 24;
	}
	else
	    IPPU.Mouse [which1] |= delta_y << 24;

	if (IPPU.Controller == SNES_MOUSE_SWAPPED)
	    IPPU.Joypads [0] = IPPU.Mouse [which1];
	else
	    IPPU.Joypads [1] = IPPU.Mouse [which1];
    }
}

void ProcessSuperScope ()
{
    int x, y;
	u32 buttons;
    
	if (IPPU.Controller == SNES_SUPERSCOPE
		&& S9xReadSuperScopePosition(x, y, buttons))
    {
#define SUPERSCOPE_SIGNATURE 0x00ff
	u32 scope;

		scope =
			SUPERSCOPE_SIGNATURE
				| ((buttons & 1) << (7 + 8))
				| ((buttons & 2) << (5 + 8))
				| ((buttons & 4) << (3 + 8))
				| ((buttons & 8) << (1 + 8));
	if (x > 255)
	    x = 255;
	if (x < 0)
	    x = 0;
	if (y > PPU.ScreenHeight - 1)
	    y = PPU.ScreenHeight - 1;
	if (y < 0)
	    y = 0;

	PPU.VBeamPosLatched = (u16) (y + 1);
	PPU.HBeamPosLatched = (u16) x;
	PPU.HVBeamCounterLatched = TRUE;
	Memory.FillRAM [0x213F] |= 0x40;
	IPPU.Joypads [1] = scope;
    }
}

void S9xNextController ()
{
    switch (IPPU.Controller)
    {
    case SNES_MULTIPLAYER5:
	IPPU.Controller = SNES_JOYPAD;
	break;
    case SNES_JOYPAD:
	if (Settings.MouseMaster)
	{
	    IPPU.Controller = SNES_MOUSE_SWAPPED;
	    break;
	}
    case SNES_MOUSE_SWAPPED:
	if (Settings.MouseMaster)
	{
	    IPPU.Controller = SNES_MOUSE;
	    break;
	}
    case SNES_MOUSE:
	if (Settings.SuperScopeMaster)
	{
	    IPPU.Controller = SNES_SUPERSCOPE;
	    break;
	}
    case SNES_SUPERSCOPE:
	if (Settings.MultiPlayer5Master)
	{
	    IPPU.Controller = SNES_MULTIPLAYER5;
	    break;
	}
    default:
	IPPU.Controller = SNES_JOYPAD;
	break;
    }
}

void S9xUpdateJoypads ()
{
    int i;

    for (i = 0; i < 5; i++)
    {
	IPPU.Joypads [i] = snesEmu.gamePad(i);
	if (IPPU.Joypads [i] & SNES_LEFT_MASK)
	    IPPU.Joypads [i] &= ~SNES_RIGHT_MASK;
	if (IPPU.Joypads [i] & SNES_UP_MASK)
	    IPPU.Joypads [i] &= ~SNES_DOWN_MASK;
    }

    //touhaiden controller Fix
	if (SNESGameFixes.TouhaidenControllerFix
		&& (IPPU.Controller == SNES_JOYPAD
			|| IPPU.Controller == SNES_MULTIPLAYER5))
    {
	for (i = 0; i < 5; i++)
	{
	    if (IPPU.Joypads [i])
		IPPU.Joypads [i] |= 0xffff0000;
	}
    }

    // Read mouse position if enabled
    if (Settings.MouseMaster)
    {
	for (i = 0; i < 2; i++)
	    S9xProcessMouse (i);
    }

    // Read SuperScope if enabled
    if (Settings.SuperScopeMaster)
	ProcessSuperScope ();

    if (Memory.FillRAM [0x4200] & 1)
    {
	PPU.Joypad1ButtonReadPos = 16;
	if (Memory.FillRAM [0x4201] & 0x80)
	{
	    PPU.Joypad2ButtonReadPos = 16;
	    PPU.Joypad3ButtonReadPos = 0;
	}
	else
	{
	    PPU.Joypad2ButtonReadPos = 0;
	    PPU.Joypad3ButtonReadPos = 16;
	}
	int ind = Settings.SwapJoypads ? 1 : 0;

	Memory.FillRAM [0x4218] = (u8) IPPU.Joypads [ind];
	Memory.FillRAM [0x4219] = (u8) (IPPU.Joypads [ind] >> 8);
	Memory.FillRAM [0x421a] = (u8) IPPU.Joypads [ind ^ 1];
	Memory.FillRAM [0x421b] = (u8) (IPPU.Joypads [ind ^ 1] >> 8);
	if (Memory.FillRAM [0x4201] & 0x80)
	{
		Memory.FillRAM [0x421c] = (u8) IPPU.Joypads [ind];
		Memory.FillRAM [0x421d] = (u8) (IPPU.Joypads [ind] >> 8);
		Memory.FillRAM [0x421e] = (u8) IPPU.Joypads [2];
		Memory.FillRAM [0x421f] = (u8) (IPPU.Joypads [2] >> 8);
	}
	else
	{
		Memory.FillRAM [0x421c] = (u8) IPPU.Joypads [3];
		Memory.FillRAM [0x421d] = (u8) (IPPU.Joypads [3] >> 8);
		Memory.FillRAM [0x421e] = (u8) IPPU.Joypads [4];
		Memory.FillRAM [0x421f] = (u8) (IPPU.Joypads [4] >> 8);
	}
    }
}


#ifdef SUPER_FX
void S9xSuperFXExec ()
{
#if 1
	if (Settings.SuperFX)
	{
		if ((Memory.FillRAM[0x3000 + GSU_SFR] & FLG_G)
			&& (Memory.FillRAM[0x3000 + GSU_SCMR] & 0x18) == 0x18)
	{
	    if (!Settings.WinterGold)
		FxEmulate (~0);
			else FxEmulate((Memory.FillRAM[0x3000 + GSU_CLSR] & 1) ? 700 : 350);
			int GSUStatus = Memory.FillRAM[0x3000
					+ GSU_SFR] | (Memory.FillRAM[0x3000 + GSU_SFR + 1] << 8);
	    if ((GSUStatus & (FLG_G | FLG_IRQ)) == FLG_IRQ)
	    {
		// Trigger a GSU IRQ.
		S9xSetIRQ (GSU_IRQ_SOURCE);
	    }
	}
	}
#else
	u32 tmp = (Memory.FillRAM[0x3034] << 16)
		+ * (u16 *) & Memory.FillRAM[0x301e];
#if 0
    if (tmp == 0x018428)
	{
		* (u16 *)
		& SRAM[0x0064] = 0xbc00;
		* (u16 *) & SRAM[0x002c] = 0x8000;
	}
#endif
    if (tmp == -1)//0x018428) //0x01bfc3) //0x09edaf) //-1) //0x57edaf)
    {
	while (Memory.FillRAM [0x3030] & 0x20)
	{
	    int i;
		s32 vError;
		u8 avReg[0x40];
	    char tmp[128];
		u8 vPipe;
		u8 vColr;
		u8 vPor;

	    FxPipeString (tmp);
			/* Make the string 32 chars long */
			if (strlen(tmp) < 32)
			{
				memset(& tmp[strlen(tmp)], ' ', 32 - strlen(tmp));
				tmp[32] = 0;
			}
	    /* Copy registers (so we can see if any changed) */
	    vColr = FxGetColorRegister();
	    vPor = FxGetPlotOptionRegister();
	    memcpy(avReg,SuperFX.pvRegisters,0x40);

	    /* Print the pipe string */
	    printf(tmp);

	    /* Execute the instruction in the pipe */
	    vPipe = FxPipe();
	    vError = FxEmulate(1);

	    /* Check if any registers changed (and print them if they did) */
	    for(i=0; i<16; i++)
	    {
		u32 a = 0;
				u32 r1 =
					((u32) avReg[i * 2])
						| (((u32) avReg[(i * 2) + 1]) << 8);
				u32 r2 =
					(u32) (SuperFX.pvRegisters[i * 2])
						| (((u32) SuperFX.pvRegisters[(i * 2) + 1]) << 8);
		if(i==15)
		    a = OPCODE_BYTES(vPipe);
		if(((r1+a)&0xffff) != r2)
		    printf(" r%d=$%04x",i,r2);
	    }
	    {
				/* Check SFR */
				u32 r1 =
					((u32) avReg[0x30]) | (((u32) avReg[0x31]) << 8);
				u32 r2 =
					(u32) (SuperFX.pvRegisters[0x30])
						| (((u32) SuperFX.pvRegisters[0x31]) << 8);
		if((r1&(1<<1)) != (r2&(1<<1)))
			printf(" Z=%d",(u32)(!!(r2&(1<<1))));
		if((r1&(1<<2)) != (r2&(1<<2)))
			printf(" CY=%d",(u32)(!!(r2&(1<<2))));
		if((r1&(1<<3)) != (r2&(1<<3)))
			printf(" S=%d",(u32)(!!(r2&(1<<3))));
		if((r1&(1<<4)) != (r2&(1<<4)))
			printf(" OV=%d",(u32)(!!(r2&(1<<4))));
		if((r1&(1<<5)) != (r2&(1<<5)))
			printf(" G=%d",(u32)(!!(r2&(1<<5))));
		if((r1&(1<<6)) != (r2&(1<<6)))
			printf(" R=%d",(u32)(!!(r2&(1<<6))));
		if((r1&(1<<8)) != (r2&(1<<8)))
			printf(" ALT1=%d",(u32)(!!(r2&(1<<8))));
		if((r1&(1<<9)) != (r2&(1<<9)))
			printf(" ALT2=%d",(u32)(!!(r2&(1<<9))));
		if((r1&(1<<10)) != (r2&(1<<10)))
			printf(" IL=%d",(u32)(!!(r2&(1<<10))));
		if((r1&(1<<11)) != (r2&(1<<11)))
			printf(" IH=%d",(u32)(!!(r2&(1<<11))));
		if((r1&(1<<12)) != (r2&(1<<12)))
			printf(" B=%d",(u32)(!!(r2&(1<<12))));
		if((r1&(1<<15)) != (r2&(1<<15)))
			printf(" IRQ=%d",(u32)(!!(r2&(1<<15))));
	    }
	    {
		/* Check PBR */
		u32 r1 = ((u32)avReg[0x34]);
		u32 r2 = (u32)(SuperFX.pvRegisters[0x34]);
		if(r1 != r2)
		    printf(" PBR=$%02x",r2);
	    }
	    {
		/* Check ROMBR */
		u32 r1 = ((u32)avReg[0x36]);
		u32 r2 = (u32)(SuperFX.pvRegisters[0x36]);
		if(r1 != r2)
		    printf(" ROMBR=$%02x",r2);
	    }
	    {
		/* Check RAMBR */
		u32 r1 = ((u32)avReg[0x3c]);
		u32 r2 = (u32)(SuperFX.pvRegisters[0x3c]);
		if(r1 != r2)
		    printf(" RAMBR=$%02x",r2);
	    }
	    {
		/* Check CBR */
				u32 r1 =
					((u32) avReg[0x3e]) | (((u32) avReg[0x3f]) << 8);
				u32 r2 =
					(u32) (SuperFX.pvRegisters[0x3e])
						| (((u32) SuperFX.pvRegisters[0x3f]) << 8);
		if(r1 != r2)
		    printf(" CBR=$%04x",r2);
	    }
	    {
		/* Check COLR */
		if(vColr != FxGetColorRegister())
		    printf(" COLR=$%02x",FxGetColorRegister());
	    }
	    {
		/* Check POR */
		if(vPor != FxGetPlotOptionRegister())
		    printf(" POR=$%02x",FxGetPlotOptionRegister());
	    }
	    printf ("\n");
	}
	S9xExit ();
    }
    else
	{
		u32 t =
			(Memory.FillRAM[0x3034] << 16)
				+ (Memory.FillRAM[0x301f] << 8)
				+ (Memory.FillRAM[0x301e] << 0);

printf ("%06x: %d\n", t, FxEmulate (2000000));
//	FxEmulate (2000000);
    }
#if 0
    if (!(CPU.Flags & TRACE_FLAG))
    {
		static int z = 1; if (z == 0)
	{
	    extern FILE *trace;
	    CPU.Flags |= TRACE_FLAG;
	    trace = fopen ("trace.log", "wb");
	}
	else
	z--;
    }
#endif
    Memory.FillRAM [0x3030] &= ~0x20;
    if (Memory.FillRAM [0x3031] & 0x80)
    {
	S9xSetIRQ (GSU_IRQ_SOURCE);
    }
#endif
}
#endif

void snesPpuSl() {
	emsl.begin("ppu");
	emsl.var("bgMode", PPU.BGMode);
	emsl.var("bg3Priority", PPU.BG3Priority);
	emsl.var("brightness", PPU.Brightness);

	emsl.var("cgFlip", PPU.CGFLIP);
	emsl.array("cgData", PPU.CGDATA, sizeof(PPU.CGDATA));
	emsl.var("firstSprite", PPU.FirstSprite);

	emsl.var("oamPriorityRotation", PPU.OAMPriorityRotation);
	emsl.var("oamAddr", PPU.OAMAddr);
	emsl.var("oamFlip", PPU.OAMFlip);
	emsl.var("oamTileAddress", PPU.OAMTileAddress);
	emsl.var("irqVBeamPos", PPU.IRQVBeamPos);
	emsl.var("irqHBeamPos", PPU.IRQHBeamPos);
	emsl.var("vBeamPosLatched", PPU.VBeamPosLatched);
	emsl.var("hBeamPosLatched", PPU.HBeamPosLatched);
	emsl.var("vBeamFlip", PPU.VBeamFlip);
	emsl.var("hBeamFlip", PPU.HBeamFlip);
	emsl.var("hvBeamCounterLatched", PPU.HVBeamCounterLatched);
	emsl.var("matrixA", PPU.MatrixA);
	emsl.var("matrixB", PPU.MatrixB);
	emsl.var("matrixC", PPU.MatrixC);
	emsl.var("matrixD", PPU.MatrixD);
	emsl.var("matrixX", PPU.CentreX);
	emsl.var("matrixY", PPU.CentreY);
	emsl.var("joypad1ButtonReadPos", PPU.Joypad1ButtonReadPos);
	emsl.var("joypad2ButtonReadPos", PPU.Joypad2ButtonReadPos);
	emsl.var("joypad3ButtonReadPos", PPU.Joypad3ButtonReadPos);
	emsl.var("cgAdd", PPU.CGADD);
	emsl.var("fixedColorRed", PPU.FixedColourRed);
	emsl.var("fixedColorGreen", PPU.FixedColourGreen);
	emsl.var("fixedColorBlue", PPU.FixedColourBlue);
	emsl.var("savedOamAddr", PPU.SavedOAMAddr);
	emsl.var("screenHeight", PPU.ScreenHeight);
	emsl.var("wram", PPU.WRAM);
	emsl.var("forcedBlanking", PPU.ForcedBlanking);
	emsl.var("objNameSelect", PPU.OBJNameSelect);
	emsl.var("objSizeSelect", PPU.OBJSizeSelect);
	emsl.var("objNameBase", PPU.OBJNameBase);
	emsl.var("oamReadFlip", PPU.OAMReadFlip);
	emsl.var("vTimerEnabled", PPU.VTimerEnabled);
	emsl.var("hTimerEnabled", PPU.HTimerEnabled);
	emsl.var("hTimerPosition", PPU.HTimerPosition);
	emsl.var("mosaic", PPU.Mosaic);
	emsl.var("mode7VFlip", PPU.Mode7VFlip);
	emsl.var("mode7HFlip", PPU.Mode7HFlip);
	emsl.var("mode7Repeat", PPU.Mode7Repeat);
	emsl.var("window1Left", PPU.Window1Left);
	emsl.var("window1Right", PPU.Window1Right);
	emsl.var("window2Left", PPU.Window2Left);
	emsl.var("window2Right", PPU.Window2Right);

	emsl.var("cgFlipRead", PPU.CGFLIPRead);
	emsl.var("need16x8Multiply", PPU.Need16x8Mulitply);
	emsl.array("bgMosaic", PPU.BGMosaic, sizeof(PPU.BGMosaic));
	emsl.array("oamData", PPU.OAMData, sizeof(PPU.OAMData));
	emsl.array("mouseSpeed", PPU.MouseSpeed, sizeof(PPU.MouseSpeed));
	emsl.end();

	emsl.begin("ppu.vma");
	emsl.var("high", PPU.VMA.High);
	emsl.var("increment", PPU.VMA.Increment);
	emsl.var("address", PPU.VMA.Address);
	emsl.var("mask1", PPU.VMA.Mask1);
	emsl.var("fullGraphicCount", PPU.VMA.FullGraphicCount);
	emsl.var("shift", PPU.VMA.Shift	);
	emsl.end();

	for (int i = 0; i < 4; i++) {
		emsl.begin(QString("ppu.bg[%1]").arg(i));
		emsl.var("scBase", PPU.BG[i].SCBase);
		emsl.var("vOffset", PPU.BG[i].VOffset);
		emsl.var("hOffset", PPU.BG[i].HOffset);
		emsl.var("bgSize", PPU.BG[i].BGSize);
		emsl.var("nameBase", PPU.BG[i].NameBase);
		emsl.var("scSize", PPU.BG[i].SCSize);
		emsl.end();
	}
	for (int i = 0; i < 128; i++) {
		emsl.begin(QString("ppu.obj[%1]").arg(i));
		emsl.var("hPos", PPU.OBJ[i].HPos);
		emsl.var("vPos", PPU.OBJ[i].VPos);
		emsl.var("name", PPU.OBJ[i].Name);
		emsl.var("vFlip", PPU.OBJ[i].VFlip);
		emsl.var("hFlip", PPU.OBJ[i].HFlip);
		emsl.var("priorirty", PPU.OBJ[i].Priority);
		emsl.var("palette", PPU.OBJ[i].Palette);
		emsl.var("size", PPU.OBJ[i].Size);
		emsl.end();
	}
	for (int i = 0; i < 6; i++) {
		emsl.begin(QString("ppu.clip[%1]").arg(i));
		emsl.var("clipWindowOverlapLogic", PPU.ClipWindowOverlapLogic[i]);
		emsl.var("clipWindow1Enable", PPU.ClipWindow1Enable[i]);
		emsl.var("clipWindow2Enable", PPU.ClipWindow2Enable[i]);
		emsl.var("clipWindow1Inside", PPU.ClipWindow1Inside[i]);
		emsl.var("clipWindow2Inside", PPU.ClipWindow2Inside[i]);
		emsl.end();
	}
	if (!emsl.save) {
		IPPU.ColorsChanged = TRUE;
		IPPU.OBJChanged = TRUE;
		S9xFixColourBrightness();
	}
}
