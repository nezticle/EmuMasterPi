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

#ifndef KEYBINPUTDEVICE_H
#define KEYBINPUTDEVICE_H

#include "hostinputdevice.h"
#include <QtCore/QHash>

class BASE_EXPORT KeybInputDevice : public HostInputDevice
{
	Q_OBJECT
public:
	explicit KeybInputDevice(QObject *parent = 0);
	void sync(EmuInput *emuInput);
	void processKey(Qt::Key key, bool down);

	Q_INVOKABLE void setPadButton(int buttonIndex, int hostKey, const QString hostKeyText);
	Q_INVOKABLE int padButton(int buttonIndex) const;
	Q_INVOKABLE QString padButtonText(int buttonIndex) const;
	Q_INVOKABLE void resetToDefaults();
	Q_INVOKABLE QString padButtonName(int buttonIndex) const;
private slots:
	void onEmuFunctionChanged();
private:
	void setupEmuFunctionList();

	QHash<int, int> m_mapping;
	int m_buttons;
	QList<int> m_keys;

	static const int m_defaultMapping[14];
	static const char *m_defaultMappingText[];
	static const char *m_padButtonName[];
};

#endif // KEYBINPUTDEVICE_H
