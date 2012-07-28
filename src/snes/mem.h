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
#ifndef SNESMEMORY_H
#define SNESMEMORY_H

#include <base/emu.h>
#include "snes9x.h"

extern void snesMemSl();

#ifdef FAST_LSB_WORD_ACCESS
#define READ_WORD(s) (*(u16 *) (s))
#define READ_DWORD(s) (*(u32 *) (s))
#define WRITE_WORD(s, d) (*(u16 *) (s) = (d)
#define WRITE_DWORD(s, d) (*(u32 *) (s) = (d)
#else
#define READ_WORD(s) ( *(u8 *) (s) |\
		      (*((u8 *) (s) + 1) << 8))
#define READ_DWORD(s) ( *(u8 *) (s) |\
		       (*((u8 *) (s) + 1) << 8) |\
		       (*((u8 *) (s) + 2) << 16) |\
		       (*((u8 *) (s) + 3) << 24))
#define WRITE_WORD(s, d) *(u8 *) (s) = (d), \
                         *((u8 *) (s) + 1) = (d) >> 8
#define WRITE_DWORD(s, d) *(u8 *) (s) = (u8) (d), \
                          *((u8 *) (s) + 1) = (u8) ((d) >> 8),\
                          *((u8 *) (s) + 2) = (u8) ((d) >> 16),\
                          *((u8 *) (s) + 3) = (u8) ((d) >> 24)
#define WRITE_3WORD(s, d) *(u8 *) (s) = (u8) (d), \
                          *((u8 *) (s) + 1) = (u8) ((d) >> 8),\
                          *((u8 *) (s) + 2) = (u8) ((d) >> 16)
#define READ_3WORD(s) ( *(u8 *) (s) |\
                       (*((u8 *) (s) + 1) << 8) |\
                       (*((u8 *) (s) + 2) << 16))
                          
#endif

#define MEMMAP_BLOCK_SIZE (0x1000)
#define MEMMAP_NUM_BLOCKS (0x1000000 / MEMMAP_BLOCK_SIZE)
#define MEMMAP_BLOCKS_PER_BANK (0x10000 / MEMMAP_BLOCK_SIZE)
#define MEMMAP_SHIFT 12
#define MEMMAP_MASK (MEMMAP_BLOCK_SIZE - 1)
#define MEMMAP_MAX_SDD1_LOGGED_ENTRIES (0x10000 / 8)

class CMemory {
public:
    bool8_32 LoadROM (const char *);
    void  InitROM (bool8_32);
    bool8_32 Init ();
    void  Deinit ();
    void  FreeSDD1Data ();
    
    void WriteProtectROM ();
    void FixROMSpeed ();
    void MapRAM ();
    void MapExtraRAM ();
    char *Safe (const char *);
    
    void LoROMMap ();
    void LoROM24MBSMap ();
    void SRAM512KLoROMMap ();
    void SRAM1024KLoROMMap ();
    void SufamiTurboLoROMMap ();
    void HiROMMap ();
    void SuperFXROMMap ();
    void TalesROMMap (bool8_32);
    void AlphaROMMap ();
    void SA1ROMMap ();
    void BSHiROMMap ();
    bool8_32 AllASCII (u8 *b, int size);
    int  ScoreHiROM (bool8_32 skip_header);
    int  ScoreLoROM (bool8_32 skip_header);
    void ApplyROMFixes ();
    void CheckForIPSPatch (const char *rom_filename, bool8_32 header,
			   s32 &rom_size);
    
    const char *TVStandard ();
    const char *Speed ();
    const char *StaticRAMSize ();
    const char *MapType ();
    const char *MapMode ();
    const char *KartContents ();
    const char *Size ();
    const char *Headers ();
    const char *ROMID ();
    const char *CompanyID ();
    
    enum {
	MAP_PPU, MAP_CPU, MAP_DSP, MAP_LOROM_SRAM, MAP_HIROM_SRAM,
	MAP_NONE, MAP_DEBUG, MAP_C4, MAP_BWRAM, MAP_BWRAM_BITMAP,
	MAP_BWRAM_BITMAP2, MAP_SA1RAM, MAP_LAST
    };
    enum { MAX_ROM_SIZE = 0x400000 }; //4Mo for now
    
    u8 *RAM;
    u8 *ROM;
    u8 *VRAM;
    u8 *SRAM;
    u8 *BWRAM;
    u8 *FillRAM;
    u8 *C4RAM;
    bool8_32 HiROM;
    bool8_32 LoROM;
    u16 SRAMMask;
    u8 SRAMSize;
    u8 *Map [MEMMAP_NUM_BLOCKS];
    u8 *WriteMap [MEMMAP_NUM_BLOCKS];
    u8 MemorySpeed [MEMMAP_NUM_BLOCKS];
    u8 BlockIsRAM [MEMMAP_NUM_BLOCKS];
    u8 BlockIsROM [MEMMAP_NUM_BLOCKS];
    char  ROMName [ROM_NAME_LEN];
    char  ROMId [5];
    char  CompanyId [3];
    u8 ROMSpeed;
    u8 ROMType;
    u8 ROMSize;
    s32 ROMFramesPerSecond;
    s32 HeaderCount;
    u32 CalculatedSize;
    u32 CalculatedChecksum;
    u32 ROMChecksum;
    u32 ROMComplementChecksum;
    u8  *SDD1Index;
    u8  *SDD1Data;
    u32 SDD1Entries;
    u32 SDD1LoggedDataCountPrev;
    u32 SDD1LoggedDataCount;
    u8  SDD1LoggedData [MEMMAP_MAX_SDD1_LOGGED_ENTRIES];
    char ROMFilename [_MAX_PATH];
};

START_EXTERN_C
extern CMemory Memory;
extern u8 *SRAM;
extern u8 *ROM;
extern u8 *RegRAM;
void S9xDeinterleaveMode2 ();
END_EXTERN_C

#ifdef NO_INLINE_SET_GET
u8 S9xGetByte (u32 Address, struct SCPUState *);
u16 S9xGetWord (u32 Address, struct SCPUState *);
void S9xSetByte (u8 Byte, u32 Address, struct SCPUState * );
void S9xSetWord (u16 Byte, u32 Address, struct SCPUState *);
void S9xSetPCBase (u32 Address, struct SCPUState *);
u8 *S9xGetMemPointer (u32 Address);
u8 *GetBasePointer (u32 Address);
#else
#define INLINE inline
#include "getset.h"
#endif // NO_INLINE_SET_GET

#endif // SNESMEMORY_H
