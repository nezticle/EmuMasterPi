/*
 * Copyright (c) 2012 Andy Nichols
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "gamepadinputdevice.h"

GamepadInputDevice::GamepadInputDevice(QObject *parent)
    : HostInputDevice("gamepad", QObject::tr("Gamepad"), parent)
{
    m_manager = new QGamepadManager(this);
    m_inputState = new QGamepadInputState(this);

    connect(m_manager, SIGNAL(gamepadEvent(QGamepadInfo*,quint64,int,int,int)),
            m_inputState, SLOT(processGamepadEvent(QGamepadInfo*,quint64,int,int,int)));
    connect(this, SIGNAL(emuFunctionChanged()), SLOT(onEmuFunctionChanged()));

    setupEmuFunctionList();

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
}

void GamepadInputDevice::setupEmuFunctionList()
{
    QStringList functionNameList;
    functionNameList << tr("None")
                     << tr("Pad A")
                     << tr("Pad B");
    setEmuFunctionNameList(functionNameList);
}

int GamepadInputDevice::buttons()
{
    int buttons = 0;

    if (m_inputState->queryGamepadButton(QGamepadInputState::Gamepad_Right1))
        buttons += EmuPad::Button_Right;
    if (m_inputState->queryGamepadButton(QGamepadInputState::Gamepad_Down1))
        buttons += EmuPad::Button_Down;
    if (m_inputState->queryGamepadButton(QGamepadInputState::Gamepad_Up1))
        buttons += EmuPad::Button_Up;
    if (m_inputState->queryGamepadButton(QGamepadInputState::Gamepad_Left1))
        buttons += EmuPad::Button_Left;
    if (m_inputState->queryGamepadButton(QGamepadInputState::Gamepad_B))
        buttons += EmuPad::Button_A;
    if (m_inputState->queryGamepadButton(QGamepadInputState::Gamepad_A))
        buttons += EmuPad::Button_B;
    if (m_inputState->queryGamepadButton(QGamepadInputState::Gamepad_Y))
        buttons += EmuPad::Button_X;
    if (m_inputState->queryGamepadButton(QGamepadInputState::Gamepad_X))
        buttons += EmuPad::Button_Y;
    if (m_inputState->queryGamepadButton(QGamepadInputState::Gamepad_TL1))
        buttons += EmuPad::Button_L1;
    if (m_inputState->queryGamepadButton(QGamepadInputState::Gamepad_TR1))
        buttons += EmuPad::Button_R1;
    if (m_inputState->queryGamepadButton(QGamepadInputState::Gamepad_Start))
        buttons += EmuPad::Button_Start;
    if (m_inputState->queryGamepadButton(QGamepadInputState::Gamepad_Select))
        buttons += EmuPad::Button_Select;
    if (m_inputState->queryGamepadButton(QGamepadInputState::Gamepad_Mode))
        buttons += EmuPad::Button_Mode;

    return buttons;
}
