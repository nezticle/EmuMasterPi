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
#ifndef SNESPPU_H
#define SNESPPU_H

#include "port.h"
#include "snes.h"

extern void snesPpuSl();

#define FIRST_VISIBLE_LINE 1

extern u8 GetBank;
extern u16 SignExtend [2];

#define TILE_2BIT 0
#define TILE_4BIT 1
#define TILE_8BIT 2

#define MAX_2BIT_TILES 4096
#define MAX_4BIT_TILES 2048
#define MAX_8BIT_TILES 1024

#define PPU_H_BEAM_IRQ_SOURCE	(1 << 0)
#define PPU_V_BEAM_IRQ_SOURCE	(1 << 1)
#define GSU_IRQ_SOURCE		(1 << 2)
#define SA1_IRQ_SOURCE		(1 << 7)
#define SA1_DMA_IRQ_SOURCE	(1 << 5)

struct ClipData {
	u32  Count [6];
	u32  Left [6][6];
	u32  Right [6][6];
};

struct InternalPPU {
    bool8_32  ColorsChanged;
	u8  HDMA;
    bool8_32  HDMAStarted;
	u8  MaxBrightness;
    bool8_32  LatchedBlanking;
    bool8_32  OBJChanged;
    bool8_32  RenderThisFrame;
    bool8_32  DirectColourMapsNeedRebuild;
	u32 FrameCount;
	u32 RenderedFramesCount;
	u32 DisplayedRenderedFrameCount;
	u32 FrameSkip;
	u8  *TileCache [3];
	u8  *TileCached [3];
    bool8_32  FirstVRAMRead;
    bool8_32  LatchedInterlace;
    bool8_32  DoubleWidthPixels;
    int    RenderedScreenHeight;
    int    RenderedScreenWidth;
	u32 Red [256];
	u32 Green [256];
	u32 Blue [256];
	u8  *XB;
	u16 ScreenColors [256];
    int	   PreviousLine;
    int	   CurrentLine;
    int	   Controller;
	u32 Joypads[5];
	u32 SuperScope;
	u32 Mouse[2];
    int    PrevMouseX[2];
    int    PrevMouseY[2];
    struct ClipData Clip [2];
};

struct SOBJ
{
    short  HPos;
	u16 VPos;
	u16 Name;
	u8  VFlip;
	u8  HFlip;
	u8  Priority;
	u8  Palette;
	u8  Size;
};

struct SPPU {
	u8  BGMode;
	u8  BG3Priority;
	u8  Brightness;

    struct {
	bool8_32 High;
	u8 Increment;
	u16 Address;
	u16 Mask1;
	u16 FullGraphicCount;
	u16 Shift;
    } VMA;

    struct {
	u16 SCBase;
	u16 VOffset;
	u16 HOffset;
	u8 BGSize;
	u16 NameBase;
	u16 SCSize;
    } BG [4];

    bool8_32  CGFLIP;
	u16 CGDATA [256];
	u8  FirstSprite;
	u8  LastSprite;
    struct SOBJ OBJ [128];
	u8  OAMPriorityRotation;
	u16 OAMAddr;

	u8  OAMFlip;
	u16 OAMTileAddress;
	u16 IRQVBeamPos;
	u16 IRQHBeamPos;
	u16 VBeamPosLatched;
	u16 HBeamPosLatched;

	u8  HBeamFlip;
	u8  VBeamFlip;
	u8  HVBeamCounterLatched;

    short  MatrixA;
    short  MatrixB;
    short  MatrixC;
    short  MatrixD;
    short  CentreX;
    short  CentreY;
	u8  Joypad1ButtonReadPos;
	u8  Joypad2ButtonReadPos;

	u8  CGADD;
	u8  FixedColourRed;
	u8  FixedColourGreen;
	u8  FixedColourBlue;
	u16 SavedOAMAddr;
	u16 ScreenHeight;
	u32 WRAM;
	u8  BG_Forced;
    bool8_32  ForcedBlanking;
    bool8_32  OBJThroughMain;
    bool8_32  OBJThroughSub;
	u8  OBJSizeSelect;
	u16 OBJNameBase;
    bool8_32  OBJAddition;
	u8  OAMReadFlip;
	u8  OAMData [512 + 32];
    bool8_32  VTimerEnabled;
    bool8_32  HTimerEnabled;
    short  HTimerPosition;
	u8  Mosaic;
    bool8_32  BGMosaic [4];
    bool8_32  Mode7HFlip;
    bool8_32  Mode7VFlip;
	u8  Mode7Repeat;
	u8  Window1Left;
	u8  Window1Right;
	u8  Window2Left;
	u8  Window2Right;
	u8  ClipCounts [6];
	u8  ClipWindowOverlapLogic [6];
	u8  ClipWindow1Enable [6];
	u8  ClipWindow2Enable [6];
    bool8_32  ClipWindow1Inside [6];
    bool8_32  ClipWindow2Inside [6];
    bool8_32  RecomputeClipWindows;
	u8  CGFLIPRead;
	u16 OBJNameSelect;
    bool8_32  Need16x8Mulitply;
	u8  Joypad3ButtonReadPos;
	u8  MouseSpeed[2];
};

#define CLIP_OR 0
#define CLIP_AND 1
#define CLIP_XOR 2
#define CLIP_XNOR 3

struct SDMA {
    bool8_32  TransferDirection;
    bool8_32  AAddressFixed;
    bool8_32  AAddressDecrement;
	u8  TransferMode;

	u8  ABank;
	u16 AAddress;
	u16 Address;
	u8  BAddress;

    // General DMA only:
	u16 TransferBytes;

    // H-DMA only:
    bool8_32  HDMAIndirectAddressing;
	u16 IndirectAddress;
	u8  IndirectBank;
	u8  Repeat;
	u8  LineCount;
	u8  FirstLine;
};

START_EXTERN_C
void S9xUpdateScreen ();
void S9xResetPPU ();
void S9xFixColourBrightness ();
void S9xUpdateJoypads ();
void S9xProcessMouse(int which1);
void S9xSuperFXExec ();

void S9xSetPPU (u8 Byte, u16 Address);
u8 S9xGetPPU (u16 Address);
void S9xSetCPU (u8 Byte, u16 Address);
u8 S9xGetCPU (u16 Address);

void S9xInitC4 ();
void S9xSetC4 (u8 Byte, u16 Address);
u8 S9xGetC4 (u16 Address);
void S9xSetC4RAM (u8 Byte, u16 Address);
u8 S9xGetC4RAM (u16 Address);

extern struct SPPU PPU;
extern struct SDMA DMA [8];
extern struct InternalPPU IPPU;
END_EXTERN_C

#include "gfx.h"
#include "mem.h"

STATIC inline u8 REGISTER_4212()
{
    GetBank = 0;
    if (CPU.V_Counter >= PPU.ScreenHeight + FIRST_VISIBLE_LINE &&
	CPU.V_Counter < PPU.ScreenHeight + FIRST_VISIBLE_LINE + 3)
	GetBank = 1;

    GetBank |= CPU.Cycles >= Settings.HBlankStart ? 0x40 : 0;
    if (CPU.V_Counter >= PPU.ScreenHeight + FIRST_VISIBLE_LINE)
	GetBank |= 0x80; /* XXX: 0x80 or 0xc0 ? */

    return (GetBank);
}

STATIC inline void FLUSH_REDRAW ()
{
    if (IPPU.PreviousLine != IPPU.CurrentLine)
	S9xUpdateScreen ();
}

STATIC inline void REGISTER_2104 (u8 byte)
{
    if (PPU.OAMAddr >= 0x110)
	return;
	
    int addr = (PPU.OAMAddr << 1) + (PPU.OAMFlip & 1);
    
    if (byte != PPU.OAMData [addr])
    {
	FLUSH_REDRAW ();
	PPU.OAMData [addr] = byte;
	IPPU.OBJChanged = TRUE;
	if (addr & 0x200)
	{
	    // X position high bit, and sprite size (x4)
	    struct SOBJ *pObj = &PPU.OBJ [(addr & 0x1f) * 4];

	    pObj->HPos = (pObj->HPos & 0xFF) | SignExtend[(byte >> 0) & 1];
	    pObj++->Size = byte & 2;
	    pObj->HPos = (pObj->HPos & 0xFF) | SignExtend[(byte >> 2) & 1];
	    pObj++->Size = byte & 8;
	    pObj->HPos = (pObj->HPos & 0xFF) | SignExtend[(byte >> 4) & 1];
	    pObj++->Size = byte & 32;
	    pObj->HPos = (pObj->HPos & 0xFF) | SignExtend[(byte >> 6) & 1];
	    pObj->Size = byte & 128;
	}
	else
	{
	    if (addr & 1)
	    {
		if (addr & 2)
		{
		    addr = PPU.OAMAddr >> 1;
		    // Tile within group, priority, h and v flip.
		    PPU.OBJ[addr].Name &= 0xFF;
			PPU.OBJ[addr].Name |= ((u16) (byte & 1)) << 8;
		    PPU.OBJ[addr].Palette = (byte >> 1) & 7;
		    PPU.OBJ[addr].Priority = (byte >> 4) & 3;
		    PPU.OBJ[addr].HFlip = (byte >> 6) & 1;
		    PPU.OBJ[addr].VFlip = (byte >> 7) & 1;
		}
		else
		{
		    // Sprite Y position
		    PPU.OBJ[PPU.OAMAddr >> 1].VPos = byte;
		}
	    }
	    else
	    {
		if (addr & 2)
		{
		    // Tile group
		    
		    PPU.OBJ[addr = PPU.OAMAddr >> 1].Name &= 0x100;
		    PPU.OBJ[addr].Name |= byte;
		}
		else
		{
		    // X position (low)
		    PPU.OBJ[addr = PPU.OAMAddr >> 1].HPos &= 0xFF00;
		    PPU.OBJ[addr].HPos |= byte;
		}
	    }
	}
    }
    PPU.OAMFlip ^= 1;
    if (!(PPU.OAMFlip & 1))
	PPU.OAMAddr++;

    Memory.FillRAM [0x2104] = byte;
}

STATIC inline void REGISTER_2118 (u8 Byte)
{
	u32 address;
    if (PPU.VMA.FullGraphicCount)
    {
	u32 rem = PPU.VMA.Address & PPU.VMA.Mask1;
	address = (((PPU.VMA.Address & ~PPU.VMA.Mask1) +
			 (rem >> PPU.VMA.Shift) +
			 ((rem & (PPU.VMA.FullGraphicCount - 1)) << 3)) << 1) & 0xffff;
	Memory.VRAM [address] = Byte;
    }
    else
    {
	Memory.VRAM[address = (PPU.VMA.Address << 1) & 0xFFFF] = Byte;
    }
    IPPU.TileCached [TILE_2BIT][address >> 4] = FALSE;
    IPPU.TileCached [TILE_4BIT][address >> 5] = FALSE;
    IPPU.TileCached [TILE_8BIT][address >> 6] = FALSE;
    if (!PPU.VMA.High)
    {
#ifdef DEBUGGER
	if (Settings.TraceVRAM && !CPU.InDMA)
	{
	    printf ("VRAM write byte: $%04X (%d,%d)\n", PPU.VMA.Address,
		    Memory.FillRAM[0x2115] & 3,
		    (Memory.FillRAM [0x2115] & 0x0c) >> 2);
	}
#endif	
	PPU.VMA.Address += PPU.VMA.Increment;
    }
//    Memory.FillRAM [0x2118] = Byte;
}

STATIC inline void REGISTER_2118_tile (u8 Byte)
{
	u32 address;
	u32 rem = PPU.VMA.Address & PPU.VMA.Mask1;
    address = (((PPU.VMA.Address & ~PPU.VMA.Mask1) +
		 (rem >> PPU.VMA.Shift) +
		 ((rem & (PPU.VMA.FullGraphicCount - 1)) << 3)) << 1) & 0xffff;
    Memory.VRAM [address] = Byte;
    IPPU.TileCached [TILE_2BIT][address >> 4] = FALSE;
    IPPU.TileCached [TILE_4BIT][address >> 5] = FALSE;
    IPPU.TileCached [TILE_8BIT][address >> 6] = FALSE;
    if (!PPU.VMA.High)
	PPU.VMA.Address += PPU.VMA.Increment;
//    Memory.FillRAM [0x2118] = Byte;
}

STATIC inline void REGISTER_2118_linear (u8 Byte)
{
	u32 address;
    Memory.VRAM[address = (PPU.VMA.Address << 1) & 0xFFFF] = Byte;
    IPPU.TileCached [TILE_2BIT][address >> 4] = FALSE;
    IPPU.TileCached [TILE_4BIT][address >> 5] = FALSE;
    IPPU.TileCached [TILE_8BIT][address >> 6] = FALSE;
    if (!PPU.VMA.High)
	PPU.VMA.Address += PPU.VMA.Increment;
//    Memory.FillRAM [0x2118] = Byte;
}

STATIC inline void REGISTER_2119 (u8 Byte)
{
	u32 address;
    if (PPU.VMA.FullGraphicCount)
    {
	u32 rem = PPU.VMA.Address & PPU.VMA.Mask1;
	address = ((((PPU.VMA.Address & ~PPU.VMA.Mask1) +
		    (rem >> PPU.VMA.Shift) +
		    ((rem & (PPU.VMA.FullGraphicCount - 1)) << 3)) << 1) + 1) & 0xFFFF;
	Memory.VRAM [address] = Byte;
    }
    else
    {
	Memory.VRAM[address = ((PPU.VMA.Address << 1) + 1) & 0xFFFF] = Byte;
    }
    IPPU.TileCached [TILE_2BIT][address >> 4] = FALSE;
    IPPU.TileCached [TILE_4BIT][address >> 5] = FALSE;
    IPPU.TileCached [TILE_8BIT][address >> 6] = FALSE;
    if (PPU.VMA.High)
    {
#ifdef DEBUGGER
	if (Settings.TraceVRAM && !CPU.InDMA)
	{
	    printf ("VRAM write word: $%04X (%d,%d)\n", PPU.VMA.Address,
		    Memory.FillRAM[0x2115] & 3,
		    (Memory.FillRAM [0x2115] & 0x0c) >> 2);
	}
#endif	
	PPU.VMA.Address += PPU.VMA.Increment;
    }
//    Memory.FillRAM [0x2119] = Byte;
}

STATIC inline void REGISTER_2119_tile (u8 Byte)
{
	u32 rem = PPU.VMA.Address & PPU.VMA.Mask1;
	u32 address = ((((PPU.VMA.Address & ~PPU.VMA.Mask1) +
		    (rem >> PPU.VMA.Shift) +
		    ((rem & (PPU.VMA.FullGraphicCount - 1)) << 3)) << 1) + 1) & 0xFFFF;
    Memory.VRAM [address] = Byte;
    IPPU.TileCached [TILE_2BIT][address >> 4] = FALSE;
    IPPU.TileCached [TILE_4BIT][address >> 5] = FALSE;
    IPPU.TileCached [TILE_8BIT][address >> 6] = FALSE;
    if (PPU.VMA.High)
	PPU.VMA.Address += PPU.VMA.Increment;
//    Memory.FillRAM [0x2119] = Byte;
}

STATIC inline void REGISTER_2119_linear (u8 Byte)
{
	u32 address;
    Memory.VRAM[address = ((PPU.VMA.Address << 1) + 1) & 0xFFFF] = Byte;
    IPPU.TileCached [TILE_2BIT][address >> 4] = FALSE;
    IPPU.TileCached [TILE_4BIT][address >> 5] = FALSE;
    IPPU.TileCached [TILE_8BIT][address >> 6] = FALSE;
    if (PPU.VMA.High)
	PPU.VMA.Address += PPU.VMA.Increment;
//    Memory.FillRAM [0x2119] = Byte;
}

STATIC inline void REGISTER_2122(u8 Byte)
{
    // CG-RAM (palette) write

    if (PPU.CGFLIP)
    {
	if ((Byte & 0x7f) != (PPU.CGDATA[PPU.CGADD] >> 8))
	{
	    if (Settings.SixteenBit&& !(Settings.os9x_hack&PPU_IGNORE_PALWRITE)){
		FLUSH_REDRAW ();
			}
	    PPU.CGDATA[PPU.CGADD] &= 0x00FF;
	    PPU.CGDATA[PPU.CGADD] |= (Byte & 0x7f) << 8;
	    IPPU.ColorsChanged = TRUE;
	    if (Settings.SixteenBit)
	    {
		IPPU.Blue [PPU.CGADD] = IPPU.XB [(Byte >> 2) & 0x1f];
		IPPU.Green [PPU.CGADD] = IPPU.XB [(PPU.CGDATA[PPU.CGADD] >> 5) & 0x1f];
		IPPU.ScreenColors [PPU.CGADD] = (u16) BUILD_PIXEL (IPPU.Red [PPU.CGADD],
							     IPPU.Green [PPU.CGADD],
							     IPPU.Blue [PPU.CGADD]);
	    }	  
	}
	PPU.CGADD++;
    }
    else
    {
	if (Byte != (u8) (PPU.CGDATA[PPU.CGADD] & 0xff))
	{
	    if (Settings.SixteenBit&& !(Settings.os9x_hack&PPU_IGNORE_PALWRITE)){
		FLUSH_REDRAW ();
			}
	    PPU.CGDATA[PPU.CGADD] &= 0x7F00;
	    PPU.CGDATA[PPU.CGADD] |= Byte;
	    IPPU.ColorsChanged = TRUE;
	    if (Settings.SixteenBit)
	    {
		IPPU.Red [PPU.CGADD] = IPPU.XB [Byte & 0x1f];
		IPPU.Green [PPU.CGADD] = IPPU.XB [(PPU.CGDATA[PPU.CGADD] >> 5) & 0x1f];
		IPPU.ScreenColors [PPU.CGADD] = (u16) BUILD_PIXEL (IPPU.Red [PPU.CGADD],
							     IPPU.Green [PPU.CGADD],
							     IPPU.Blue [PPU.CGADD]);
	    }	 
	}
    }
    PPU.CGFLIP ^= 1;
//    Memory.FillRAM [0x2122] = Byte;
}

STATIC inline void REGISTER_2180(u8 Byte)
{
    Memory.RAM[PPU.WRAM++] = Byte;
    PPU.WRAM &= 0x1FFFF;
    Memory.FillRAM [0x2180] = Byte;
}

#endif // SNESPPU_H
