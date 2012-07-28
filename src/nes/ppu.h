/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef NESPPU_H
#define NESPPU_H

class NesPpu;
#include "ppusprite.h"
#include "ppuscroll.h"
#include <base/emu.h>
#include <QtGui/QImage>
#include <QtCore/QVector>
#include <QtGui/QRgb>
#include <QtCore/QStringList>

class NesPpu;

extern QImage nesPpuFrame;
extern NesPpu nesPpu;
extern NesPpuScroll nesPpuScroll;
extern u16 nesPpuTilePageOffset;
extern u8 nesPpuRegs[8];
extern int nesPpuScanline;

class NesPpu : public QObject
{
	Q_OBJECT
	Q_PROPERTY(bool spriteLimit READ spriteLimit WRITE setSpriteLimit NOTIFY spriteLimitChanged)
public:
	enum ChipType {
		PPU2C02,	// NTSC NES
		PPU2C03B,	// Playchoice 10
		PPU2C04,	// Vs. Unisystem
		PPU2C05_01,	// Vs. Unisystem (Ninja Jajamaru Kun)
		PPU2C05_02,	// Vs. Unisystem (Mighty Bomb Jack)
		PPU2C05_03,	// Vs. Unisystem (Gumshoe)
		PPU2C05_04,	// Vs. Unisystem (Top Gun)
		PPU2C07		// PAL NES
	};
	enum Register {
		Control0 = 0,
		Control1 = 1,
		Status = 2,
		SpriteRAMAddress = 3,
		SpriteRAMIO = 4,
		Scroll = 5,
		VRAMAddress = 6,
		VRAMIO = 7
	};
	enum ControlReg0Bit {
		//	Indicates whether a NMI should occur upon V-Blank.
		VBlankEnableCR0Bit = 0x80,
		//	Specifies the size of sprites in pixels 8x8 (0) or 8x16(1).
		SpriteSizeCR0Bit = 0x20,
		/* Identifies which pattern table the background is
		 * stored in, either $0000 (0) or $1000 (1). */
		BackgroundTableCR0Bit = 0x10,
		/* Identifies which pattern table sprites are stored in,
		 * either $0000 (0) or $1000 (1).*/
		SpriteTableCR0Bit = 0x08,
		/* Specifies amount to increment address by, either 1 if
		 * this is 0 or 32 if this is 1. */
		IncrementCR0Bit = 0x04,
		/* Name table address, changes between the four
		 * name tables at $2000 (0), $2400 (1), $2800 (2) and $2C00 (3). */
		NameTableAddressMaskCR0Bit = 3
	};

	enum ControlReg1Bit {
		/* Indicates background colour in monochrome
		 * mode or colour intensity in colour mode. */
		BackgroundColorCR1Bit = 0xE0,
		//	If this is 0, sprites should not be displayed.
		SpriteDisplayCR1Bit = 0x10,
		//	If this is 0, the background should not be displayed.
		BackgroundDisplayCR1Bit = 0x08,
		/* Specifies whether to clip the sprites, that is whether
		 * to hide sprites in the left 8 pixels on screen (0) or to show
		 * them (1). */
		SpriteClipDisableCR1Bit = 0x04,
		/* Specifies whether to clip the background, that is
		 * whether to hide the background in the left 8 pixels on
		 * screen (0) or to show them (1). */
		BackgroundClipDisableCR1Bit = 0x02,
		/* Indicates whether the system is in colour (0) or
		 * monochrome mode (1) */
		MonochromeModeCR1Bit = 0x01
	};

	enum StatusRegBit {
		//	Indicates whether V-Blank is occurring.
		VBlankSRBit = 0x80,
		/* Sprite 0 hit flag, set when a non-transparent pixel of
		 * sprite 0 overlaps a non-transparent background pixel. */
		Sprite0HitSRBit = 0x40,
		/* Scanline sprite count, if set, indicates more than 8
		 * sprites on the current scanline. */
		SpriteMaxSRBit = 0x20,
		//	If set, indicates that writes to VRAM should be ignored.
		DisableVRAMWriteSRBit = 0x10
	};

	static const uint NameTableOffset = 0x2000;
	static const uint AttributeTableOffset = 0x03C0;
	static const uint PalettesAddress = 0x3F00;

	static const int VisibleScreenWidth = 32 * 8;
	static const int VisibleScreenHeight = 30 * 8;

	static const int FetchCycles = 2;
	static const int ScanlineCycles = 341;
	static const int HDrawCycles = 256;
	static const int HBlankCycles = 85;
	static const int ScanlineEndCycles = 1;

	void setSpriteLimit(bool on);
	bool spriteLimit() const;
	int scanlineCount() const;
signals:
	void spriteLimitChanged();
};

static inline bool nesPpuIsBackgroundVisible()
{ return nesPpuRegs[NesPpu::Control1] & NesPpu::BackgroundDisplayCR1Bit; }
static inline bool nesPpuIsSpriteVisible()
{ return nesPpuRegs[NesPpu::Control1] & NesPpu::SpriteDisplayCR1Bit; }
static inline bool nesPpuIsDisplayOn()
{ return nesPpuIsBackgroundVisible() || nesPpuIsSpriteVisible(); }

extern void nesPpuInit();
extern void nesPpuWriteReg(u16 addr, u8 data);
extern   u8 nesPpuReadReg(u16 addr);
extern void nesPpuDma(u8 page);
extern void nesPpuSetVBlank(bool on);

extern  int nesPpuNextScanline();
extern void nesPpuProcessFrameStart();
extern void nesPpuProcessScanlineStart();
extern void nesPpuProcessScanlineNext();
extern void nesPpuProcessDummyScanline();

extern bool nesPpuDrawPrologue();
extern void nesPpuDrawEpilogue();
extern void nesPpuDrawBackgroundTile(int i);
extern void nesPpuDrawBackgroundLine();

extern bool nesPpuCheckSprite0HitHere();

extern QRgb nesPpuGetPixel(int x, int y);

extern void nesPpuSl();

#endif // NESPPU_H
