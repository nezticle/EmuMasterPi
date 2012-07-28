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

#include "mapper067.h"
#include "disk.h"

static u8 irq_enable;
static u8 irq_toggle;
static s32 irq_counter;

static void writeHigh(u16 addr, u8 data)
{
	switch (addr & 0xF800) {
	case 0x8800:
		nesSetVrom2KBank(0, data);
		break;
	case 0x9800:
		nesSetVrom2KBank(2, data);
		break;
	case 0xA800:
		nesSetVrom2KBank(4, data);
		break;
	case 0xB800:
		nesSetVrom2KBank(6, data);
		break;

	case 0xC800:
		if (!irq_toggle) {
			irq_counter = (irq_counter&0x00FF) | (data<<8);
		} else {
			irq_counter = (irq_counter&0xFF00) | data;
		}
		irq_toggle ^= 1;
		nesMapperSetIrqSignalOut(false);
		break;
	case 0xD800:
		irq_enable = data & 0x10;
		irq_toggle = 0;
		nesMapperSetIrqSignalOut(false);
		break;

	case 0xE800:
		nesSetMirroring(static_cast<NesMirroring>(data & 0x03));
		break;

	case 0xF800:
		nesSetRom16KBank(4, data);
		break;
	}
}

static void clock(int cycles)
{
	if (irq_enable) {
		if ((irq_counter -= cycles) <= 0) {
			irq_enable = 0;
			irq_counter = 0xFFFF;
			nesMapperSetIrqSignalOut(true);
		}
	}
}

void Mapper067::reset()
{
	NesMapper::reset();
	writeHigh = ::writeHigh;
	clock = ::clock;

	irq_enable = 0;
	irq_toggle = 0;
	irq_counter = 0;

	nesSetRom8KBanks(0, 1, nesRomSize8KB-2, nesRomSize8KB-1);

	nesSetVrom4KBank(0, 0);
	nesSetVrom4KBank(4, nesVromSize4KB-1);

	u32 crc = nesDiskCrc;

	if (crc == 0x7f2a04bf) // For Fantasy Zone 2(J)
		nesEmuSetRenderMethod(NesEmu::PreAllRender);
}

void Mapper067::extSl()
{
	emsl.var("irq_enable", irq_enable);
	emsl.var("irq_counter", irq_counter);
	emsl.var("irq_toggle", irq_toggle);
}
