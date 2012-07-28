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

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include "base_global.h"
#include <QtCore/QObject>
#include <QtCore/QVariant>

class BASE_EXPORT Configuration
{
public:
	static void setupAppInfo();

	Configuration();
	void setValue(const QString &name, const QVariant &value);
	QVariant value(const QString &name,
				   const QVariant &defaultValue = QVariant()) const;
	QVariant defaultValue(const QString &name) const;

	void sl();
private:
	void constructDefaults();

	QHash<QString, QVariant> m_data;
	QHash<QString, QVariant> m_defaultData;
};

BASE_EXPORT extern Configuration emConf;

#endif // CONFIGURATION_H
