/* gameplaySP
 *
 * Copyright (C) 2006 Exophase <exophase@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef GBAGPU_H
#define GBAGPU_H

#include "common.h"

#if defined(__cplusplus)
class GbaGpu {
public:
	void sl();
};

extern GbaGpu gbaGpu;

extern "C" {
#endif

void update_scanline();

void debug_screen_printf(const char *format, ...);
void debug_screen_printl(const char *format, ...);
void debug_screen_newline(u32 count);

extern s32 affine_reference_x[2];
extern s32 affine_reference_y[2];

typedef void (* tile_render_function)(u32 layer_number, u32 start, u32 end,
 void *dest_ptr);
typedef void (* bitmap_render_function)(u32 start, u32 end, void *dest_ptr);

void expand_normal(u16 *screen_ptr, u32 start, u32 end);
void expand_blend(u32 *screen_src_ptr, u16 *screen_dest_ptr, u32 start, u32 end);

typedef struct
{
  tile_render_function normal_render_base;
  tile_render_function normal_render_transparent;
  tile_render_function alpha_render_base;
  tile_render_function alpha_render_transparent;
  tile_render_function color16_render_base;
  tile_render_function color16_render_transparent;
  tile_render_function color32_render_base;
  tile_render_function color32_render_transparent;
} tile_layer_render_struct;

typedef struct
{
  bitmap_render_function normal_render;
} bitmap_layer_render_struct;

#if defined(__cplusplus)
}
#endif

#endif // GBAGPU_H
