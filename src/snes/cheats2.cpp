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
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "snes9x.h"
#include "cheats.h"
#include "mem.h"

extern SCheatData Cheat;

void S9xInitCheatData ()
{
    Cheat.RAM = Memory.RAM;
    Cheat.SRAM = ::SRAM;
    Cheat.FillRAM = Memory.FillRAM;
}

void S9xAddCheat (bool8 enable, bool8 save_current_value, 
		  u32 address, u8 byte)
{
    if (Cheat.num_cheats < sizeof (Cheat.c) / sizeof (Cheat. c [0]))
    {
	Cheat.c [Cheat.num_cheats].address = address;
	Cheat.c [Cheat.num_cheats].byte = byte;
	Cheat.c [Cheat.num_cheats].enabled = TRUE;
	if (save_current_value)
	{
	    Cheat.c [Cheat.num_cheats].saved_byte = S9xGetByte (address);
	    Cheat.c [Cheat.num_cheats].saved = TRUE;
	}
	Cheat.num_cheats++;
    }
}

void S9xDeleteCheat (u32 which1)
{
    if (which1 < Cheat.num_cheats)
    {
	if (Cheat.c [which1].enabled)
	    S9xRemoveCheat (which1);

	memmove (&Cheat.c [which1], &Cheat.c [which1 + 1],
		 sizeof (Cheat.c [0]) * (Cheat.num_cheats - which1 - 1));
	Cheat.num_cheats = 0;
    }
}

void S9xDeleteCheats ()
{
    S9xRemoveCheats ();
    Cheat.num_cheats = 0;
}

void S9xEnableCheat (u32 which1)
{
    if (which1 < Cheat.num_cheats && !Cheat.c [which1].enabled)
    {
	Cheat.c [which1].enabled = TRUE;
	S9xApplyCheat (which1);
    }
}

void S9xDisableCheat (u32 which1)
{
    if (which1 < Cheat.num_cheats && Cheat.c [which1].enabled)
    {
	S9xRemoveCheat (which1);
	Cheat.c [which1].enabled = FALSE;
    }
}

void S9xRemoveCheat (u32 which1)
{
    if (Cheat.c [which1].saved)
    {
	u32 address = Cheat.c [which1].address;

	int block = (address >> MEMMAP_SHIFT) & MEMMAP_MASK;
	u8 *ptr = Memory.Map [block];
	    
	if (ptr >= (u8 *) CMemory::MAP_LAST)
	    *(ptr + (address & 0xffff)) = Cheat.c [which1].saved_byte;
	else
	    S9xSetByte (address, Cheat.c [which1].saved_byte);
    }
}

void S9xApplyCheat (u32 which1)
{
    u32 address = Cheat.c [which1].address;

    if (!Cheat.c [which1].saved)
	Cheat.c [which1].saved_byte = S9xGetByte (address);

    int block = (address >> MEMMAP_SHIFT) & MEMMAP_MASK;
    u8 *ptr = Memory.Map [block];
    
    if (ptr >= (u8 *) CMemory::MAP_LAST)
	*(ptr + (address & 0xffff)) = Cheat.c [which1].byte;
    else
	S9xSetByte (address, Cheat.c [which1].byte);
    Cheat.c [which1].saved = TRUE;
}

void S9xApplyCheats ()
{
    if (Settings.ApplyCheats)
    {
        for (u32 i = 0; i < Cheat.num_cheats; i++)
            if (Cheat.c [i].enabled)
                S9xApplyCheat (i);
    }
}

void S9xRemoveCheats ()
{
    for (u32 i = 0; i < Cheat.num_cheats; i++)
	if (Cheat.c [i].enabled)
	    S9xRemoveCheat (i);
}

bool8 S9xLoadCheatFile (const char *filename)
{
    Cheat.num_cheats = 0;

    FILE *fs = fopen (filename, "rb");
    u8 data [28];

    if (!fs)
	return (FALSE);

    while (fread ((void *) data, 1, 28, fs) == 28)
    {
	Cheat.c [Cheat.num_cheats].enabled = (data [0] & 4) == 0;
	Cheat.c [Cheat.num_cheats].byte = data [1];
	Cheat.c [Cheat.num_cheats].address = data [2] | (data [3] << 8) |  (data [4] << 16);
	Cheat.c [Cheat.num_cheats].saved_byte = data [5];
	Cheat.c [Cheat.num_cheats].saved = (data [0] & 8) != 0;
	memmove (Cheat.c [Cheat.num_cheats].name, &data [8], 20);
	Cheat.c [Cheat.num_cheats++].name [20] = 0;
    }
    fclose (fs);

    return (TRUE);
}

bool8 S9xSaveCheatFile (const char *filename)
{
    if (Cheat.num_cheats == 0)
    {
    /*
#ifndef _SNESPPC
	(void) remove (filename);
#endif*/
	return (TRUE);
    }

    FILE *fs = fopen (filename, "wb");
    u8 data [28];

    if (!fs)
	return (FALSE);

    u32 i;
    for (i = 0; i < Cheat.num_cheats; i++)
    {
	memset (data, 0, 28);
	if (i == 0)
	{
	    data [6] = 254;
	    data [7] = 252;
	}
	if (!Cheat.c [i].enabled)
	    data [0] |= 4;

	if (Cheat.c [i].saved)
	    data [0] |= 8;

	data [1] = Cheat.c [i].byte;
	data [2] = (u8) Cheat.c [i].address;
	data [3] = (u8) (Cheat.c [i].address >> 8);
	data [4] = (u8) (Cheat.c [i].address >> 16);
	data [5] = Cheat.c [i].saved_byte;

	memmove (&data [8], Cheat.c [i].name, 19);
	if (fwrite (data, 28, 1, fs) != 1)
	{
	    fclose (fs);
	    return (FALSE);
	}
    }
    return (fclose (fs) == 0);
}

