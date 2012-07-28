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

#include "keybinputdevice.h"
#include "emu.h"
#include <QtCore/QSettings>
#include <QtCore/QDataStream>

KeybInputDevice::KeybInputDevice(QObject *parent) :
	HostInputDevice("keyb", QObject::tr("Keyboard"), parent) ,
	m_buttons(0)
{
	setupEmuFunctionList();

	QSettings s;
	s.beginGroup("keyboard");
	for (uint i = 0;  i < sizeof(m_defaultMapping)/sizeof(int); i++) {
		int hostKey = s.value(QString::number(i), m_defaultMapping[i]).toInt();
		int button = 1 << i;
		m_mapping[hostKey] = button;
	}
	s.endGroup();

	setEmuFunction(1);

	QObject::connect(this, SIGNAL(emuFunctionChanged()), SLOT(onEmuFunctionChanged()));
}

void KeybInputDevice::setupEmuFunctionList()
{
	QStringList functionNameList;
	functionNameList << tr("None")
					 << tr("Pad A")
					 << tr("Pad B")
					 << tr("Keyboard");
	setEmuFunctionNameList(functionNameList);
}

void KeybInputDevice::onEmuFunctionChanged()
{
	m_buttons = 0;
	m_keys.clear();
}

void KeybInputDevice::sync(EmuInput *emuInput)
{
	if (emuFunction() <= 0)
		return;

	if (emuFunction() <= 2) {
		int padIndex = emuFunction() - 1;
		emuInput->pad[padIndex].setButtons(m_buttons);
	} else if (emuFunction() == 3) {
		while (!m_keys.isEmpty())
			emuInput->keyb.enqueue(m_keys.takeFirst());
	}
}

void KeybInputDevice::processKey(Qt::Key key, bool down)
{
	if (emuFunction() <= 0)
		return;

	if (emuFunction() <= 2) {
		int button = m_mapping[key];
		if (down)
			m_buttons |=  button;
		else
			m_buttons &= ~button;
	} else if (emuFunction() == 3) {
		int k = key;
		if (down)
			k |= (1 << 31);
		m_keys.append(k);
	}
}

const int KeybInputDevice::m_defaultMapping[14] =
{
	Qt::Key_Right,
	Qt::Key_Down,
	Qt::Key_Up,
	Qt::Key_Left,

	Qt::Key_C,
	Qt::Key_X,
	Qt::Key_S,
	Qt::Key_D,

	Qt::Key_G,
	Qt::Key_H,
	Qt::Key_T,
	Qt::Key_Z,

	Qt::Key_Q,
	Qt::Key_W
};

const char *KeybInputDevice::m_defaultMappingText[] =
{
	"Right",
	"Down",
	"Up",
	"Left",

	"c",
	"x",
	"s",
	"d",

	"g",
	"h",
	"t",
	"z",

	"q",
	"w"
};

void KeybInputDevice::setPadButton(int buttonIndex, int hostKey, const QString hostKeyText)
{
	QSettings s;
	s.beginGroup("keyboard");
	s.setValue(QString::number(buttonIndex), hostKey);
	s.setValue(QString("%1.text").arg(buttonIndex), hostKeyText);
	m_mapping[hostKey] = buttonIndex << 1;
	s.endGroup();
}

int KeybInputDevice::padButton(int buttonIndex) const
{
	return m_mapping.key(buttonIndex << 1);
}

QString KeybInputDevice::padButtonText(int buttonIndex) const
{
	QSettings s;
	s.beginGroup("keyboard");
	QString text = s.value(QString("%1.text").arg(buttonIndex),
						   m_defaultMappingText[buttonIndex]).toString();
	s.endGroup();
	return text;
}

void KeybInputDevice::resetToDefaults() {
	QSettings s;
	s.beginGroup("keyboard");
	for (uint i = 0;  i < sizeof(m_defaultMapping)/sizeof(int); i++) {
		int hostKey = m_defaultMapping[i];
		s.setValue(QString::number(i), hostKey);
		m_mapping[hostKey] = 1 << i;
	}
	s.endGroup();
}

const char *KeybInputDevice::m_padButtonName[] =
{
	"Right",
	"Down",
	"Up",
	"Left",

	"A",
	"B",
	"X",
	"Y",

	"L1",
	"R1",
	"L2",
	"R2",

	"Start",
	"Select"
};

QString KeybInputDevice::padButtonName(int buttonIndex) const
{
	return m_padButtonName[buttonIndex];
}
