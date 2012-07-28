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
#ifndef _GFX_H_
#define _GFX_H_

#include "port.h"

struct SGFX{
    // Initialize these variables
    u8  *Screen;
    u8  *SubScreen;
    u8  *ZBuffer;
    u8  *SubZBuffer;
    u32 Pitch;

    // Setup in call to S9xGraphicsInit()
    int   Delta;
    u16 *X2;
    u16 *ZERO_OR_X2;
    u16 *ZERO;
    u32 RealPitch; // True pitch of Screen buffer.
    u32 Pitch2;    // Same as RealPitch except while using speed up hack for Glide.
    u32 ZPitch;    // Pitch of ZBuffer
    u32 PPL;	      // Number of pixels on each of Screen buffer
    u32 PPLx2;
    u32 PixSize;
    u8  *S;
    u8  *DB;
    u16 *ScreenColors;
    u32 DepthDelta;
    u8  Z1;
    u8  Z2;
    u32 FixedColour;
    const char *InfoString;
    u32 InfoStringTimeout;
    u32 StartY;
    u32 EndY;
    struct ClipData *pCurrentClip;
    u32 Mode7Mask;
    u32 Mode7PriorityMask;
    int	   OBJList [129];
    u32 Sizes [129];
    int    VPositions [129];

    u8  r212c;
    u8  r212d;
    u8  r2130;
    u8  r2131;
    bool8_32  Pseudo;
    
#ifdef GFX_MULTI_FORMAT
    u32 PixelFormat;
    u32 (*BuildPixel) (u32 R, u32 G, u32 B);
    u32 (*BuildPixel2) (u32 R, u32 G, u32 B);
    void   (*DecomposePixel) (u32 Pixel, u32 &R, u32 &G, u32 &B);
#endif
};

struct SLineData {
    struct {
	u16 VOffset;
	u16 HOffset;
    } BG [4];
};

#define H_FLIP 0x4000
#define V_FLIP 0x8000
#define BLANK_TILE 2
#define IN_VIDEO_MEMORY 3

struct SBG
{
    u32 TileSize;
    u32 BitShift;
    u32 TileShift;
    u32 TileAddress;
    u32 NameSelect;
    u32 SCBase;

    u32 StartPalette;
    u32 PaletteShift;
    u32 PaletteMask;
    
	u8 *Buffer;
    u8 *Buffered;
    bool8_32  DirectColourMode;
};

struct SLineMatrixData
{
    short MatrixA;
    short MatrixB;
    short MatrixC;
    short MatrixD;
    short CentreX;
    short CentreY;
};

extern u32 odd_high [4][16];
extern u32 odd_low [4][16];
extern u32 even_high [4][16];
extern u32 even_low [4][16];
extern SBG BG;
extern u16 DirectColourMaps [8][256];

//extern u8 add32_32 [32][32];
//extern u8 add32_32_half [32][32];
//extern u8 sub32_32 [32][32];
//extern u8 sub32_32_half [32][32];
extern u8 mul_brightness [16][32];

// Could use BSWAP instruction on Intel port...
#define SWAP_DWORD(dw) dw = ((dw & 0xff) << 24) | ((dw & 0xff00) << 8) | \
		            ((dw & 0xff0000) >> 8) | ((dw & 0xff000000) >> 24)

#ifdef FAST_LSB_WORD_ACCESS
#define READ_2BYTES(s) (*(u16 *) (s))
#define WRITE_2BYTES(s, d) *(u16 *) (s) = (d)
#else
#ifdef LSB_FIRST
#define READ_2BYTES(s) (*(u8 *) (s) | (*((u8 *) (s) + 1) << 8))
#define WRITE_2BYTES(s, d) *(u8 *) (s) = (d), \
			   *((u8 *) (s) + 1) = (d) >> 8
#else  // else MSB_FISRT
#define READ_2BYTES(s) (*(u8 *) (s) | (*((u8 *) (s) + 1) << 8))
#define WRITE_2BYTES(s, d) *(u8 *) (s) = (d), \
			   *((u8 *) (s) + 1) = (d) >> 8
#endif // LSB_FIRST
#endif // i386
#define SUB_SCREEN_DEPTH 0
#define MAIN_SCREEN_DEPTH 32

#if defined(OLD_COLOUR_BLENDING)
#define COLOR_ADD(C1, C2) \
GFX.X2 [((((C1) & RGB_REMOVE_LOW_BITS_MASK) + \
	  ((C2) & RGB_REMOVE_LOW_BITS_MASK)) >> 1) + \
	((C1) & (C2) & RGB_LOW_BITS_MASK)]
#else
#define COLOR_ADD(C1, C2) \
(GFX.X2 [((((C1) & RGB_REMOVE_LOW_BITS_MASK) + \
	  ((C2) & RGB_REMOVE_LOW_BITS_MASK)) >> 1) + \
	 ((C1) & (C2) & RGB_LOW_BITS_MASK)] | \
 (((C1) ^ (C2)) & RGB_LOW_BITS_MASK))	   
#endif

#define COLOR_ADD1_2(C1, C2) \
((((((C1) & RGB_REMOVE_LOW_BITS_MASK) + \
          ((C2) & RGB_REMOVE_LOW_BITS_MASK)) >> 1) + \
         ((C1) & (C2) & RGB_LOW_BITS_MASK)) | ALPHA_BITS_MASK)

#if defined(OLD_COLOUR_BLENDING)
#define COLOR_SUB(C1, C2) \
GFX.ZERO_OR_X2 [(((C1) | RGB_HI_BITS_MASKx2) - \
		 ((C2) & RGB_REMOVE_LOW_BITS_MASK)) >> 1]
#else
#define COLOR_SUB(C1, C2) \
(GFX.ZERO_OR_X2 [(((C1) | RGB_HI_BITS_MASKx2) - \
                  ((C2) & RGB_REMOVE_LOW_BITS_MASK)) >> 1] + \
((C1) & RGB_LOW_BITS_MASK) - ((C2) & RGB_LOW_BITS_MASK))
#endif

		 
#define COLOR_SUB1_2(C1, C2) \
GFX.ZERO [(((C1) | RGB_HI_BITS_MASKx2) - \
	   ((C2) & RGB_REMOVE_LOW_BITS_MASK)) >> 1]

typedef void (*NormalTileRenderer) (u32 Tile, u32 Offset, 
				    u32 StartLine, u32 LineCount, struct SGFX * gfx);
typedef void (*ClippedTileRenderer) (u32 Tile, u32 Offset,
				     u32 StartPixel, u32 Width,
				     u32 StartLine, u32 LineCount, struct SGFX * gfx);
typedef void (*LargePixelRenderer) (u32 Tile, u32 Offset,
				    u32 StartPixel, u32 Pixels,
				    u32 StartLine, u32 LineCount, struct SGFX * gfx);


START_EXTERN_C
void S9xStartScreenRefresh ();
void S9xDrawScanLine (u8 Line);
void S9xEndScreenRefresh ();
void S9xSetupOBJ (struct SOBJ *);
void S9xUpdateScreen ();
void RenderLine (u8 line);
void S9xBuildDirectColourMaps ();

// External port interface which must be implemented or initialised for each
// port.
extern struct SGFX GFX;

bool8_32 S9xGraphicsInit ();
void S9xGraphicsDeinit();
bool8_32 S9xDeinitUpdate (int Width, int Height, bool8_32 sixteen_bit);
//void S9xSetPalette ();

#ifdef GFX_MULTI_FORMAT
bool8_32 S9xSetRenderPixelFormat (int format);
#endif

END_EXTERN_C

#endif
