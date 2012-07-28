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

#include "mapper226.h"

static u8 reg[2];

static void writeHigh(u16 addr, u8 data)
{
	if (addr & 0x001 )
		reg[1] = data;
	else
		reg[0] = data;

	if (reg[0] & 0x40)
		nesSetMirroring(VerticalMirroring);
	else
		nesSetMirroring(HorizontalMirroring);

	u8 bank = ((reg[0]&0x1E)>>1)|((reg[0]&0x80)>>3)|((reg[1]&0x01)<<5);

	if (reg[0] & 0x20) {
		if (reg[0] & 0x01) {
			nesSetRom8KBank(4, bank*4+2 );
			nesSetRom8KBank(5, bank*4+3 );
			nesSetRom8KBank(6, bank*4+2 );
			nesSetRom8KBank(7, bank*4+3 );
		} else {
			nesSetRom8KBank(4, bank*4+0 );
			nesSetRom8KBank(5, bank*4+1 );
			nesSetRom8KBank(6, bank*4+0 );
			nesSetRom8KBank(7, bank*4+1 );
		}
	} else {
		nesSetRom8KBank(4, bank*4+0 );
		nesSetRom8KBank(5, bank*4+1 );
		nesSetRom8KBank(6, bank*4+2 );
		nesSetRom8KBank(7, bank*4+3 );
	}
}

void Mapper226::reset()
{
	NesMapper::reset();
	writeHigh = ::writeHigh;

	nesSetRom32KBank(0);

	reg[0] = 0;
	reg[1] = 0;
}

void Mapper226::extSl()
{
	emsl.array("reg", reg, sizeof(reg));
}
