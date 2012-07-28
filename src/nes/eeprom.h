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

#ifndef EEPROM_H
#define EEPROM_H

#include <base/emu.h>

class X24C01 {
public:
	void reset(u8 *eedata);
	void write(bool scl_in, bool sda_in);
	bool read() const;

	void sl(const QString &groupName);
private:
	enum State {
		Idle,
		Address,
		Read,
		Write,
		Ack,
		AckWait
	};
	int m_nowState;
	int m_nextState;
	int m_bitCounter;
	u8 m_address;
	u8 m_data;
	bool m_sda;
	bool m_sclOld;
	bool m_sdaOld;

	u8 *m_eedata;
};

class X24C02 {
public:
	void reset(u8 *eedata);
	void write(bool scl_in, bool sda_in);
	bool read() const;

	void sl(const QString &groupName);
private:
	enum State {
		Idle,
		DeviceAddress,
		Address,
		Read,
		Write,
		Ack,
		Nak,
		AckWait
	};
	int m_nowState;
	int m_nextState;
	int m_bitCounter;
	u8 m_address;
	u8 m_data;
	u8 m_rw;
	bool m_sda;
	bool m_sclOld, m_sdaOld;

	u8 *m_eedata;
};

#endif // EEPROM_H
