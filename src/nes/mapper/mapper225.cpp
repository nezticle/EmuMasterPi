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

#include "mapper225.h"

static void writeHigh(u16 addr, u8 data)
{
	Q_UNUSED(data)

	u8 prg_bank = (addr & 0x0F80) >> 7;
	u8 chr_bank = addr & 0x003F;

	nesSetVrom1KBank(0, chr_bank*8+0);
	nesSetVrom1KBank(1, chr_bank*8+1);
	nesSetVrom1KBank(2, chr_bank*8+2);
	nesSetVrom1KBank(3, chr_bank*8+3);
	nesSetVrom1KBank(4, chr_bank*8+4);
	nesSetVrom1KBank(5, chr_bank*8+5);
	nesSetVrom1KBank(6, chr_bank*8+6);
	nesSetVrom1KBank(7, chr_bank*8+7);

	if (addr & 0x2000)
		nesSetMirroring(HorizontalMirroring);
	else
		nesSetMirroring(VerticalMirroring);

	if (addr & 0x1000) {
		if (addr & 0x0040) {
			nesSetRom8KBank(4, prg_bank*4+2);
			nesSetRom8KBank(5, prg_bank*4+3);
			nesSetRom8KBank(6, prg_bank*4+2);
			nesSetRom8KBank(7, prg_bank*4+3);
		} else {
			nesSetRom8KBank(4, prg_bank*4+0);
			nesSetRom8KBank(5, prg_bank*4+1);
			nesSetRom8KBank(6, prg_bank*4+0);
			nesSetRom8KBank(7, prg_bank*4+1);
		}
	} else {
		nesSetRom8KBank(4, prg_bank*4+0);
		nesSetRom8KBank(5, prg_bank*4+1);
		nesSetRom8KBank(6, prg_bank*4+2);
		nesSetRom8KBank(7, prg_bank*4+3);
	}
}

void Mapper225::reset()
{
	NesMapper::reset();
	writeHigh = ::writeHigh;

	nesSetRom32KBank(0);
	if (nesVromSize1KB)
		nesSetVrom8KBank(0);
}
