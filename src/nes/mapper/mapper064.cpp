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

#include "mapper064.h"
#include "ppu.h"

static u8 reg[3];
static u8 irq_enable;
static u8 irq_mode;
static s32 irq_counter;
static s32 irq_counter2;
static u8 irq_latch;
static u8 irq_reset;

static void writeHigh(u16 addr, u8 data)
{
	switch (addr & 0xF003) {
	case 0x8000:
		reg[0] = data&0x0F;
		reg[1] = data&0x40;
		reg[2] = data&0x80;
		break;

	case 0x8001:
		switch (reg[0]) {
		case 0x00:
			if (reg[2]) {
				nesSetVrom1KBank(4, data+0);
				nesSetVrom1KBank(5, data+1);
			} else {
				nesSetVrom1KBank(0, data+0);
				nesSetVrom1KBank(1, data+1);
			}
			break;
		case 0x01:
			if (reg[2]) {
				nesSetVrom1KBank(6, data+0);
				nesSetVrom1KBank(7, data+1);
			} else {
				nesSetVrom1KBank(2, data+0);
				nesSetVrom1KBank(3, data+1);
			}
			break;
		case 0x02:
			if (reg[2]) {
				nesSetVrom1KBank(0, data);
			} else {
				nesSetVrom1KBank(4, data);
			}
			break;
		case 0x03:
			if (reg[2]) {
				nesSetVrom1KBank(1, data);
			} else {
				nesSetVrom1KBank(5, data);
			}
			break;
		case 0x04:
			if (reg[2]) {
				nesSetVrom1KBank(2, data);
			} else {
				nesSetVrom1KBank(6, data);
			}
			break;
		case 0x05:
			if (reg[2]) {
				nesSetVrom1KBank(3, data);
			} else {
				nesSetVrom1KBank(7, data);
			}
			break;
		case 0x06:
			if (reg[1]) {
				nesSetRom8KBank(5, data);
			} else {
				nesSetRom8KBank(4, data);
			}
			break;
		case 0x07:
			if (reg[1]) {
				nesSetRom8KBank(6, data);
			} else {
				nesSetRom8KBank(5, data);
			}
			break;
		case 0x08:
			nesSetVrom1KBank(1, data);
			break;
		case 0x09:
			nesSetVrom1KBank(3, data);
			break;
		case 0x0F:
			if (reg[1]) {
				nesSetRom8KBank(4, data);
			} else {
				nesSetRom8KBank(6, data);
			}
			break;
		}
		break;

	case 0xA000:
		nesSetMirroring(static_cast<NesMirroring>(data & 0x01));
		break;

	case 0xC000:
		irq_latch = data;
		if (irq_reset) {
			irq_counter = irq_latch;
		}
		break;
	case 0xC001:
		irq_reset = 0xFF;
		irq_counter = irq_latch;
		irq_mode = data & 0x01;
		break;
	case 0xE000:
		irq_enable = 0;
		if (irq_reset) {
			irq_counter = irq_latch;
		}
		nesMapperSetIrqSignalOut(false);
		break;
	case 0xE001:
		irq_enable = 0xFF;
		if (irq_reset) {
			irq_counter = irq_latch;
		}
		break;
	}
}

static void clock(int cycles)
{
	if (!irq_mode)
		return;

	irq_counter2 += cycles;
	while( irq_counter2 >= 4) {
		irq_counter2 -= 4;
		if (irq_counter >= 0) {
			irq_counter--;
			if (irq_counter < 0) {
				if (irq_enable)
					nesMapperSetIrqSignalOut(true);
			}
		}
	}
}

static void horizontalSync()
{
	if (irq_mode)
		return;

	irq_reset = 0;

	if (nesPpuScanline < NesPpu::VisibleScreenHeight && nesPpuIsDisplayOn()) {
		if (irq_counter >= 0) {
			irq_counter--;
			if (irq_counter < 0) {
				if (irq_enable) {
					irq_reset = 1;
					nesMapperSetIrqSignalOut(true);
				}
			}
		}
	}
}

void Mapper064::reset()
{
	NesMapper::reset();
	writeHigh = ::writeHigh;
	horizontalSync = ::horizontalSync;
	clock = ::clock;

	nesSetRom8KBanks(nesRomSize8KB-1, nesRomSize8KB-1, nesRomSize8KB-1, nesRomSize8KB-1);

	if (nesVromSize1KB)
		nesSetVrom8KBank(0);

	reg[0] = reg[1] = reg[2] = 0;

	irq_enable = 0;
	irq_mode = 0;
	irq_counter = 0;
	irq_counter2 = 0;
	irq_latch = 0;
	irq_reset = 0;
}

void Mapper064::extSl()
{
	emsl.array("reg", reg, sizeof(reg));
	emsl.var("irq_enable", irq_enable);
	emsl.var("irq_mode", irq_mode);
	emsl.var("irq_counter", irq_counter);
	emsl.var("irq_counter2", irq_counter2);
	emsl.var("irq_latch", irq_latch);
	emsl.var("irq_reset", irq_reset);
}
