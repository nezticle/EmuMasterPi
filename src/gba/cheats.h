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

#ifndef GBACHEATS_H
#define GBACHEATS_H

#include <base/emu.h>
#include <QtCore/QAbstractListModel>
#include <QtCore/QStringList>
#include <QtGui/QValidator>

class GbaGameSharkCheatGroup;

class GbaGameSharkCheat
{
public:
	GbaGameSharkCheat(const GbaGameSharkCheatGroup &group,
					  const QString &cheat);
	bool isValid() const { return m_addr != 0 || m_data != 0; }
	QString idcode() const;
	u32 addr() const { return m_addr; }
	u32 data() const { return m_data; }
private:
	u32 m_addr;
	u32 m_data;
};

class GbaGameSharkCheatGroup
{
public:
	enum Generation {
		V1,
		V3
	};

	GbaGameSharkCheatGroup(Generation gen,
						   const QStringList &list);
	Generation generation() const { return m_generation; }
	u32 seed(int i) const { return m_seed[i]; }
	QString idcode() const;
	void process() const;
	void setEnabled(bool on);
	bool isEnabled() const { return m_enabled; }
private:
	void changeEncryption(u16 data);
	void processV1() const;
	void processV3() const;

	Generation m_generation;
	QStringList m_cheatStrings;
	QList<GbaGameSharkCheat> m_cheats;
	bool m_enabled;
	u32 m_seed[4];
	u16 m_deadface;
};

class GbaGameSharkValidator : public QValidator
{
public:
	void fixup(QString & input) const;
	State validate(QString & input, int & pos) const;
};

class GbaCheats : public QAbstractListModel
{
	Q_OBJECT
public:
	enum RoleType {
		GroupRole = Qt::UserRole+1,
		DescriptionRole,
		EnableRole
	};
	GbaCheats();
	int rowCount(const QModelIndex &parent) const;
	QVariant data(const QModelIndex &index, int role) const;

	Q_INVOKABLE void setEnabled(int i, bool on);
	Q_INVOKABLE void addNew(const QStringList &codeList,
							const QString &description, bool v3);
	Q_INVOKABLE void removeAt(int i);
	Q_INVOKABLE bool checkIdcodeForGroup(int groupIndex, const QString &expected) const;

	void sl();
private:
	void reloadCheats();

	QList<QStringList> m_groups;
	QList<int> m_generations;
	QStringList m_descriptions;
	QList<bool> m_enabled;
};

extern GbaCheats gbaCheats;

extern void gbaCheatsProcess();

#endif // GBACHEATS_H
