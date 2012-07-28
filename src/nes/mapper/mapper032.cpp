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

#include "mapper032.h"
#include "disk.h"

static u8 patch;
static u8 reg;

static void writeHigh(u16 addr, u8 data)
{
	switch (addr & 0xF000) {
	case 0x8000:
		if (reg & 0x02)
			nesSetRom8KBank(6, data);
		else
			nesSetRom8KBank(4, data);
		break;
	case 0x9000:
		reg = data;
		nesSetMirroring(static_cast<NesMirroring>(data & 0x01));
		break;
	case 0xA000:
		nesSetRom8KBank(5, data);
		break;
	}
	switch (addr & 0xF007) {
	case 0xB000:
	case 0xB001:
	case 0xB002:
	case 0xB003:
	case 0xB004:
	case 0xB005:
		nesSetVrom1KBank(addr & 0x0007, data);
		break;
	case 0xB006:
		nesSetVrom1KBank(6, data);
		if (patch && (data & 0x40))
			nesSetMirroring(0, 0, 0, 1);
		break;
	case 0xB007:
		nesSetVrom1KBank(7, data);
		if (patch && (data & 0x40))
			nesSetMirroring(SingleLow);
		break;
	}
}

void Mapper032::reset()
{
	NesMapper::reset();
	writeHigh = ::writeHigh;

	patch = 0;
	reg = 0;

	nesSetRom8KBanks(0, 1, nesRomSize8KB-2, nesRomSize8KB-1);
	if (nesVromSize1KB)
		nesSetVrom8KBank(0);

	u32 crc = nesDiskCrc;
	// For Major League(J)
	if (crc == 0xc0fed437) {
		patch = 1;
	}
	// For Ai Sensei no Oshiete - Watashi no Hoshi(J)
	if (crc == 0xfd3fc292) {
		nesSetRom8KBanks(30, 31, 30, 31);
	}
}

void Mapper032::extSl()
{
	emsl.var("reg", reg);
}
