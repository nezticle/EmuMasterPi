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

#include "mapper065.h"
#include "disk.h"

static u8 patch;
static u8 irq_enable;
static s32 irq_counter;
static s32 irq_latch;

static void writeHigh(u16 addr, u8 data)
{
	switch (addr) {
	case 0x8000:
		nesSetRom8KBank(4, data);
		break;

	case 0x9000:
		if (!patch)
			nesSetMirroring(static_cast<NesMirroring>(((data & 0x40)^0x40) >> 6));
		break;

	case 0x9001:
		if (patch)
			nesSetMirroring(static_cast<NesMirroring>((data & 0x80) >> 7));
		break;

	case 0x9003:
		if (!patch) {
			irq_enable = data & 0x80;
			nesMapperSetIrqSignalOut(false);
		}
		break;
	case 0x9004:
		if (!patch) {
			irq_counter = irq_latch;
		}
		break;
	case 0x9005:
		if (patch) {
			irq_counter = (u8)(data<<1);
			irq_enable = data;
			nesMapperSetIrqSignalOut(false);
		} else {
			irq_latch = (irq_latch & 0x00FF)|(data<<8);
		}
		break;

	case 0x9006:
		if (patch) {
			irq_enable = 1;
		} else {
			irq_latch = (irq_latch & 0xFF00)|data;
		}
		break;

	case 0xB000:
	case 0xB001:
	case 0xB002:
	case 0xB003:
	case 0xB004:
	case 0xB005:
	case 0xB006:
	case 0xB007:
		nesSetVrom1KBank(addr & 0x0007, data);
		break;

	case 0xA000:
		nesSetRom8KBank(5, data);
		break;
	case 0xC000:
		nesSetRom8KBank(6, data);
		break;
	}
}

static void clock(int cycles)
{
	Q_ASSERT(!patch);
	if (irq_enable) {
		if (irq_counter <= 0) {
			nesMapperSetIrqSignalOut(true);
		} else {
			irq_counter -= cycles;
		}
	}
}

static void horizontalSync()
{
	Q_ASSERT(patch);
	if (irq_enable) {
		if (irq_counter == 0) {
			nesMapperSetIrqSignalOut(true);
		} else {
			irq_counter--;
		}
	}
}

void Mapper065::reset()
{
	NesMapper::reset();
	writeHigh = ::writeHigh;

	patch = 0;

	// Kaiketsu Yanchamaru 3(J)
	if (nesDiskCrc == 0xe30b7f64)
		patch = 1;

	nesSetRom8KBanks(0, 1, nesRomSize8KB-2, nesRomSize8KB-1);

	if (nesVromSize1KB)
		nesSetVrom8KBank(0);

	irq_enable = 0;
	irq_counter = 0;
	irq_latch = 0;

	if (patch)
		horizontalSync = ::horizontalSync;
	else
		clock = ::clock;
}

void Mapper065::extSl()
{
	emsl.var("irq_enable", irq_enable);
	emsl.var("irq_counter", irq_counter);
	emsl.var("irq_latch", irq_latch);
}
