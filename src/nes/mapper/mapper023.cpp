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

#include "mapper023.h"
#include "disk.h"

static u16 addrmask;
static u8 reg[9];
static u8 irq_enable;
static u8 irq_counter;
static u8 irq_latch;
static int irq_clock;

static void writeHigh(u16 addr, u8 data)
{
	switch (addr & addrmask) {
	case 0x8000:
	case 0x8004:
	case 0x8008:
	case 0x800C:
		if (reg[8]) {
			nesSetRom8KBank(6, data);
		} else {
			nesSetRom8KBank(4, data);
		}
		break;

	case 0x9000:
		if (data != 0xFF)
			nesSetMirroring(static_cast<NesMirroring>(data & 3));
		break;

	case 0x9008:
		reg[8] = data & 0x02;
		break;

	case 0xA000:
	case 0xA004:
	case 0xA008:
	case 0xA00C:
		nesSetRom8KBank(5, data);
		break;

	case 0xB000:
		reg[0] = (reg[0] & 0xF0) | (data & 0x0F);
		nesSetVrom1KBank(0, reg[0]);
		break;
	case 0xB001:
	case 0xB004:
		reg[0] = (reg[0] & 0x0F) | ((data & 0x0F) << 4);
		nesSetVrom1KBank(0, reg[0]);
		break;

	case 0xB002:
	case 0xB008:
		reg[1] = (reg[1] & 0xF0) | (data & 0x0F);
		nesSetVrom1KBank(1, reg[1]);
		break;

	case 0xB003:
	case 0xB00C:
		reg[1] = (reg[1] & 0x0F) | ((data & 0x0F) << 4);
		nesSetVrom1KBank(1, reg[1]);
		break;

	case 0xC000:
		reg[2] = (reg[2] & 0xF0) | (data & 0x0F);
		nesSetVrom1KBank(2, reg[2]);
		break;

	case 0xC001:
	case 0xC004:
		reg[2] = (reg[2] & 0x0F) | ((data & 0x0F) << 4);
		nesSetVrom1KBank(2, reg[2]);
		break;

	case 0xC002:
	case 0xC008:
		reg[3] = (reg[3] & 0xF0) | (data & 0x0F);
		nesSetVrom1KBank(3, reg[3]);
		break;

	case 0xC003:
	case 0xC00C:
		reg[3] = (reg[3] & 0x0F) | ((data & 0x0F) << 4);
		nesSetVrom1KBank(3, reg[3]);
		break;

	case 0xD000:
		reg[4] = (reg[4] & 0xF0) | (data & 0x0F);
		nesSetVrom1KBank(4, reg[4]);
		break;

	case 0xD001:
	case 0xD004:
		reg[4] = (reg[4] & 0x0F) | ((data & 0x0F) << 4);
		nesSetVrom1KBank(4, reg[4]);
		break;

	case 0xD002:
	case 0xD008:
		reg[5] = (reg[5] & 0xF0) | (data & 0x0F);
		nesSetVrom1KBank(5, reg[5]);
		break;

	case 0xD003:
	case 0xD00C:
		reg[5] = (reg[5] & 0x0F) | ((data & 0x0F) << 4);
		nesSetVrom1KBank(5, reg[5]);
		break;

	case 0xE000:
		reg[6] = (reg[6] & 0xF0) | (data & 0x0F);
		nesSetVrom1KBank(6, reg[6]);
		break;

	case 0xE001:
	case 0xE004:
		reg[6] = (reg[6] & 0x0F) | ((data & 0x0F) << 4);
		nesSetVrom1KBank(6, reg[6]);
		break;

	case 0xE002:
	case 0xE008:
		reg[7] = (reg[7] & 0xF0) | (data & 0x0F);
		nesSetVrom1KBank(7, reg[7]);
		break;

	case 0xE003:
	case 0xE00C:
		reg[7] = (reg[7] & 0x0F) | ((data & 0x0F) << 4);
		nesSetVrom1KBank(7, reg[7]);
		break;

	case 0xF000:
		irq_latch = (irq_latch & 0xF0) | (data & 0x0F);
		nesMapperSetIrqSignalOut(false);
		break;
	case 0xF004:
		irq_latch = (irq_latch & 0x0F) | ((data & 0x0F) << 4);
		nesMapperSetIrqSignalOut(false);
		break;

	case 0xF008:
		irq_enable = data & 0x03;
		irq_counter = irq_latch;
		irq_clock = 0;
		nesMapperSetIrqSignalOut(false);
		break;

	case 0xF00C:
		irq_enable = (irq_enable & 0x01) * 3;
		nesMapperSetIrqSignalOut(false);
		break;
	}
}

static void clock(int cycles)
{
	if (irq_enable & 0x02) {
		irq_clock += cycles*3;
		while (irq_clock >= 341) {
			irq_clock -= 341;
			irq_counter++;
			if (irq_counter == 0) {
				irq_counter = irq_latch;
				nesMapperSetIrqSignalOut(true);
			}
		}
	}
}

void Mapper023::reset()
{
	NesMapper::reset();
	writeHigh = ::writeHigh;
	clock = ::clock;

	addrmask = 0xffff;

	for (int i = 0; i < 8; i++)
		reg[i] = i;
	reg[8] = 0;

	irq_enable = 0;
	irq_counter = 0;
	irq_latch = 0;
	irq_clock = 0;

	nesSetRom8KBanks(0, 1, nesRomSize8KB-2, nesRomSize8KB-1);
	nesSetVrom8KBank(0);

	u32 crc = nesDiskCrc;

	if (crc == 0x93794634		// Akumajou Special Boku Dracula Kun(J)
	 || crc == 0xc7829dae		// Akumajou Special Boku Dracula Kun(T-Eng)
	 || crc == 0xf82dc02f) {	// Akumajou Special Boku Dracula Kun(T-Eng v1.02)
		addrmask = 0xf00c;
		nesEmuSetRenderMethod(NesEmu::PreAllRender);
	}
	if (crc == 0xdd53c4ae) {	// Tiny Toon Adventures(J)
		nesEmuSetRenderMethod(NesEmu::PostAllRender);
	}
}

void Mapper023::extSl()
{
	emsl.array("reg", reg, sizeof(reg));
	emsl.var("irq_enable", irq_enable);
	emsl.var("irq_counter", irq_counter);
	emsl.var("irq_latch", irq_latch);
	emsl.var("irq_clock", irq_clock);
}
