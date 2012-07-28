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
#ifndef _MISSING_H_
#define _MISSING_H_

struct HDMA
{
    u8 used;
    u8 bbus_address;
    u8 abus_bank;
    u16 abus_address;
    u8 indirect_address;
    u8 force_table_address_write;
    u8 force_table_address_read;
    u8 line_count_write;
    u8 line_count_read;
};

struct Missing
{
    u8 emulate6502;
    u8 decimal_mode;
    u8 mv_8bit_index;
    u8 mv_8bit_acc;
    u8 interlace;
    u8 lines_239;
    u8 pseudo_512;
    struct HDMA hdma [8];
    u8 modes [8];
    u8 mode7_fx;
    u8 mode7_flip;
    u8 mode7_bgmode;
    u8 direct;
    u8 matrix_multiply;
    u8 oam_read;
    u8 vram_read;
    u8 cgram_read;
    u8 wram_read;
    u8 dma_read;
    u8 vram_inc;
    u8 vram_full_graphic_inc;
    u8 virq;
    u8 hirq;
    u16 virq_pos;
    u16 hirq_pos;
    u8 h_v_latch;
    u8 h_counter_read;
    u8 v_counter_read;
    u8 fast_rom;
    u8 window1 [6];
    u8 window2 [6];
    u8 sprite_priority_rotation;
    u8 subscreen;
    u8 subscreen_add;
    u8 subscreen_sub;
    u8 fixed_colour_add;
    u8 fixed_colour_sub;
    u8 mosaic;
    u8 sprite_double_height;
    u8 dma_channels;
    u8 dma_this_frame;
    u8 oam_address_read;
    u8 bg_offset_read;
    u8 matrix_read;
    u8 hdma_channels;
    u8 hdma_this_frame;
    u16 unknownppu_read;
    u16 unknownppu_write;
    u16 unknowncpu_read;
    u16 unknowncpu_write;
    u16 unknowndsp_read;
    u16 unknowndsp_write;
};

EXTERN_C struct Missing missing;
#endif
