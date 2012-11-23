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

#include "hostinput.h"
#include "hostvideo.h"
#include "emu.h"
#include "configuration.h"
#include "keybinputdevice.h"
#include "gamepadinputdevice.h"
#include "memutils.h"
#include <QtGui/QKeyEvent>

/*!
	\class HostInput
	HostInput class manages input devices. It filters input events from main
	window and passes them to appropriate objects. HostInput starts sixaxis
	server.

	The events are converted and passed to emulation only on update() call.
	This is done by HostVideo after each frame.
 */

/*! Creates a HostInput object with the given \a emu. */
HostInput::HostInput(Emu *emu) :
    m_emu(emu)
{
	m_devices.append(new KeybInputDevice(this));
    m_devices.append(new GamepadInputDevice(this));
}

/*! Destroys HostInput object. */
HostInput::~HostInput()
{
}

/*! \internal */
bool HostInput::eventFilter(QObject *o, QEvent *e)
{
	Q_UNUSED(o)
	if (e->type() == QEvent::KeyPress || e->type() == QKeyEvent::KeyRelease) {
		// filter key events
		bool down = (e->type() == QEvent::KeyPress);
		QKeyEvent *ke = static_cast<QKeyEvent *>(e);
		if (!ke->isAutoRepeat())
			keybInputDevice()->processKey(static_cast<Qt::Key>(ke->key()), down);
		ke->accept();
		return true;
    }

	return false;
}

/*! Returns keyboard input device. */
KeybInputDevice *HostInput::keybInputDevice() const
{
    return static_cast<KeybInputDevice *>(m_devices.at(0));
}

/*!
	\fn qreal HostInput::padOpacity() const
	Returns opacity of images on touch screen.
 */

/*!
	\fn QList<HostInputDevice *> HostInput::devices() const
	Returns a list of input devices available on host.
 */

/*! Synchronizes input values from host to the emulation. */
void HostInput::sync()
{
	EmuInput *emuInput = m_emu->input();
	memset32(emuInput, 0, sizeof(EmuInput)/4);
    for (int i = 0; i < m_devices.size(); i++) {
        m_devices.at(i)->sync(emuInput);
    }
//    //check for killswitch
//    if (emuInput->pad[0].buttons() & EmuPad::Button_Mode)
//        emit quit();
}

/*! Loads function in the emulation for each input device.  */
void HostInput::loadFromConf()
{
	for (int i = 0; i < m_devices.size(); i++)
		m_devices.at(i)->updateEmuFunction();
}
