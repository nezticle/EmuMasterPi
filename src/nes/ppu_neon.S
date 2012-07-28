/*
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

.text
.align 4
.global ppuTileBlitNeon

#define load32(register, value) \
	movw register, :lower16:value; \
	movt register, :upper16:value;

@ r0 - destination - place for pixels of tile line that is generated
@ r1 - pen set used to color a line of tile
@ r2 - c1
@ r3 - c2
ppuTileBlitNeon:
	@ load pens
	vldm r1, {q3}
	@ combine c1 and c2
	orr r2, r2, r3, lsl #8
	vdup.16 d0, r2
	load32(r2, 0x00fefcfa)
	vdup.32 q1, r2
	vzip.8 d2, d3
	vshl.u8 d0, d0, d2
	mov r2, #3
	vdup.8 d1, r2
	vand d0, d0, d1
	vshl.i8 d0, d0, #2
	vmov d1, d0
	vzip.8 d0, d1
	vmov q1, q0
	vzip.16 q0, q1
	load32(r1, 0x03020100)
	vdup.32 q2, r1
	vorr q0, q0, q2
	vorr q1, q1, q2
	@ move pens to pixel data using created indices
	vtbl.8 d0, {q3}, d0
	vtbl.8 d1, {q3}, d1
	vtbl.8 d2, {q3}, d2
	vtbl.8 d3, {q3}, d3
	@ store pixels
	vstm r0, {q0,q1}
	mov pc, lr
