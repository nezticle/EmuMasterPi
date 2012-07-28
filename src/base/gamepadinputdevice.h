#ifndef GAMEPADINPUTDEVICE_H
#define GAMEPADINPUTDEVICE_H

#include "hostinputdevice.h"
#include <Bsquask/joystick/joystickmanager.h>
#include <Bsquask/inputstate.h>
#include <Bsquask/keymap.h>

class BASE_EXPORT GamepadInputDevice : public HostInputDevice
{
    Q_OBJECT
public:
    explicit GamepadInputDevice(QObject *parent = 0);
    void sync(EmuInput *emuInput);

private slots:
    void onEmuFunctionChanged();
private:
    void setupEmuFunctionList();
    void setupKeybindings();
    int buttons();

private:
    Bsquask::JoystickManager *m_manager;
    Bsquask::InputState *m_inputState;
    //Bsquask::Keymap m_keymap;

};

#endif // GAMEPADINPUTDEVICE_H
