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

#include "mapper243.h"
#include "disk.h"

static u8 reg[4];

static void writeLow(u16 addr, u8 data)
{
	if ((addr&0x4101) == 0x4100) {
		reg[0] = data;
	} else if ((addr&0x4101) == 0x4101) {
		switch (reg[0] & 0x07) {
		case 0:
			reg[1] = 0;
			reg[2] = 3;
			break;
		case 4:
			reg[2] = (reg[2]&0x06)|(data&0x01);
			break;
		case 5:
			reg[1] = data&0x01;
			break;
		case 6:
			reg[2] = (reg[2]&0x01)|((data&0x03)<<1);
			break;
		case 7:
			reg[3] = data&0x01;
			break;
		default:
			break;
		}

		nesSetRom32KBank(reg[1]);
		nesSetVrom8KBank(reg[2]);

		if (reg[3])
			nesSetMirroring(VerticalMirroring);
		else
			nesSetMirroring(HorizontalMirroring);
	}
}

void Mapper243::reset()
{
	NesMapper::reset();
	writeLow = ::writeLow;

	nesSetRom32KBank(0);

	if (nesVromSize8KB > 4)
		nesSetVrom8KBank(4);
	else
		nesSetVrom8KBank(0);

	nesSetMirroring(VerticalMirroring);

	for (int i = 0; i < 4; i++)
		reg[i] = 0;
}

void Mapper243::extSl()
{
	emsl.array("reg", reg, sizeof(reg));
}
