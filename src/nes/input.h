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

#ifndef NESINPUT_H
#define NESINPUT_H

#include <base/emu.h>

class NesInput : public QObject
{
	Q_OBJECT
	Q_ENUMS(ExtraDevice)
	Q_PROPERTY(ExtraDevice extraDevice READ extraDevice WRITE setExtraDevice NOTIFY extraDeviceChanged)
public:
	enum ExtraDevice {
		None,
		Zapper,
		Paddle
	};

	void setExtraDevice(ExtraDevice extraDevice);
	ExtraDevice extraDevice() const;

	void sl();
signals:
	void extraDeviceChanged();
};

class NesInputExtraDevice
{
public:
	virtual void reset() {}
	virtual void strobe() {}
	virtual void write(u16 addr, u8 data) { Q_UNUSED(addr) Q_UNUSED(data) }
	virtual   u8 read(u16 addr) { Q_UNUSED(addr) return 0x00; }
	virtual void sync(const EmuInput *hostInput) { Q_UNUSED(hostInput) }
};

extern NesInput nesInput;
extern NesInputExtraDevice nesInputNullExtra;

extern void nesInputReset();
extern void nesInputWrite(u16 addr, u8 data);
extern   u8 nesInputRead(u16 addr);
extern void nesInputSyncWithHost(const EmuInput *hostInput);

#endif // NESINPUT_H
