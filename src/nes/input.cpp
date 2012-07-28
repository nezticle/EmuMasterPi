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

#include "input.h"
#include "nes.h"
#include "ppu.h"
#include "disk.h"
#include "inputpaddle.h"
#include "inputzapper.h"

NesInput nesInput;
NesInputExtraDevice nesInputNullExtra;
static NesInputExtraDevice *extra = &nesInputNullExtra;

static bool nextStrobe;
static u8 pad[2];
static u8 padReg[2];

static void detectExtraDevices();

static inline void strobe()
{
	for (int i = 0; i < 2; i++)
		padReg[i] = pad[i];
	extra->strobe();
}

void nesInputReset()
{
	for (int i = 0; i < 2; i++) {
		pad[i] = 0;
		padReg[i] = 0;
	}
	nextStrobe = false;

	nesInputZapper.reset();
	nesInputPaddle.reset();

	detectExtraDevices();
}

void nesInputWrite(u16 addr, u8 data)
{
	Q_ASSERT(addr < 2);
	if (addr == 0) {
		if (data & 0x01) {
			nextStrobe = true;
		} else if (nextStrobe) {
			nextStrobe = false;
			strobe();
		}
	}
	extra->write(addr, data);
}

u8 nesInputRead(u16 addr)
{
	Q_ASSERT(addr < 2);
	u8 data = (padReg[addr] & 1) | extra->read(addr);
	padReg[addr] >>= 1;
	return data;
}

static const int buttonMapping[8] =
{
	EmuPad::Button_A,
	EmuPad::Button_B,
	EmuPad::Button_Select,
	EmuPad::Button_Start,
	EmuPad::Button_Up,
	EmuPad::Button_Down,
	EmuPad::Button_Left,
	EmuPad::Button_Right
};

static inline u8 hostToEmu(int buttons)
{
	u8 result = 0;
	for (int i = 0; i < 8; i++) {
		if (buttons & buttonMapping[i])
			result |= 1 << i;
	}
	return result;
}

void nesInputSyncWithHost(const EmuInput *hostInput)
{
	for (int i = 0; i < 2; i++) {
		int lastPad = pad[i];
		int hostButtons = hostInput->pad[i].buttons();
		pad[i] = hostToEmu(hostButtons);
		if (hostButtons & EmuPad::Button_X)
			pad[i] = (pad[i] & ~1) | ((lastPad&1)^1);
		if (hostButtons & EmuPad::Button_Y)
			pad[i] = (pad[i] & ~2) | ((lastPad&2)^2);
	}
	extra->sync(hostInput);
}

void NesInput::setExtraDevice(ExtraDevice extraDevice)
{
	switch (extraDevice) {
	case Zapper:	extra = &nesInputZapper; break;
	case Paddle:	extra = &nesInputPaddle; break;
	default:		extra = &nesInputNullExtra; break;
	}
	emit extraDeviceChanged();
}

NesInput::ExtraDevice NesInput::extraDevice() const
{
	if (extra == &nesInputZapper)
		return Zapper;
	else if (extra == &nesInputPaddle)
		return Paddle;
	else
		return None;
}

void NesInput::sl()
{
	int extra = extraDevice();
	emsl.begin("input");
	emsl.var("extra", extra);
	emsl.end();

	if (!emsl.save) {
		if (extra != extraDevice())
			setExtraDevice(static_cast<ExtraDevice>(extra));
	}
}

static void detectExtraDevices()
{
	u32 crc = nesDiskCrc;
	if (crc == 0xfbfc6a6c		// Adventures of Bayou Billy, The(E)
	 || crc == 0xcb275051		// Adventures of Bayou Billy, The(U)
	 || crc == 0xfb69c131		// Baby Boomer(Unl)(U)
	 || crc == 0xf2641ad0		// Barker Bill's Trick Shooting(U)
	 || crc == 0xbc1dce96		// Chiller (Unl)(U)
	 || crc == 0x90ca616d		// Duck Hunt(JUE)
	 || crc == 0x59e3343f		// Freedom Force(U)
	 || crc == 0x242a270c		// Gotcha!(U)
	 || crc == 0x7b5bd2de		// Gumshoe(UE)
	 || crc == 0x255b129c		// Gun Sight(J)
	 || crc == 0x8963ae6e		// Hogan's Alley(JU)
	 || crc == 0x51d2112f		// Laser Invasion(U)
	 || crc == 0x0a866c94		// Lone Ranger, The(U)
//	 || crc == 0xe4c04eea		// Mad City(J)
	 || crc == 0x9eef47aa		// Mechanized Attack(U)
	 || crc == 0xc2db7551		// Shooting Range(U)
	 || crc == 0x163e86c0		// To The Earth(U)
	 || crc == 0x42d893e4		// Operation Wolf(J)
	 || crc == 0x1388aeb9		// Operation Wolf(U)
	 || crc == 0x0d3cf705		// Wild Gunman(J)
	 || crc == 0x389960db) {	// Wild Gunman(JUE)
		nesInput.setExtraDevice(NesInput::Zapper);
	}
	if( crc == 0x35893b67		// Arkanoid(J)
	 || crc == 0x6267fbd1) {	// Arkanoid 2(J)
		nesInput.setExtraDevice(NesInput::Paddle);
	}
}
