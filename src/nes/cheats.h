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

#ifndef NESCHEATS_H
#define NESCHEATS_H

#include <base/emu.h>
#include <QtCore/QAbstractListModel>
#include <QtCore/QStringList>
#include <QtGui/QValidator>

class GameGenieCode
{
public:
	bool parse(const QString &s);
	u16 address() const;
	u8 expectedData() const;
	u8 replaceData() const;
	bool isEightCharWide() const;

	void setExpectedData(u8 data);
private:
	u16 m_address;
	u8 m_expected;
	u8 m_replace;
	bool m_eightChars;
};

inline u16 GameGenieCode::address() const
{ return m_address; }
inline u8 GameGenieCode::expectedData() const
{ return m_expected; }
inline u8 GameGenieCode::replaceData() const
{ return m_replace; }
inline bool GameGenieCode::isEightCharWide() const
{ return m_eightChars; }
inline void GameGenieCode::setExpectedData(u8 data)
{ m_expected = data; }

class GameGenieValidator : public QValidator
{
public:
	void fixup(QString & s) const;
	State validate(QString & input, int & pos) const;
};

class NesCheats : public QAbstractListModel
{
	Q_OBJECT
public:
	enum RoleType {
		CodeRole = Qt::UserRole+1,
		DescriptionRole,
		EnableRole
	};
	NesCheats();
	int rowCount(const QModelIndex &parent) const;
	QVariant data(const QModelIndex &index, int role) const;

	void sl();
	QList<GameGenieCode> enabledList() const;
	Q_INVOKABLE void setEnabled(int i, bool on);
	Q_INVOKABLE void addNew(const QString &code, const QString &description);
	Q_INVOKABLE void removeAt(int i);
private:
	void setCurrent(const QList<GameGenieCode> &codes);

	QStringList m_codes;
	QStringList m_descriptions;
	QList<bool> m_enable;
};

extern NesCheats nesCheats;

extern void nesCheatsProcess();

#endif // NESCHEATS_H
