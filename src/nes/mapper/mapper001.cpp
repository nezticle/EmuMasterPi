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

#include "mapper001.h"
#include "disk.h"

static u8 patch;
static u8 wram_patch;
static u8 wram_bank;
static u8 wram_count;
static u16 last_addr;
static u8 reg[4];
static u8 shift;
static u8 regbuf;

static NesMirroring mirroringFromRegs()
{
	switch (reg[0] & 3) {
	case 0: return SingleLow; break;
	case 1: return SingleHigh; break;
	case 2: return VerticalMirroring; break;
	case 3: return HorizontalMirroring; break;
	default: UNREACHABLE(); return SingleLow; break;
	}
}

static void writeHigh(u16 addr, u8 data)
{
	if (wram_patch == 1 && addr == 0xbfff) {
		wram_count++;
		wram_bank += data & 0x01;
		if (wram_count == 5) {
			nesSetWram8KBank(3, wram_bank ? 1 : 0);
			wram_bank = wram_count = 0;
		}
	}
	if (patch != 1) {
		if ((addr & 0x6000) != (last_addr & 0x6000))
			shift = regbuf = 0;
		last_addr = addr;
	}
	if (data & 0x80) {
		shift = regbuf = 0;
		reg[0] |= 0x0c;		// D3=1,D2=1
		return;
	}
	if (data & 0x01)
		regbuf |= 1 << shift;
	if (++shift < 5)
		return;
	addr = (addr&0x7fff)>>13;
	reg[addr] = regbuf;

	regbuf = 0;
	shift = 0;
	if (patch != 1) {
		// for normal cartridge
		switch (addr) {
		case 0:
			nesSetMirroring(mirroringFromRegs());
			break;
		case 1:
		case 2:
			if (nesVromSize1KB) {
				if (reg[0] & 0x10) {
					// CHR 4K bank lower($0000-$0fff)
					nesSetVrom4KBank(0, reg[1]);
					// CHR 4K bank higher($1000-$1fff)
					nesSetVrom4KBank(4, reg[2]);
				} else {
					// CHR 8K bank($0000-$1fff)
					nesSetVrom8KBank(reg[1] >> 1);
				}
			} else {
				// for Romancia
				if (reg[0] & 0x10)
					nesSetCram4KBank((addr&2)<<1, reg[addr]);
			}
			break;
		case 3:
			if (!(reg[0] & 0x08)) {
				// PRG 32K bank ($8000-$ffff)
				nesSetRom32KBank(reg[3] >> 1);
			} else {
				if (reg[0] & 0x04) {
					// PRG 16K bank ($8000-$bfff)
					nesSetRom16KBank(4, reg[3]);
					nesSetRom16KBank(6, nesRomSize16KB-1);
				} else {
					// PRG 16K bank ($c000-$ffff)
					nesSetRom16KBank(6, reg[3]);
					nesSetRom16KBank(4, 0);
				}
			}
			break;
		}
	} else {
		// for 512K/1M byte cartridge
		int	promBase = 0;
		if (nesRomSize16KB >= 32)
			promBase = reg[1] & 0x10;
		// for FinalFantasy I&II
		if (wram_patch == 2) {
			if (!(reg[1] & 0x18))
				nesSetWram8KBank(3, 0);
			else
				nesSetWram8KBank(3, 1);
		}
		if (addr == 0)
			nesSetMirroring(mirroringFromRegs());
		// register #1 and #2
		if (nesVromSize1KB) {
			if (reg[0] & 0x10) {
				// CHR 4K bank lower($0000-$0fff)
				nesSetVrom4KBank(0, reg[1]);
				// CHR 4K bank higher($1000-$1fff)
				nesSetVrom4KBank(4, reg[2]);
			} else {
				// CHR 8K bank($0000-$1fff)
				nesSetVrom8KBank(reg[1] >> 1);
			}
		} else {
			// for Romancia
			if (reg[0] & 0x10) {
				nesSetCram4KBank(0, reg[1]);
				nesSetCram4KBank(4, reg[2]);
			}
		}
		// register #3
		if (!(reg[0] & 0x08)) {
			// PRG 32K bank ($8000-$ffff)
			nesSetRom32KBank((reg[3] & (0xf + promBase)) >> 1);
		} else {
			if (reg[0] & 0x04) {
				// PRG 16K bank ($8000-$bfff)
				nesSetRom16KBank(4, promBase + (reg[3] & 0x0f));
				if (nesRomSize16KB >= 32)
					nesSetRom16KBank(6, promBase+16-1);
			} else {
				// PRG 16K bank ($c000-$ffff)
				nesSetRom16KBank(6, promBase + (reg[3] & 0x0f));
				if (nesRomSize16KB >= 32)
					nesSetRom16KBank(4, promBase);
			}
		}
	}
}

void Mapper001::reset()
{
	NesMapper::reset();
	writeHigh = ::writeHigh;

	reg[0] = 0x0c; // D3=1,D2=1
	reg[1] = reg[2] = reg[3] = 0;
	shift = regbuf = 0;
	last_addr = 0;

	patch = 0;
	wram_patch = 0;
	wram_bank = 0;
	wram_count = 0;

	if (nesRomSize16KB < 32) {
		nesSetRom8KBanks(0, 1, nesRomSize8KB-2, nesRomSize8KB-1);
	} else {
		// for 512K/1M byte Cartridge
		nesSetRom16KBank(4, 0);
		nesSetRom16KBank(6, 16-1);
		patch = 1;
	}
	u32 crc = nesDiskCrc;

	if (crc == 0xb8e16bd0) {	// Snow Bros.(J)
		patch = 2;
	}
	if (crc == 0xc96c6f04) {	// Venus Senki(J)
		nesEmuSetRenderMethod(NesEmu::PostAllRender);
	}
	if (crc == 0x4d2edf70) {	// Night Rider(J)
		nesEmuSetRenderMethod(NesEmu::TileRender);
	}
	if (crc == 0xcd2a73f0) {	// Pirates!(U)
		nesEmuSetRenderMethod(NesEmu::TileRender);
		patch = 2;
	}
	if (crc == 0xd878ebf5) {	// Ninja Ryukenden(J)
		nesEmuSetRenderMethod(NesEmu::PostAllRender);
	}
	if (crc == 0x466efdc2) {	// Final Fantasy(J)
		nesEmuSetRenderMethod(NesEmu::TileRender);
	}
	if (crc == 0xc9556b36) {	// Final Fantasy I&II(J)
		nesEmuSetRenderMethod(NesEmu::TileRender);
		wram_patch = 2;
	}
	if (crc == 0x717e1169) {	// Cosmic Wars(J)
		nesEmuSetRenderMethod(NesEmu::PreAllRender);
	}
	if (crc == 0xc05d2034) {	// Snake's Revenge(U)
		nesEmuSetRenderMethod(NesEmu::PreAllRender);
	}

	if (crc == 0xb8747abf		// Best Play - Pro Yakyuu Special(J)
	 || crc == 0x29449ba9		// Nobunaga no Yabou - Zenkoku Ban(J)
	 || crc == 0x2b11e0b0		// Nobunaga no Yabou - Zenkoku Ban(J)(alt)
	 || crc == 0x4642dda6		// Nobunaga's Ambition(U)
	 || crc == 0xfb69743a		// Aoki Ookami to Shiroki Mejika - Genghis Khan(J)
	 || crc == 0x2225c20f		// Genghis Khan(U)
	 || crc == 0xabbf7217		// Sangokushi(J)
	) {
		wram_patch = 1;
		wram_bank  = 0;
		wram_count = 0;
	}
}

void Mapper001::extSl()
{
	emsl.var("shift", shift);
	emsl.var("regbuf", regbuf);
	emsl.var("wram_bank", wram_bank);
	emsl.var("wram_count", wram_count);
	emsl.var("last_addr", last_addr);
	emsl.array("reg", reg, sizeof(reg));
}
