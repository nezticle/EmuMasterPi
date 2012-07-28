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

#ifndef HOSTINPUTDEVICE_H
#define HOSTINPUTDEVICE_H

#include "emuinput.h"
#include <QtCore/QObject>
#include <QtCore/QStringList>

class HostInputDevice : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QString shortName READ shortName CONSTANT)
	Q_PROPERTY(QString name READ name CONSTANT)
	Q_PROPERTY(int emuFunction READ emuFunction WRITE setEmuFunction NOTIFY emuFunctionChanged)
	Q_PROPERTY(QString emuFunctionName READ emuFunctionName NOTIFY emuFunctionChanged)
	Q_PROPERTY(QStringList emuFunctionNameList READ emuFunctionNameList CONSTANT)
public:
	explicit HostInputDevice(const QString &shortName,
							 const QString &name,
							 QObject *parent = 0);
	QString shortName() const;
	QString name() const;

	int emuFunction() const;
	bool setEmuFunction(int index);
	void updateEmuFunction();

	QString emuFunctionName() const;
	QStringList emuFunctionNameList() const;

	void setDeviceIndex(int index);

	virtual void sync(EmuInput *emuInput) = 0;
signals:
	void emuFunctionChanged();
protected:
	void setEmuFunctionNameList(const QStringList &list);
private:
	QString m_shortName;
	QString m_name;
	int m_emuFunction;
	QString m_confName;
	QStringList m_emuFunctionNameList;
};

inline QString HostInputDevice::shortName() const
{ return m_shortName; }
inline QString HostInputDevice::name() const
{ return m_name; }
inline int HostInputDevice::emuFunction() const
{ return m_emuFunction; }

#endif // HOSTINPUTDEVICE_H
