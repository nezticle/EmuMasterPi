DESTDIR = ../../bin
LIBS += -L../../lib -lbase -lBsquaskCommon
INCLUDEPATH += ..

unix {
	QMAKE_LFLAGS += -Wl,--rpath-link,../../lib -Wl,--rpath,/opt/emumaster/bin -Wl,--rpath,/opt/emumaster/lib
	target.path = /opt/emumaster/bin

        INSTALLS += target
}
