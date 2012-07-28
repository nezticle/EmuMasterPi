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

#include "mapper048.h"
#include "ppu.h"

static u8 reg;
static u8 irq_enable;
static u8 irq_counter;
static u8 irq_latch;

static void writeHigh(u16 addr, u8 data)
{
	switch (addr) {
	case 0x8000:
		if (!reg)
			nesSetMirroring(static_cast<NesMirroring>((data & 0x40) >> 6));
		nesSetRom8KBank(4, data);
		break;
	case 0x8001:
		nesSetRom8KBank(5, data);
		break;

	case 0x8002:
		nesSetVrom2KBank(0, data);
		break;
	case 0x8003:
		nesSetVrom2KBank(2, data);
		break;
	case 0xA000:
		nesSetVrom1KBank(4, data);
		break;
	case 0xA001:
		nesSetVrom1KBank(5, data);
		break;
	case 0xA002:
		nesSetVrom1KBank(6, data);
		break;
	case 0xA003:
		nesSetVrom1KBank(7, data);
		break;

	case 0xC000:
		irq_latch = data;
		break;

	case 0xC001:
		irq_counter = irq_latch;
		break;

	case 0xC002:
		irq_enable = 1;
		break;
	case 0xC003:
		irq_enable = 0;
		nesMapperSetIrqSignalOut(false);
		break;

	case 0xE000:
		nesSetMirroring(static_cast<NesMirroring>((data & 0x40) >> 6));
		reg = 1;
		break;
	}
}

static void horizontalSync()
{
	if (nesPpuScanline < NesPpu::VisibleScreenHeight && nesPpuIsDisplayOn()) {
		if (irq_enable) {
			if (irq_counter == 0xFF)
				nesMapperSetIrqSignalOut(true);
			irq_counter++;
		}
	}
}

void Mapper048::reset()
{
	NesMapper::reset();
	writeHigh = ::writeHigh;
	horizontalSync = ::horizontalSync;

	nesSetRom8KBanks(0, 1, nesRomSize8KB-2, nesRomSize8KB-1);
	if (nesVromSize1KB)
		nesSetVrom8KBank(0);

	reg = 0;
	irq_enable = 0;
	irq_counter = 0;
	irq_latch = 0;
}

void Mapper048::extSl()
{
	emsl.var("reg", reg);
	emsl.var("irq_enable", irq_enable);
	emsl.var("irq_counter", irq_counter);
}
