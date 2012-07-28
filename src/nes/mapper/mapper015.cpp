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

#include "mapper015.h"

static u8 prg[4];
static u8 mirror;

static void updateBanks()
{
	//EMU->SetPRG_RAM8(0x6, 0);
	nesSetRom8KBank(4, prg[0]);
	nesSetRom8KBank(5, prg[1]);
	nesSetRom8KBank(6, prg[2]);
	nesSetRom8KBank(7, prg[3]);

	nesSetCram8KBank(0);
	if (mirror)
		nesSetMirroring(HorizontalMirroring);
	else
		nesSetMirroring(VerticalMirroring);
}

static void writeHigh(u16 addr, u8 data)
{
	u8 prgBank = (data & 0x3f) << 1;
	u8 prgFlip = (data & 0x80) >> 7;

	switch (addr) {
	case 0x8000:
		prgBank &= 0x7c;
		prg[0] = prgBank | (0 ^ prgFlip);
		prg[1] = prgBank | (1 ^ prgFlip);
		prg[2] = prgBank | (2 ^ prgFlip);
		prg[3] = prgBank | (3 ^ prgFlip);
		break;
	case 0x8001:
		prg[0] = prgBank | (0 ^ prgFlip);
		prg[1] = prgBank | (1 ^ prgFlip);
		prg[2] = 0x7e | (0 ^ prgFlip);
		prg[3] = 0x7f | (1 ^ prgFlip);
		break;
	case 0x8002:
		prg[0] = prgBank ^ prgFlip;
		prg[1] = prgBank ^ prgFlip;
		prg[2] = prgBank ^ prgFlip;
		prg[3] = prgBank ^ prgFlip;
		break;
	case 0x8003:
		prg[0] = prgBank | (0 ^ prgFlip);
		prg[1] = prgBank | (1 ^ prgFlip);
		prg[2] = prgBank | (0 ^ prgFlip);
		prg[3] = prgBank | (1 ^ prgFlip);
		break;
	}
	updateBanks();
}

void Mapper015::reset()
{
	NesMapper::reset();
	writeHigh = ::writeHigh;

	prg[0]=0;
	prg[1]=1;
	prg[2]=2;
	prg[3]=3;
	mirror = 0;

	nesSetRom32KBank(0);
}
