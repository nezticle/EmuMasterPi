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

#include "mapper255.h"
#include "disk.h"

static u8 reg[4];

static u8 readLow(u16 addr)
{
	if (addr >= 0x5800)
		return reg[addr & 0x0003] & 0x0F;
	else
		return addr >> 8;
}

static void writeLow(u16 addr, u8 data)
{
	if (addr >= 0x5800)
		reg[addr & 0x0003] = data & 0x0F;
}

static void writeHigh(u16 addr, u8 data)
{
	Q_UNUSED(data)

	u8 prg = (addr & 0x0F80) >> 7;
	int chr = (addr & 0x003F);
	int bank = (addr & 0x4000) >> 14;

	if (addr & 0x2000)
		nesSetMirroring(HorizontalMirroring);
	else
		nesSetMirroring(VerticalMirroring);

	if (addr & 0x1000) {
		if (addr & 0x0040) {
			nesSetRom8KBank(4, 0x80*bank+prg*4+2);
			nesSetRom8KBank(5, 0x80*bank+prg*4+3);
			nesSetRom8KBank(6, 0x80*bank+prg*4+2);
			nesSetRom8KBank(7, 0x80*bank+prg*4+3);
		} else {
			nesSetRom8KBank(4, 0x80*bank+prg*4+0);
			nesSetRom8KBank(5, 0x80*bank+prg*4+1);
			nesSetRom8KBank(6, 0x80*bank+prg*4+0);
			nesSetRom8KBank(7, 0x80*bank+prg*4+1);
		}
	} else {
		nesSetRom8KBank(4, 0x80*bank+prg*4+0);
		nesSetRom8KBank(5, 0x80*bank+prg*4+1);
		nesSetRom8KBank(6, 0x80*bank+prg*4+2);
		nesSetRom8KBank(7, 0x80*bank+prg*4+3);
	}

	nesSetVrom1KBank(0, 0x200*bank+chr*8+0);
	nesSetVrom1KBank(1, 0x200*bank+chr*8+1);
	nesSetVrom1KBank(2, 0x200*bank+chr*8+2);
	nesSetVrom1KBank(3, 0x200*bank+chr*8+3);
	nesSetVrom1KBank(4, 0x200*bank+chr*8+4);
	nesSetVrom1KBank(5, 0x200*bank+chr*8+5);
	nesSetVrom1KBank(6, 0x200*bank+chr*8+6);
	nesSetVrom1KBank(7, 0x200*bank+chr*8+7);
}

void Mapper255::reset()
{
	NesMapper::reset();
	readLow = ::readLow;
	writeLow = ::writeLow;
	writeHigh = ::writeHigh;

	nesSetRom32KBank(0);
	nesSetVrom8KBank(0);
	nesSetMirroring(VerticalMirroring);

	for (int i = 0; i < 4; i++)
		reg[i] = 0;
}

void Mapper255::extSl()
{
	emsl.array("reg", reg, sizeof(reg));
}
