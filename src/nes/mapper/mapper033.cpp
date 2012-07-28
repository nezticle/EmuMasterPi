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

#include "mapper033.h"
#include "disk.h"

static u8 patch;
static u8 reg[7];

static void updateBanks()
{
	nesSetVrom2KBank(0, reg[0]);
	nesSetVrom2KBank(2, reg[1]);

	nesSetVrom1KBank(4, reg[2]);
	nesSetVrom1KBank(5, reg[3]);
	nesSetVrom1KBank(6, reg[4]);
	nesSetVrom1KBank(7, reg[5]);
}

static void writeHigh(u16 addr, u8 data)
{
	switch (addr) {
	case 0x8000:
		if (patch) {
			if (data & 0x40)
				nesSetMirroring(HorizontalMirroring);
			else
				nesSetMirroring(VerticalMirroring);
			nesSetRom8KBank(4, data & 0x1F);
		} else {
			nesSetRom8KBank(4, data);
		}
		break;
	case 0x8001:
		if (patch) {
			nesSetRom8KBank(5, data & 0x1F);
		} else {
			nesSetRom8KBank(5, data);
		}
		break;

	case 0x8002:
		reg[0] = data;
		updateBanks();
		break;
	case 0x8003:
		reg[1] = data;
		updateBanks();
		break;
	case 0xA000:
		reg[2] = data;
		updateBanks();
		break;
	case 0xA001:
		reg[3] = data;
		updateBanks();
		break;
	case 0xA002:
		reg[4] = data;
		updateBanks();
		break;
	case 0xA003:
		reg[5] = data;
		updateBanks();
		break;

	case 0xE001:
	case 0xE002:
	case 0xE003:
		break;
	case 0xE000:
		if (data & 0x40)
			nesSetMirroring(HorizontalMirroring);
		else
			nesSetMirroring(VerticalMirroring);
		break;
	}
}

void Mapper033::reset()
{
	NesMapper::reset();
	writeHigh = ::writeHigh;

	reg[0] = 0;
	reg[1] = 2;
	reg[2] = 4;
	reg[3] = 5;
	reg[4] = 6;
	reg[5] = 7;
	reg[6] = 1;

	nesSetRom8KBanks(0, 1, nesRomSize8KB-2, nesRomSize8KB-1);
	if (nesVromSize8KB)
		updateBanks();

	u32 crc = nesDiskCrc;
	// Check For Old #33 games.... (CRC code by NesToy)
	if (crc == 0x5e9bc161		// Akira(J)
	 || crc == 0xecdbafa4		// Bakushou!! Jinsei Gekijou(J)
	 || crc == 0x59cd0c31		// Don Doko Don(J)
	 || crc == 0x837c1342		// Golf Ko Open(J)
	 || crc == 0x42d893e4		// Operation Wolf(J)
	 || crc == 0x1388aeb9		// Operation Wolf(U)
	 || crc == 0x07ee6d8f		// Power Blazer(J)
	 || crc == 0x5193fb54		// Takeshi no Sengoku Fuuunji(J)
	 || crc == 0xa71c3452) {	// Insector X(J)
		patch = 1;
	}

	nesEmuSetRenderMethod(NesEmu::PreRender);

	if (crc == 0x202df297) {	// Captain Saver(J)
		nesEmuSetRenderMethod(NesEmu::TileRender);
	}
	if (crc == 0x63bb86b5) {	// The Jetsons(J)
		nesEmuSetRenderMethod(NesEmu::TileRender);
	}
}

void Mapper033::extSl()
{
	emsl.array("reg", reg, sizeof(reg));
}
