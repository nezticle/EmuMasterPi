# EmuMasterPi
 
NES, SNES, and GBA emulators for RaspberryPi using Qt 5.

This project is a port of EmuMaster for the Nokia N9 to the Raspberry Pi.


**Requirements:**   
1. [qtbase](http://qt.gitorious.org/qt/qtbase)  
2. [qtmultimedia](http://qt.gitorious.org/qt/qtmultimedia)  
3. [qtgamepad](https://github.com/nezticle/qtgamepad)

###Usage

Binaries are installed in:  
`/opt/apps/emumaster/bin/`  

If you wanted to run an SNES game:  
`/opt/apps/emumaster/bin/snes MyGameName.smc`  

Roms need to be installed in:   
`~/roms/snes/`  
`~/roms/nes/`  
`~/roms/gba/`  

These emulators are inteded to be played using a USB Gamepad like the USB Xbox controller.

The GBA emulator requires a Gameboy Advance BIOS which will be exactly 16384 bytes large and should have the following md5sum value:  
`a860e8c0b6d573d191e4ec7db1b1e4f6`  
When you do get it name it gba_bios.bin and put it in the directory:  
`/roms/gba/`  