TEMPLATE = lib
DESTDIR = ../../lib
INCLUDEPATH += ..
QT = core gui multimedia
LIBS += -L../../lib -lBsquaskRenderer

unix {
    QMAKE_LFLAGS += -Wl,--rpath,/opt/emumaster/lib
    target.path = /opt/emumaster/lib
    INSTALLS += target
}

DEFINES += BASE_PROJECT BSQUASK_ENABLE_JOYSTICKS

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

unix {

    shaders.path = /opt/emumaster/data/shader
    shaders.files = \
        ../../data/shader/hq2x.vsh \
        ../../data/shader/hq2x.fsh \
        ../../data/shader/hq4x.vsh \
        ../../data/shader/hq4x.fsh \
        ../../data/shader/2xSal.vsh \
        ../../data/shader/2xSal.fsh \
        ../../data/shader/grayScale.vsh \
        ../../data/shader/grayScale.fsh \
        ../../data/shader/sharpen.vsh \
        ../../data/shader/sharpen.fsh \


    INSTALLS += shaders
}
