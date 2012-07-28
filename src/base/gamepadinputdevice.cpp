#include "gamepadinputdevice.h"

GamepadInputDevice::GamepadInputDevice(QObject *parent)
    : HostInputDevice("gamepad", QObject::tr("Gamepad"), parent)
    , m_manager(new Bsquask::JoystickManager(this))
    , m_inputState(new Bsquask::InputState(this))
//    , m_keymap(m_inputState)
{


    connect(m_manager, SIGNAL(joystickEvent(quint64,int,int,int)),
            m_inputState, SLOT(processJoystickEvent(quint64,int,int,int)));
    connect(this, SIGNAL(emuFunctionChanged()), SLOT(onEmuFunctionChanged()));
    setupEmuFunctionList();
    setupKeybindings();

    setEmuFunction(1);
}

void GamepadInputDevice::sync(EmuInput *emuInput)
{
    if (emuFunction() <= 0)
        return;

    if (emuFunction() == 1)
        emuInput->pad[0].setButtons(buttons());
    else if (emuFunction() == 2)
        emuInput->pad[1].setButtons(buttons());
}

void GamepadInputDevice::onEmuFunctionChanged()
{
    //Clean inputstate && keymap
    //m_keymap.reset();
}

void GamepadInputDevice::setupEmuFunctionList()
{
    QStringList functionNameList;
    functionNameList << tr("None")
                     << tr("Pad A")
                     << tr("Pad B");
    setEmuFunctionNameList(functionNameList);
}

void GamepadInputDevice::setupKeybindings()
{
//    m_keymap.addAction("Right", Bsquask::InputState::Gamepad_Right1);
//    m_keymap.addAction("Down", Bsquask::InputState::Gamepad_Down1);
//    m_keymap.addAction("Up", Bsquask::InputState::Gamepad_Up1);
//    m_keymap.addAction("Left", Bsquask::InputState::Gamepad_Left1);

//    //Reversed for Nintendo (from xbox gamepad)
//    m_keymap.addAction("A", Bsquask::InputState::Gamepad_B);
//    m_keymap.addAction("B", Bsquask::InputState::Gamepad_A);
//    m_keymap.addAction("X", Bsquask::InputState::Gamepad_Y);
//    m_keymap.addAction("Y", Bsquask::InputState::Gamepad_X);

//    m_keymap.addAction("L1", Bsquask::InputState::Gamepad_Left1);
//    m_keymap.addAction("R1", Bsquask::InputState::Gamepad_Right1);

//    m_keymap.addAction("Start", Bsquask::InputState::Gamepad_Start);
//    m_keymap.addAction("Select", Bsquask::InputState::Gamepad_Select);
}

int GamepadInputDevice::buttons()
{
    int buttons = 0;

    if (m_inputState->queryJoystickButton(Bsquask::InputState::Gamepad_Right1))
        buttons += EmuPad::Button_Right;
    if (m_inputState->queryJoystickButton(Bsquask::InputState::Gamepad_Down1))
        buttons += EmuPad::Button_Down;
    if (m_inputState->queryJoystickButton(Bsquask::InputState::Gamepad_Up1))
        buttons += EmuPad::Button_Up;
    if (m_inputState->queryJoystickButton(Bsquask::InputState::Gamepad_Left1))
        buttons += EmuPad::Button_Left;
    if (m_inputState->queryJoystickButton(Bsquask::InputState::Gamepad_B))
        buttons += EmuPad::Button_A;
    if (m_inputState->queryJoystickButton(Bsquask::InputState::Gamepad_A))
        buttons += EmuPad::Button_B;
    if (m_inputState->queryJoystickButton(Bsquask::InputState::Gamepad_Y))
        buttons += EmuPad::Button_X;
    if (m_inputState->queryJoystickButton(Bsquask::InputState::Gamepad_X))
        buttons += EmuPad::Button_Y;
    if (m_inputState->queryJoystickButton(Bsquask::InputState::Gamepad_TL1))
        buttons += EmuPad::Button_L1;
    if (m_inputState->queryJoystickButton(Bsquask::InputState::Gamepad_TR1))
        buttons += EmuPad::Button_R1;
    if (m_inputState->queryJoystickButton(Bsquask::InputState::Gamepad_Start))
        buttons += EmuPad::Button_Start;
    if (m_inputState->queryJoystickButton(Bsquask::InputState::Gamepad_Select))
        buttons += EmuPad::Button_Select;
    if (m_inputState->queryJoystickButton(Bsquask::InputState::Gamepad_Mode))
        buttons += EmuPad::Button_Mode;

    //qDebug("Buttons: %d", buttons);

    return buttons;
}
