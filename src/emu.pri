DESTDIR = ../../bin
LIBS += -L../../lib -lbase
INCLUDEPATH += ..

QT = core gui multimedia gamepad

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
        ../../data/shader/sharpen.fsh \

        INSTALLS += target shaders
}
