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

#include "hostinputdevice.h"
#include "configuration.h"

/*!
	\class HostInputDevice
	HostInputDevice class is an abstract input device on the host side.
	Each input device can execute one of the several possible functions
	in the emulation, it can be pad, keyboard, mouse, etc. The function
	is stored in the configuration.
 */

/*!
	Creates a new object of HostInputDevice class with the given \a shortName,
	\a name, and \a parent. The \a name is human readable name, while \a shortName
	is used for storing info in the configuration and to resolve filename
	of the image presenting the input device in QML code.
 */
HostInputDevice::HostInputDevice(const QString &shortName,
								 const QString &name,
								 QObject *parent) :
	QObject(parent),
	m_shortName(shortName),
	m_name(name),
	m_emuFunction(0)
{
	Q_ASSERT(!m_shortName.isEmpty());
	setDeviceIndex(0);
}

/*!
	Changes current function in the emulation to the given \a index.
	The \a index takes values from 0 to the number of available functions.
	Available functions are set by setEmuFunctionNameList().
	Returns true on success, otherwise false.

	\sa setEmuFunctionNameList()
 */
bool HostInputDevice::setEmuFunction(int index)
{
	if (index < 0 || index >= m_emuFunctionNameList.size())
		return false;
	if (index != m_emuFunction) {
		m_emuFunction = index;
		emConf.setValue(m_confName, index);
		emit emuFunctionChanged();
	}
	return true;
}

/*! Returns a name of the current function that input device performs in the emulation. */
QString HostInputDevice::emuFunctionName() const
{
	return m_emuFunctionNameList.at(m_emuFunction);
}

/*!
	Sets a list with names of available functions that input device can perform
	in the emulation.
 */
void HostInputDevice::setEmuFunctionNameList(const QStringList &list)
{
	m_emuFunctionNameList = list;
}

/*!
	Returns a list with names of available functions that input device can perform
	in the emulation.
 */
QStringList HostInputDevice::emuFunctionNameList() const
{
	return m_emuFunctionNameList;
}

void HostInputDevice::setDeviceIndex(int index)
{
	m_confName = QString("input.%1%2.emuFunction").arg(m_shortName).arg(index);
}

void HostInputDevice::updateEmuFunction()
{
    qDebug("%s", m_confName.toAscii().constData());
	int conf = emConf.value(m_confName, -1).toInt();
	if (conf >= 0)
		setEmuFunction(conf);
}

/*!
	\fn HostInputDevice::update(int *data)
	Writes data to the emulated systems.
	The data depends on the chosen configuration.
*/
