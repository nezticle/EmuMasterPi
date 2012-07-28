include(../emu.pri)

DEFINES += USE_ASM_VIDEO_EXPAND
QMAKE_CFLAGS += -std=c99

SOURCES += \
    arm/video_blend.S \
    arm/arm_stub.S \
    gba.cpp \
    input.cpp

HEADERS += \
    arm/arm_emit.h \
    arm/arm_dpimacros.h \
    arm/arm_codegen.h \
    cpu.h \
    common.h \
    cheats.h \
    mem.h \
    spu.h \
    gpu.h \
    gba.h \
    input.h

SOURCES += \
    cheats.cpp \
    cpu.cpp \
    cpu_threaded.c \
    mem.cpp \
    spu.cpp \
    gpu.cpp

unix {
	game_config.path = /opt/emumaster/data
	game_config.files = gba_game_config.txt
	INSTALLS += game_config
}
