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

#include "mapper024.h"
#include "disk.h"

static u8 irq_enable;
static u8 irq_counter;
static u8 irq_latch;
static int irq_clock;

static void writeHigh(u16 addr, u8 data)
{
	switch (addr & 0xF003) {
	case 0x8000:
		nesSetRom16KBank(4, data);
		break;

	case 0x9000: case 0x9001: case 0x9002:
	case 0xA000: case 0xA001: case 0xA002:
	case 0xB000: case 0xB001: case 0xB002:
		// TODO nes->apu->ExWrite( addr, data);
		break;

	case 0xB003:
		nesSetMirroring(static_cast<NesMirroring>((data >> 3) & 0x03));
		break;

	case 0xC000:
		nesSetRom8KBank(6, data);
		break;

	case 0xD000:
		nesSetVrom1KBank(0, data);
		break;

	case 0xD001:
		nesSetVrom1KBank(1, data);
		break;

	case 0xD002:
		nesSetVrom1KBank(2, data);
		break;

	case 0xD003:
		nesSetVrom1KBank(3, data);
		break;

	case 0xE000:
		nesSetVrom1KBank(4, data);
		break;

	case 0xE001:
		nesSetVrom1KBank(5, data);
		break;

	case 0xE002:
		nesSetVrom1KBank(6, data);
		break;

	case 0xE003:
		nesSetVrom1KBank(7, data);
		break;

	case 0xF000:
		irq_latch = data;
		break;
	case 0xF001:
		irq_enable = data & 0x03;
		if (irq_enable & 0x02) {
			irq_counter = irq_latch;
			irq_clock = 0;
		}
		nesMapperSetIrqSignalOut(false);
		break;
	case 0xF002:
		irq_enable = (irq_enable & 0x01) * 3;
		nesMapperSetIrqSignalOut(false);
		break;
	}
}

static void clock(int cycles)
{
	if (irq_enable & 0x02) {
		if ((irq_clock+=cycles) >= 0x72) {
			irq_clock -= 0x72;
			if (irq_counter == 0xFF) {
				irq_counter = irq_latch;
				nesMapperSetIrqSignalOut(true);
			} else {
				irq_counter++;
			}
		}
	}
}

void Mapper024::reset()
{
	NesMapper::reset();
	writeHigh = ::writeHigh;
	clock = ::clock;

	irq_enable = 0;
	irq_counter = 0;
	irq_latch = 0;
	irq_clock = 0;

	nesSetRom8KBanks(0, 1, nesRomSize8KB-2, nesRomSize8KB-1);
	if (nesVromSize1KB)
		nesSetVrom8KBank(0);

	nesEmuSetRenderMethod(NesEmu::PostRender);
	// TODO nes->apu->SelectExSound( 1);
}

void Mapper024::extSl()
{
	emsl.var("irq_enable", irq_enable);
	emsl.var("irq_counter", irq_counter);
	emsl.var("irq_latch", irq_latch);
	emsl.var("irq_clock", irq_clock);
}
