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

#include "mapper009.h"
#include "disk.h"

static u8 reg[4];
static u8 latch_a, latch_b;

static void writeHigh(u16 addr, u8 data)
{
	switch (addr & 0xf000) {
	case 0xa000:
		nesSetRom8KBank(4, data);
		break;
	case 0xb000:
		reg[0] = data;
		if (latch_a == 0xfd) {
			nesSetVrom4KBank(0, reg[0]);
		}
		break;
	case 0xc000:
		reg[1] = data;
		if (latch_a == 0xfe) {
			nesSetVrom4KBank(0, reg[1]);
		}
		break;
	case 0xd000:
		reg[2] = data;
		if (latch_b == 0xfd) {
			nesSetVrom4KBank(4, reg[2]);
		}
		break;
	case 0xe000:
		reg[3] = data;
		if (latch_b == 0xfe) {
			nesSetVrom4KBank(4, reg[3]);
		}
		break;
	case 0xf000:
		if (data & 0x01)
			nesSetMirroring(HorizontalMirroring);
		else
			nesSetMirroring(VerticalMirroring);
		break;
	}
}

static void characterLatch(u16 addr)
{
	if ((addr&0x1ff0) == 0x0fd0 && latch_a != 0xfd) {
		latch_a = 0xfd;
		nesSetVrom4KBank(0, reg[0]);
	} else if ((addr&0x1ff0) == 0x0fe0 && latch_a != 0xfe) {
		latch_a = 0xfe;
		nesSetVrom4KBank(0, reg[1]);
	} else if ((addr&0x1ff0) == 0x1fd0 && latch_b != 0xfd) {
		latch_b = 0xfd;
		nesSetVrom4KBank(4, reg[2]);
	} else if ((addr&0x1ff0) == 0x1fe0 && latch_b != 0xfe) {
		latch_b = 0xfe;
		nesSetVrom4KBank(4, reg[3]);
	}
}

void Mapper009::reset()
{
	NesMapper::reset();
	writeHigh = ::writeHigh;
	characterLatch = ::characterLatch;

	nesSetRom8KBanks(0, nesRomSize8KB-3, nesRomSize8KB-2, nesRomSize8KB-1);

	latch_a = 0xfe;
	latch_b = 0xfe;

	reg[0] = 0; reg[1] = 4;
	reg[2] = 0; reg[3] = 0;

	nesSetVrom4KBank(0, 4);
	nesSetVrom4KBank(4, 0);
}

void Mapper009::extSl()
{
	emsl.array("reg", reg, sizeof(reg));
	emsl.var("latch_a", latch_a);
	emsl.var("latch_b", latch_b);
}
