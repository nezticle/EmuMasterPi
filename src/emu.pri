DESTDIR = ../../bin
INCLUDEPATH += ..

QT = core gui multimedia gamepad quick

HEADERS += \
    ../base/hostaudio.h \
    ../base/hostvideo.h \
    ../base/hostinput.h \
    ../base/pathmanager.h \
    ../base/statelistmodel.h \
    ../base/configuration.h \
    ../base/hostinputdevice.h \
    ../base/keybinputdevice.h \
    ../base/crc32.h \
    ../base/emuview.h \
    ../base/emu.h \
    ../base/emuinput.h \
    ../base/stringlistproxy.h \
    ../base/audioringbuffer.h \
    ../base/memutils.h \
    ../base/gamepadinputdevice.h

SOURCES += \
    ../base/hostaudio.cpp \
    ../base/hostvideo.cpp \
    ../base/hostinput.cpp \
    ../base/pathmanager.cpp \
    ../base/statelistmodel.cpp \
    ../base/configuration.cpp \
    ../base/hostinputdevice.cpp \
    ../base/keybinputdevice.cpp \
    ../base/crc32.cpp \
    ../base/emuview.cpp \
    ../base/emu.cpp \
    ../base/emuinput.cpp \
    ../base/stringlistproxy.cpp \
    ../base/memutils.cpp \
    ../base/gamepadinputdevice.cpp

OTHER_FILES += \
    ../../data/qml/main.qml

linux-rasp-pi-g++ {
        target.path = /opt/apps/emumaster/bin

        shaders.path = /opt/apps/emumaster/data/shader
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
        ../../data/shader/sharpen.fsh
        qml.path = /opt/apps/emumaster/data/qml
        qml.files = $$OTHER_FILES

        INSTALLS += target shaders qml
}
