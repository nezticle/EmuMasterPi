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

#include "mapper251.h"
#include "disk.h"
#include "ppu.h"

static u8 reg[11];
static u8 breg[4];

static void setBank()
{
	int chr[6];
	int prg[4];

	for (int i = 0; i < 6; i++)
		chr[i] = (reg[i]|(breg[1]<<4)) & ((breg[2]<<4)|0x0F);

	if (reg[8] & 0x80) {
		nesSetVrom1KBank(4, chr[0]);
		nesSetVrom1KBank(5, chr[0]+1);
		nesSetVrom1KBank(6, chr[1]);
		nesSetVrom1KBank(7, chr[1]+1);
		nesSetVrom1KBank(0, chr[2]);
		nesSetVrom1KBank(1, chr[3]);
		nesSetVrom1KBank(2, chr[4]);
		nesSetVrom1KBank(3, chr[5]);
	} else {
		nesSetVrom1KBank(0, chr[0]);
		nesSetVrom1KBank(1, chr[0]+1);
		nesSetVrom1KBank(2, chr[1]);
		nesSetVrom1KBank(3, chr[1]+1);
		nesSetVrom1KBank(4, chr[2]);
		nesSetVrom1KBank(5, chr[3]);
		nesSetVrom1KBank(6, chr[4]);
		nesSetVrom1KBank(7, chr[5]);
	}

	prg[0] = (reg[6]&((breg[3]&0x3F)^0x3F))|(breg[1]);
	prg[1] = (reg[7]&((breg[3]&0x3F)^0x3F))|(breg[1]);
	prg[2] = prg[3] =((breg[3]&0x3F)^0x3F)|(breg[1]);
	prg[2] &= nesRomSize8KB-1;

	if (reg[8] & 0x40)
		nesSetRom8KBanks(prg[2],prg[1],prg[0],prg[3]);
	else
		nesSetRom8KBanks(prg[0],prg[1],prg[2],prg[3]);
}

static void writeLow(u16 addr, u8 data)
{
	Q_UNUSED(addr)
	if ((addr & 0xE001) == 0x6000) {
		if (reg[9]) {
			breg[reg[10]++] = data;
			if (reg[10] == 4) {
				reg[10] = 0;
				setBank();
			}
		}
	}
}

static void writeHigh(u16 addr, u8 data)
{
	switch (addr & 0xE001) {
	case 0x8000:
		reg[8] = data;
		setBank();
		break;
	case 0x8001:
		reg[reg[8]&0x07] = data;
		setBank();
		break;
	case 0xA001:
		if (data & 0x80) {
			reg[ 9] = 1;
			reg[10] = 0;
		} else {
			reg[ 9] = 0;
		}
		break;
	}
}

void Mapper251::reset()
{
	NesMapper::reset();
	writeLow = ::writeLow;
	writeHigh = ::writeHigh;

	nesSetRom8KBanks(0, 1, nesRomSize8KB-2, nesRomSize8KB-1);
	nesSetMirroring(VerticalMirroring);

	memset(reg, 0, sizeof(reg));
	memset(breg, 0, sizeof(breg));
}

void Mapper251::extSl()
{
	emsl.array("reg", reg, sizeof(reg));
	emsl.array("breg", breg, sizeof(breg));
}
