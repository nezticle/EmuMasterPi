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

#include "mapper235.h"

static void writeHigh(u16 addr, u8 data)
{
	Q_UNUSED(data)

	u8 prg = ((addr&0x0300)>>3) | (addr&0x001F);
	u8 bus = 0;

	if (nesRomSize8KB == 64*2) {
		switch (addr & 0x0300) {
		case 0x0000: break;
		case 0x0100: bus = 1; break;
		case 0x0200: bus = 1; break;
		case 0x0300: bus = 1; break;
		}
	} else if (nesRomSize8KB == 128*2) {
		switch (addr & 0x0300) {
		case 0x0000: break;
		case 0x0100: bus = 1; break;
		case 0x0200: prg = (prg&0x1F)|0x20; break;
		case 0x0300: bus = 1; break;
		}
	} else if (nesRomSize8KB == 192*2) {
		switch (addr & 0x0300) {
		case 0x0000: break;
		case 0x0100: bus = 1; break;
		case 0x0200: prg = (prg&0x1F)|0x20; break;
		case 0x0300: prg = (prg&0x1F)|0x40; break;
		}
	}

	if (addr & 0x0800) {
		if (addr & 0x1000) {
			nesSetRom8KBank(4, prg*4+2);
			nesSetRom8KBank(5, prg*4+3);
			nesSetRom8KBank(6, prg*4+2);
			nesSetRom8KBank(7, prg*4+3);
		} else {
			nesSetRom8KBank(4, prg*4+0);
			nesSetRom8KBank(5, prg*4+1);
			nesSetRom8KBank(6, prg*4+0);
			nesSetRom8KBank(7, prg*4+1);
		}
	} else {
		nesSetRom32KBank(prg);
	}

	if (bus) {
		for (int i = 4; i < 8; i++)
			nesSetWram8KBank(i, 1);
	}

	if (addr & 0x0400)
		nesSetMirroring(SingleLow);
	else if (addr & 0x2000)
		nesSetMirroring(HorizontalMirroring);
	else
		nesSetMirroring(VerticalMirroring);
}

void Mapper235::reset()
{
	NesMapper::reset();
	writeHigh = ::writeHigh;

	memset(nesWram+0x2000, 0xFF, 0x2000);
	nesSetRom32KBank(0);
}
