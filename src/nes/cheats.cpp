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

#include "cheats.h"
#include "nes.h"
#include "mapper.h"
#include "disk.h"
#include "cheats.h"
#include <base/pathmanager.h>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>

NesCheats nesCheats;
static QList<GameGenieCode> enabledCheats;

void nesCheatsProcess()
{
	for (int i = 0; i < enabledCheats.size(); i++) {
		GameGenieCode &code = enabledCheats[i];
		uint addr = code.address() | 0x8000;
		if (code.isEightCharWide()) {
			if (nesCpuReadDirect(addr) == code.expectedData())
				nesCpuWriteDirect(addr, code.replaceData());
		} else {
			code.setExpectedData(nesCpuReadDirect(addr));
			nesCpuWriteDirect(addr, code.replaceData());
		}
	}
}

void NesCheats::setCurrent(const QList<GameGenieCode> &codes)
{
	for (int i = 0; i < enabledCheats.size(); i++) {
		const GameGenieCode &code = enabledCheats.at(i);
		u16 addr = code.address() | 0x8000;
		if (nesCpuReadDirect(addr) == code.replaceData())
			nesCpuWriteDirect(addr, code.expectedData());
	}
	enabledCheats = codes;
	nesCheatsProcess();
}

static const char *gameGenieString = "APZLGITYEOXUKSVN";

bool GameGenieCode::parse(const QString &s)
{
	m_address = 0;
	m_replace = 0;
	m_expected = 0;

	u8 table[8];

	if (s.size() != 6 && s.size() != 8)
		return false;
	m_eightChars = (s.size() == 8);

	for (int i = 0; i < s.size(); i++) {
		char c = s.at(i).toLatin1();
		const char *p = strchr(gameGenieString, c);
		if (!p)
			return false;
		table[i] = p - gameGenieString;
	}

	if (!m_eightChars) {
		m_address |= (table[3] & 0x07) <<12;
		m_address |= (table[4] & 0x08) << 8;
		m_address |= (table[5] & 0x07) << 8;
		m_address |= (table[1] & 0x08) << 4;
		m_address |= (table[2] & 0x07) << 4;
		m_address |= (table[3] & 0x08);
		m_address |= (table[4] & 0x07);

		m_replace |= (table[0] & 0x08) << 4;
		m_replace |= (table[1] & 0x07) << 4;
		m_replace |= (table[5] & 0x08);
		m_replace |= (table[0] & 0x07);
	} else {
		m_address |= (table[3] & 0x07) <<12;
		m_address |= (table[4] & 0x08) << 8;
		m_address |= (table[5] & 0x07) << 8;
		m_address |= (table[1] & 0x08) << 4;
		m_address |= (table[2] & 0x07) << 4;
		m_address |= (table[3] & 0x08);
		m_address |= (table[4] & 0x07);

		m_replace |= (table[0] & 0x08) << 4;
		m_replace |= (table[1] & 0x07) << 4;
		m_replace |= (table[7] & 0x08);
		m_replace |= (table[0] & 0x07);

		m_expected |= (table[6] & 0x08) << 4;
		m_expected |= (table[7] & 0x07) << 4;
		m_expected |= (table[5] & 0x08);
		m_expected |= (table[6] & 0x07);
	}
	return true;
}

void GameGenieValidator::fixup(QString &input) const
{
	input = input.toUpper();
	QString result;
	for (int i = 0; i < input.size(); i++) {
		char c = input.at(i).toLatin1();
		if (strchr(gameGenieString, c))
			result.append(c);
	}
	input = result;
}

QValidator::State GameGenieValidator::validate(QString &input, int &pos) const
{
	Q_UNUSED(pos)
	fixup(input);
	return GameGenieCode().parse(input) ? Acceptable : Intermediate;
}

NesCheats::NesCheats()
{
}

void NesCheats::sl()
{
	emsl.begin("cheats");
	emsl.var("codes", m_codes);
	emsl.var("descriptions", m_descriptions);
	emsl.var("enable", m_enable);
	emsl.end();

	if (!emsl.save) {
		setCurrent(enabledList());
        beginResetModel();
        endResetModel();
    }
}

QHash<int, QByteArray> NesCheats::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles.insert(CodeRole, "code");
    roles.insert(DescriptionRole, "codeDescription");
    roles.insert(EnableRole, "codeEnabled");
    return roles;
}

QList<GameGenieCode> NesCheats::enabledList() const
{
	QList<GameGenieCode> result;
	for (int i = 0; i < m_codes.size(); i++) {
		if (m_enable.at(i)) {
			GameGenieCode gcc;
			gcc.parse(m_codes.at(i));
			result.append(gcc);
		}
	}
	return result;
}

void NesCheats::setEnabled(int i, bool on)
{
	if (i >= 0 && i < m_enable.size() && m_enable.at(i) != on) {
		m_enable[i] = on;
		emit dataChanged(index(i), index(i));
		setCurrent(enabledList());
	}
}

int NesCheats::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)
	return m_codes.size();
}

QVariant NesCheats::data(const QModelIndex &index, int role) const
{
	int i = index.row();
	if (i < 0 || i >= m_codes.size())
		return QVariant();
	if (role == CodeRole)
		return m_codes.at(i);
	else if (role == DescriptionRole)
		return m_descriptions.at(i);
	else if (role == EnableRole)
		return m_enable.at(i);
	return QVariant();
}

void NesCheats::addNew(const QString &code, const QString &description)
{
	QString codeUpper = code.toUpper();
	if (!GameGenieCode().parse(codeUpper))
		return;
	if (m_codes.contains(codeUpper))
		return;

	beginInsertRows(QModelIndex(), m_codes.size(), m_codes.size());
	m_codes.append(codeUpper);
	m_descriptions.append(description);
	m_enable.append(true);
	endInsertRows();

	setCurrent(enabledList());
}

void NesCheats::removeAt(int i)
{
	if (i < 0 || i >= m_codes.size())
		return;

	beginRemoveRows(QModelIndex(), i, i);
	m_codes.removeAt(i);
	m_descriptions.removeAt(i);
	m_enable.removeAt(i);
	endRemoveRows();

	setCurrent(enabledList());
}
