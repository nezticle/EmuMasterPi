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

#ifndef HOSTINPUT_H
#define HOSTINPUT_H

class Emu;
class HostInputDevice;
class KeybInputDevice;
#include "base_global.h"
#include <QtCore/QObject>
#include <QtGamepad/QGamepadInputState>
#include <QtGamepad/QGamepadKeyBindings>
#include <QtGamepad/QGamepadManager>

class HostInput : public QObject
{
	Q_OBJECT
public:
	explicit HostInput(Emu *emu);
	~HostInput();

	void sync();

public slots:
	void loadFromConf();
signals:
	void pause();
	void quit();
protected:
	bool eventFilter(QObject *o, QEvent *e);
private:
    void initKeymapping();
	Emu *m_emu;
    QGamepadInputState *m_inputState;
    QGamepadKeyBindings *m_keybindings;
    QGamepadManager *m_gamepadManager;
};

#endif // HOSTINPUT_H
