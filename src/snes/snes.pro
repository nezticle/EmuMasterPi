include(../emu.pri)

ASM_SOURCES = \
	os9x_65c816.S \
	spc_decode.S \
	spc700a.S \
	m3d_func.S \
	generatePPUasm/ppuasmfunc16.S

SOURCES += $$ASM_SOURCES \
    dsp1.cpp \
    cheats2.cpp \
    cheats.cpp \
    spc700.cpp \
    mem.cpp \
    spu.cpp \
    snes.cpp

HEADERS += \
    sdd1emu.h \
    snapshot.h \
    port.h \
    fxinst.h \
    fxemu.h \
    os9x_asm_cpu.h \
    ppu.h \
    tile.h \
    srtc.h \
    spc700.h \
    snes9x.h \
    soundux.h \
    seta.h \
    gfx.h \
    dma.h \
    c4.h \
    sdd1.h \
    dsp1.h \
    cheats.h \
    cpu.h \
    missing.h \
    65c816.h \
    sa1.h \
    sar.h \
    pixform.h \
    messages.h \
    getset.h \
    display.h \
    debug.h \
    spu.h \
    mem.h \
    snes.h

SOURCES += \
    sdd1emu.cpp \
    fxemu.cpp \
    fxinst.cpp \
    os9x_asm_cpu.cpp \
    ppu.cpp \
    globals.cpp \
    tile.cpp \
    srtc.cpp \
    soundux.cpp \
    seta010.cpp \
    seta011.cpp \
    seta018.cpp \
    seta.cpp \
    gfx.cpp \
    dma.cpp \
    data.cpp \
    cpu.cpp \
    clip.cpp \
    c4emu.cpp \
    c4.cpp \
    sdd1.cpp
