/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef STRINGLISTPROXY_H
#define STRINGLISTPROXY_H

#include "base_global.h"
#include <QtCore/QStringListModel>

class BASE_EXPORT StringListProxy : public QStringListModel
{
	Q_OBJECT
	Q_PROPERTY(QStringList stringListModel READ stringList WRITE setStringList NOTIFY stringListChanged)
	Q_PROPERTY(int count READ count NOTIFY stringListChanged)
public:
	explicit StringListProxy(QObject *parent = 0);
	void setStringList(const QStringList &strings);
	int count() const;
	Q_INVOKABLE QString get(int index) const;
signals:
	void stringListChanged();
};

#endif // STRINGLISTPROXY_H
