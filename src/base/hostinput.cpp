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
#include <QtGui/QGuiApplication>

HostInput::HostInput(Emu *emu, QWindow *window)
    : m_emu(emu)
    , m_window(window)
    , m_menuEnabled(false)
{
    m_inputState = new QGamepadInputState(this);
    m_keybindings = new QGamepadKeyBindings(m_inputState);
    m_gamepadManager = new QGamepadManager(this);

    m_window->installEventFilter(this);

    initKeymapping();

    connect(m_gamepadManager, SIGNAL(gamepadEvent(QGamepadInfo*,quint64,int,int,int)),
            m_inputState, SLOT(processGamepadEvent(QGamepadInfo*,quint64,int,int,int)));
}

/*! Destroys HostInput object. */
HostInput::~HostInput()
{
}

/*! \internal */
bool HostInput::eventFilter(QObject *o, QEvent *e)
{
    Q_UNUSED(o)

    if (!m_menuEnabled) {
        if (e->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
            m_inputState->processKeyPressEvent(keyEvent);
            keyEvent->accept();
            return true;
        } else if (e->type() == QEvent::KeyRelease) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
            m_inputState->processKeyReleaseEvent(keyEvent);
            keyEvent->accept();
            return true;
        }
    } else { //check for escape key
        if (e->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
            if (keyEvent->key() == Qt::Key_Escape) {
                m_inputState->processKeyPressEvent(keyEvent);
                keyEvent->accept();
                return true;
            }
        } else if (e->type() == QEvent::KeyRelease) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
            if (keyEvent->key() == Qt::Key_Escape) {
                m_inputState->processKeyReleaseEvent(keyEvent);
                keyEvent->accept();
                return true;
            }
        }
    }

    return false;
}

void HostInput::initKeymapping()
{
    //Player 1
    m_keybindings->addAction("Pad0_Button_Up", QGamepadInputState::Gamepad_Up1, 0);
    m_keybindings->addAction("Pad0_Button_Down", QGamepadInputState::Gamepad_Down1, 0);
    m_keybindings->addAction("Pad0_Button_Right", QGamepadInputState::Gamepad_Right1, 0);
    m_keybindings->addAction("Pad0_Button_Left", QGamepadInputState::Gamepad_Left1, 0);
    m_keybindings->addAction("Pad0_Button_A", QGamepadInputState::Gamepad_B, 0);
    m_keybindings->addAction("Pad0_Button_B", QGamepadInputState::Gamepad_A, 0);
    m_keybindings->addAction("Pad0_Button_X", QGamepadInputState::Gamepad_Y, 0);
    m_keybindings->addAction("Pad0_Button_Y", QGamepadInputState::Gamepad_X, 0);
    m_keybindings->addAction("Pad0_Button_L1", QGamepadInputState::Gamepad_TL1, 0);
    m_keybindings->addAction("Pad0_Button_R1", QGamepadInputState::Gamepad_TR1, 0);
    m_keybindings->addAction("Pad0_Button_L2", QGamepadInputState::Gamepad_TL2, 0);
    m_keybindings->addAction("Pad0_Button_R2", QGamepadInputState::Gamepad_TR2, 0);
    m_keybindings->addAction("Pad0_Button_Start", QGamepadInputState::Gamepad_Start, 0);
    m_keybindings->addAction("Pad0_Button_Select", QGamepadInputState::Gamepad_Select, 0);
    m_keybindings->addAction("Button_Menu", QGamepadInputState::Gamepad_Mode, 0);
    //Player2
    m_keybindings->addAction("Pad1_Button_Up", QGamepadInputState::Gamepad_Up1, 1);
    m_keybindings->addAction("Pad1_Button_Down", QGamepadInputState::Gamepad_Down1, 1);
    m_keybindings->addAction("Pad1_Button_Right", QGamepadInputState::Gamepad_Right1, 1);
    m_keybindings->addAction("Pad1_Button_Left", QGamepadInputState::Gamepad_Left1, 1);
    m_keybindings->addAction("Pad1_Button_A", QGamepadInputState::Gamepad_B, 1);
    m_keybindings->addAction("Pad1_Button_B", QGamepadInputState::Gamepad_A, 1);
    m_keybindings->addAction("Pad1_Button_X", QGamepadInputState::Gamepad_Y, 1);
    m_keybindings->addAction("Pad1_Button_Y", QGamepadInputState::Gamepad_X, 1);
    m_keybindings->addAction("Pad1_Button_L1", QGamepadInputState::Gamepad_TL1, 1);
    m_keybindings->addAction("Pad1_Button_R1", QGamepadInputState::Gamepad_TR1, 1);
    m_keybindings->addAction("Pad1_Button_L2", QGamepadInputState::Gamepad_TL2, 1);
    m_keybindings->addAction("Pad1_Button_R2", QGamepadInputState::Gamepad_TR2, 1);
    m_keybindings->addAction("Pad1_Button_Start", QGamepadInputState::Gamepad_Start, 1);
    m_keybindings->addAction("Pad1_Button_Select", QGamepadInputState::Gamepad_Select, 1);
    m_keybindings->addAction("Button_Menu", QGamepadInputState::Gamepad_Mode, 1);

    //Player 1 Keyboard
    m_keybindings->addAction("Pad0_Button_Up", Qt::Key_Up);
    m_keybindings->addAction("Pad0_Button_Down", Qt::Key_Down);
    m_keybindings->addAction("Pad0_Button_Right", Qt::Key_Right);
    m_keybindings->addAction("Pad0_Button_Left", Qt::Key_Left);
    m_keybindings->addAction("Pad0_Button_A", Qt::Key_X);
    m_keybindings->addAction("Pad0_Button_B", Qt::Key_Z);
    m_keybindings->addAction("Pad0_Button_X", Qt::Key_S);
    m_keybindings->addAction("Pad0_Button_Y", Qt::Key_A);
    m_keybindings->addAction("Pad0_Button_L1", Qt::Key_Q);
    m_keybindings->addAction("Pad0_Button_R1", Qt::Key_W);
    m_keybindings->addAction("Pad0_Button_Start", Qt::Key_Return);
    m_keybindings->addAction("Pad0_Button_Select", Qt::Key_Space);
    m_keybindings->addAction("Button_Menu", Qt::Key_Escape);

    m_keybindings->registerMonitoredAction("Button_Menu");
    connect(m_keybindings, SIGNAL(monitoredActionActivated(QString)), this, SLOT(monitoredActionActivated(QString)));
    connect(m_keybindings, SIGNAL(monitoredActionDeactivated(QString)), this, SLOT(monitoredActionDeactivated(QString)));
}

/*! Synchronizes input values from host to the emulation. */
void HostInput::sync()
{
    EmuInput *emuInput = m_emu->input();
    memset32(emuInput, 0, sizeof(EmuInput)/4);

    int buttons = 0;

    if (m_keybindings->checkAction("Pad0_Button_Right"))
        buttons += EmuPad::Button_Right;
    if (m_keybindings->checkAction("Pad0_Button_Down"))
        buttons += EmuPad::Button_Down;
    if (m_keybindings->checkAction("Pad0_Button_Up"))
        buttons += EmuPad::Button_Up;
    if (m_keybindings->checkAction("Pad0_Button_Left"))
        buttons += EmuPad::Button_Left;
    if (m_keybindings->checkAction("Pad0_Button_A"))
        buttons += EmuPad::Button_A;
    if (m_keybindings->checkAction("Pad0_Button_B"))
        buttons += EmuPad::Button_B;
    if (m_keybindings->checkAction("Pad0_Button_X"))
        buttons += EmuPad::Button_X;
    if (m_keybindings->checkAction("Pad0_Button_Y"))
        buttons += EmuPad::Button_Y;
    if (m_keybindings->checkAction("Pad0_Button_L1"))
        buttons += EmuPad::Button_L1;
    if (m_keybindings->checkAction("Pad0_Button_R1"))
        buttons += EmuPad::Button_R1;
    if (m_keybindings->checkAction("Pad0_Button_Start"))
        buttons += EmuPad::Button_Start;
    if (m_keybindings->checkAction("Pad0_Button_Select"))
        buttons += EmuPad::Button_Select;

    emuInput->pad[0].setButtons(buttons);

    buttons = 0;

    if (m_keybindings->checkAction("Pad1_Button_Right"))
        buttons += EmuPad::Button_Right;
    if (m_keybindings->checkAction("Pad1_Button_Down"))
        buttons += EmuPad::Button_Down;
    if (m_keybindings->checkAction("Pad1_Button_Up"))
        buttons += EmuPad::Button_Up;
    if (m_keybindings->checkAction("Pad1_Button_Left"))
        buttons += EmuPad::Button_Left;
    if (m_keybindings->checkAction("Pad1_Button_B"))
        buttons += EmuPad::Button_A;
    if (m_keybindings->checkAction("Pad1_Button_A"))
        buttons += EmuPad::Button_B;
    if (m_keybindings->checkAction("Pad1_Button_Y"))
        buttons += EmuPad::Button_X;
    if (m_keybindings->checkAction("Pad1_Button_X"))
        buttons += EmuPad::Button_Y;
    if (m_keybindings->checkAction("Pad1_Button_L1"))
        buttons += EmuPad::Button_L1;
    if (m_keybindings->checkAction("Pad1_Button_R1"))
        buttons += EmuPad::Button_R1;
    if (m_keybindings->checkAction("Pad1_Button_Start"))
        buttons += EmuPad::Button_Start;
    if (m_keybindings->checkAction("Pad1_Button_Select"))
        buttons += EmuPad::Button_Select;

    emuInput->pad[1].setButtons(buttons);

}

bool HostInput::menuEnabled()
{
    return m_menuEnabled;
}

void HostInput::setMenuEnabled(bool enabled)
{
    if(m_menuEnabled == enabled)
        return;

    m_menuEnabled = enabled;

    if (m_menuEnabled) {
        m_keybindings->registerMonitoredAction("Pad0_Button_Right");
        m_keybindings->registerMonitoredAction("Pad0_Button_Left");
        m_keybindings->registerMonitoredAction("Pad0_Button_Up");
        m_keybindings->registerMonitoredAction("Pad0_Button_Down");
        m_keybindings->registerMonitoredAction("Pad0_Button_Start");
        m_keybindings->registerMonitoredAction("Pad0_Button_A");
        m_keybindings->registerMonitoredAction("Pad0_Button_B");
    } else {
        m_keybindings->deregisterMonitoredAction("Pad0_Button_Right");
        m_keybindings->deregisterMonitoredAction("Pad0_Button_Left");
        m_keybindings->deregisterMonitoredAction("Pad0_Button_Up");
        m_keybindings->deregisterMonitoredAction("Pad0_Button_Down");
        m_keybindings->deregisterMonitoredAction("Pad0_Button_Start");
        m_keybindings->deregisterMonitoredAction("Pad0_Button_A");
        m_keybindings->deregisterMonitoredAction("Pad0_Button_B");
    }

    emit menuEnabledChanged(m_menuEnabled);
}

/*! Loads function in the emulation for each input device.  */
void HostInput::loadFromConf()
{

}

void HostInput::monitoredActionActivated(const QString &action)
{
    if (action == "Button_Menu") {
        //Menu Action activated
        return;
    }

    if(m_menuEnabled) {
        if (action == "Pad0_Button_Right") {
            qGuiApp->sendEvent(m_window, new QKeyEvent(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier));
        } else if (action == "Pad0_Button_Left") {
            qGuiApp->sendEvent(m_window, new QKeyEvent(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier));
        } else if (action == "Pad0_Button_Up") {
            qGuiApp->sendEvent(m_window, new QKeyEvent(QEvent::KeyPress, Qt::Key_Up, Qt::NoModifier));
        } else if (action == "Pad0_Button_Down") {
            qGuiApp->sendEvent(m_window, new QKeyEvent(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier));
        } else if (action == "Pad0_Button_Start") {
            qGuiApp->sendEvent(m_window, new QKeyEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier));
        } else if (action == "Pad0_Button_A") {
            qGuiApp->sendEvent(m_window, new QKeyEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier));
        } else if (action == "Pad0_Button_B") {
            qGuiApp->sendEvent(m_window, new QKeyEvent(QEvent::KeyPress, Qt::Key_Backspace, Qt::NoModifier));
        }
    }
}

void HostInput::monitoredActionDeactivated(const QString &action)
{
    if (action == "Button_Menu") {
        //Menu Action deactivated
        if (m_menuEnabled)
            setMenuEnabled(false);
        else
            setMenuEnabled(true);
        return;
    }

    if(m_menuEnabled) {
        if (action == "Pad0_Button_Right") {
            qGuiApp->sendEvent(m_window, new QKeyEvent(QEvent::KeyRelease, Qt::Key_Right, Qt::NoModifier));
        } else if (action == "Pad0_Button_Left") {
            qGuiApp->sendEvent(m_window, new QKeyEvent(QEvent::KeyRelease, Qt::Key_Left, Qt::NoModifier));
        } else if (action == "Pad0_Button_Up") {
            qGuiApp->sendEvent(m_window, new QKeyEvent(QEvent::KeyRelease, Qt::Key_Up, Qt::NoModifier));
        } else if (action == "Pad0_Button_Down") {
            qGuiApp->sendEvent(m_window, new QKeyEvent(QEvent::KeyRelease, Qt::Key_Down, Qt::NoModifier));
        } else if (action == "Pad0_Button_Start") {
            qGuiApp->sendEvent(m_window, new QKeyEvent(QEvent::KeyRelease, Qt::Key_Return, Qt::NoModifier));
        } else if (action == "Pad0_Button_A") {
            qGuiApp->sendEvent(m_window, new QKeyEvent(QEvent::KeyRelease, Qt::Key_Return, Qt::NoModifier));
        } else if (action == "Pad0_Button_B") {
            qGuiApp->sendEvent(m_window, new QKeyEvent(QEvent::KeyRelease, Qt::Key_Backspace, Qt::NoModifier));
        }
    }
}
