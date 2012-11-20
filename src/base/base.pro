TEMPLATE = lib
QT = core gui multimedia gamepad
CONFIG += staticlib
INCLUDEPATH += ..
DESTDIR = ../../lib

DEFINES += BASE_PROJECT

HEADERS += \
    hostaudio.h \
    hostvideo.h \
    hostinput.h \
    base_global.h \
    pathmanager.h \
    statelistmodel.h \
    configuration.h \
    hostinputdevice.h \
    keybinputdevice.h \
    crc32.h \
    emuview.h \
    emuthread.h \
    emu.h \
    emuinput.h \
    stringlistproxy.h \
    audioringbuffer.h \
    memutils.h \
    gamepadinputdevice.h

SOURCES += \
    hostaudio.cpp \
    hostvideo.cpp \
    hostinput.cpp \
    pathmanager.cpp \
    statelistmodel.cpp \
    configuration.cpp \
    hostinputdevice.cpp \
    keybinputdevice.cpp \
    crc32.cpp \
    emuview.cpp \
    emuthread.cpp \
    emu.cpp \
    emuinput.cpp \
    stringlistproxy.cpp \
    memutils.cpp \
    gamepadinputdevice.cpp
