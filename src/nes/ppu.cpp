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

#include "ppu.h"
#include "mapper.h"
#include "nes.h"
#include "cpubase.h"
#include "sync_p.h"
#include <arm/constants.h>
#include <base/configuration.h>
#include <Qt/qmath.h>

// default content of paletter memory
static const u8 paletteDefaultMem[] =
{
	0,	1,	2,	3,
	0,	5,	6,	7,
	0,	9,	10,	11,
	0,	13,	14,	15,
	0,	17,	18,	19,
	0,	21,	22,	23,
	0,	25,	26,	27,
	0,	29,	30,	31
};

static const char *spriteLimitConfName = "nes.ppu.spriteLimit";

QImage nesPpuFrame;
NesPpu nesPpu;
NesPpuScroll nesPpuScroll;

u8 nesPpuRegs[8]; // registers at 0x2000-0x2007
static u8 dataLatch;
static u8 bufferedData;
static u8 securityValue;

int nesPpuScanline;
static QRgb *scanlineData;

bool vBlankOut;

u16 nesPpuTilePageOffset;
static u16 spritePageOffset;
static u16 scrollAddressIncrement;
static uint spriteSize;

static u8 bgWritten[33+3];// 3 bytes pad
static u8 bit2Rev[256];

static const int NumSprites = 64;
static u8 spriteMem[NumSprites*4];

static u8 paletteMem[32];
static QRgb palettePens[512];
static QRgb palettePenLut[32];
static u32 paletteMask;
static u32 paletteEmphasis;
static bool palettePenLutNeedsRebuild;

static bool ppuSpriteLimit;

static void fillPens();


/*!
	Builds a lookup table for current palette mask and emphasis. It is used
	for drawing speedup.
 */
static void buildPenLut()
{
	for (int i = 0; i < 32; i++) {
		palettePenLut[i] = palettePens[(paletteMem[i] & paletteMask)
				+ paletteEmphasis];
	}
}

/*!
	Updates palette mask and emphasis basing on PPU registers.
 */
static void updateColorEmphasisAndMask()
{
	u32 newEmphasis = u32(nesPpuRegs[NesPpu::Control1] & NesPpu::BackgroundColorCR1Bit);
	if (newEmphasis != 0x20 && newEmphasis != 0x40 && newEmphasis != 0x80)
		newEmphasis = 0x00;
	newEmphasis *= 2;
	u32 newMask = ((nesPpuRegs[NesPpu::Control1] & NesPpu::MonochromeModeCR1Bit) ? 0xf0 : 0xff);
	if (newEmphasis != paletteEmphasis || newMask != paletteMask) {
		paletteEmphasis = newEmphasis;
		paletteMask = newMask;
		palettePenLutNeedsRebuild = true;
	}
}

/*!
	Updates cached data of PPU registers.
 */
static inline void updateCachedControl0Bits()
{
	// the char ram bank points either 0x0000 or 0x1000 (page 0 or page 4)
	u8 data = nesPpuRegs[NesPpu::Control0];
	nesPpuTilePageOffset = (data & NesPpu::BackgroundTableCR0Bit) << 8;
	spritePageOffset = (data & NesPpu::SpriteTableCR0Bit) << 9;
	scrollAddressIncrement = ((data & NesPpu::IncrementCR0Bit) ? 32 : 1);
	spriteSize = ((data & NesPpu::SpriteSizeCR0Bit) ? 16 : 8);
}

/*!
	Sets desired chip \a type.
 */
static void setChipType(NesPpu::ChipType type)
{
	switch (type) {
	case NesPpu::PPU2C05_01:	securityValue = 0x1b; break;
	case NesPpu::PPU2C05_02:	securityValue = 0x3d; break;
	case NesPpu::PPU2C05_03:	securityValue = 0x1c; break;
	case NesPpu::PPU2C05_04:	securityValue = 0x1b; break;
	default:					securityValue = 0x00; break;
	}
}

/*!
	Initializes PPU emulation.
 */
void nesPpuInit()
{
	nesPpuFrame = QImage(8+NesPpu::VisibleScreenWidth+8,
						 NesPpu::VisibleScreenHeight,
						 QImage::Format_RGB32);

	memset(nesPpuRegs, 0, sizeof(nesPpuRegs));
	memset(spriteMem, 0, sizeof(spriteMem));
	updateCachedControl0Bits();

	securityValue = 0;
	dataLatch = 0;
	bufferedData = 0xff;

	nesPpuScanline = 0;
	scanlineData = 0;

	vBlankOut = false;

	nesPpuScroll.address = 0;
	nesPpuScroll.latch = 0;
	nesPpuScroll.toggle = 0;
	nesPpuScroll.xFine = 0;

	for (int i = 0; i < 256; i++) {
		u8 m = 0x80;
		u8 c = 0;
		for (int j = 0; j < 8; j++) {
			if (i & (1<<j))
				c |= m;
			m >>= 1;
		}
		bit2Rev[i] = c;
	}

	if (nesSystemType == NES_PAL)
		setChipType(NesPpu::PPU2C07);
	else
		setChipType(NesPpu::PPU2C02);

	fillPens();
	memcpy(paletteMem, paletteDefaultMem, sizeof(paletteMem));
	updateColorEmphasisAndMask();

	ppuSpriteLimit = emConf.value(spriteLimitConfName, true).toBool();
}

/*!
	The function handles writes to palette memory. It marks palette lookup table
	dirty if needed.
 */
static void writePalette(u16 addr, u8 data)
{
	Q_ASSERT(addr < 32);
	data &= 0x3f;
	if (!(addr & 0x03)) {
		if (!(addr & 0x0f)) {
			if (paletteMem[0] != data) {
				for (int i = 0; i < 32; i += 4)
					paletteMem[i] = data;
				palettePenLutNeedsRebuild = true;
			}
		}
	} else {
		if (paletteMem[addr] != data) {
			paletteMem[addr] = data;
			palettePenLutNeedsRebuild = true;
		}
	}
}

/*!
	The function handles reads from palette memory.
 */
static u8 readPalette(u16 address)
{
	Q_ASSERT(address < 32);
	return paletteMem[address] & paletteMask;
}

/*!
	The function invokes rebuilding of palette lookup table if needed
	and returns this table as result.
 */
static QRgb *currentPens()
{
	if (palettePenLutNeedsRebuild) {
		buildPenLut();
		palettePenLutNeedsRebuild = false;
	}
	return palettePenLut;
}

/*!
	The function updates value of the Vertical Blank signal. Even if we are
	currently in vertical blank area the signal can be masked by the value
	in the control register.
 */
static void updateVBlankOut()
{
	bool vBlankEnabled = (nesPpuRegs[NesPpu::Control0] & NesPpu::VBlankEnableCR0Bit);
	bool vBlankState = (nesPpuRegs[NesPpu::Status] & NesPpu::VBlankSRBit);
	bool newVBlank = (vBlankEnabled && vBlankState);
	if (newVBlank != vBlankOut) {
		vBlankOut = newVBlank;
		nesCpu->setSignal(NesCpuBase::NmiSignal, vBlankOut);
	}
}

/*!
	The function handles writes to PPU registers.
 */
void nesPpuWriteReg(u16 addr, u8 data)
{
	Q_ASSERT(addr < 8);
	if (securityValue && !(addr & 6))
		addr ^= 1;
	nesPpuRegs[addr] = data;
	switch (static_cast<NesPpu::Register>(addr)) {
	case NesPpu::Control0:
		updateVBlankOut();
		// update the name table number on our refresh latches
		nesPpuScroll.latch &= ~(3 << 10);
		nesPpuScroll.latch |= (data & 3) << 10;
		updateCachedControl0Bits();
		break;
	case NesPpu::Control1:
		updateColorEmphasisAndMask();
		break;
	case NesPpu::SpriteRAMAddress:
		break;
	case NesPpu::SpriteRAMIO:
		// if the PPU is currently rendering the screen,
		// 0xff is written instead of the desired data.
		if (nesPpuScanline < NesPpu::VisibleScreenHeight && nesPpuIsSpriteVisible()) {
			data = 0xff;
		} else {
			if ((nesPpuRegs[NesPpu::SpriteRAMAddress] & 0x03) == 0x02)
				data &= 0xe3;
		}
		spriteMem[nesPpuRegs[NesPpu::SpriteRAMAddress]++] = data;
		break;
	case NesPpu::Scroll:
		if (nesPpuScroll.toggle ^= 1) {
			nesPpuScroll.latch &= ~0x1f;
			nesPpuScroll.latch |= (data >> 3) & 0x1f;

			nesPpuScroll.xFine = data & 7;
		} else {
			nesPpuScroll.latch &= ~((0xf8 << 2) | (7 << 12));
			nesPpuScroll.latch |= (data & 0xf8) << 2;
			nesPpuScroll.latch |= (data & 7) << 12;
		}
		break;
	case NesPpu::VRAMAddress:
		if (nesPpuScroll.toggle ^= 1) {
			nesPpuScroll.latch &= 0x00ff;
			nesPpuScroll.latch |= (data & 0x3f) << 8;
		} else {
			nesPpuScroll.latch &= 0x7f00;
			nesPpuScroll.latch |= data;
			nesPpuScroll.address = nesPpuScroll.latch;
		}
		break;
	case NesPpu::VRAMIO: {
		uint vramAddress = nesPpuScroll.address & 0x3fff;
		nesPpuScroll.address = (nesPpuScroll.address+scrollAddressIncrement) & 0x7fff;
		if (vramAddress >= NesPpu::PalettesAddress)
			writePalette(vramAddress & 0x1f, data);
		else if (nesPpuBank1KType(vramAddress >> 10) != VromBank)
			nesPpuWrite(vramAddress, data);
		break;
	}
	default:
		break;
	}
	dataLatch = data;
}

/*!
	The function handles reads from PPU registers.
 */
u8 nesPpuReadReg(u16 addr)
{
	Q_ASSERT(addr < 8);
	switch (static_cast<NesPpu::Register>(addr)) {
	case NesPpu::Status:
		// the top 3 bits of the status register are the only ones that report data. The
		// remainder contain whatever was last in the PPU data latch, except on the RC2C05 (protection)
		if (securityValue) {
			dataLatch = nesPpuRegs[NesPpu::Status] & (NesPpu::VBlankSRBit|NesPpu::Sprite0HitSRBit);
			dataLatch |= securityValue;
		} else {
			dataLatch = nesPpuRegs[NesPpu::Status] | (dataLatch & 0x1f);
		}
		// reset hi/lo scroll toggle
		nesPpuScroll.toggle = 0;
		// if the vblank bit is set, clear all status bits but the 2 sprite flags
		if (dataLatch & NesPpu::VBlankSRBit) {
			nesPpuRegs[NesPpu::Status] &= ~NesPpu::VBlankSRBit;
			updateVBlankOut();
		}
		break;
	case NesPpu::SpriteRAMIO:
		dataLatch = spriteMem[nesPpuRegs[NesPpu::SpriteRAMAddress]];
		break;
	case NesPpu::VRAMIO: {
		u16 vramAddress = nesPpuScroll.address & 0x3fff;
		nesPpuScroll.address = (nesPpuScroll.address+scrollAddressIncrement) & 0x7fff;
		if (vramAddress >= NesPpu::PalettesAddress)
			return readPalette(vramAddress & 0x1f);
		else
			dataLatch = bufferedData;
		bufferedData = nesPpuRead(vramAddress);
		break;
	}
	default:
		break;
	}
	return dataLatch;
}

void nesPpuSetVBlank(bool on)
{
	if (on)
		nesPpuRegs[NesPpu::Status] |= NesPpu::VBlankSRBit;
	else
		nesPpuRegs[NesPpu::Status] &= ~(NesPpu::VBlankSRBit|NesPpu::Sprite0HitSRBit);
	updateVBlankOut();
}

static inline void setSpriteMax(bool on)
{
	if (on)
		nesPpuRegs[NesPpu::Status] |= NesPpu::SpriteMaxSRBit;
	else
		nesPpuRegs[NesPpu::Status] &= ~NesPpu::SpriteMaxSRBit;
}

int nesPpuNextScanline()
{
	nesPpuScanline++;
	scanlineData += nesPpuFrame.bytesPerLine()/sizeof(QRgb);
	return nesPpuScanline;
}

void nesPpuDma(u8 page)
{
	u16 address = page << 8;
	u8 *p = spriteMem;
	for (int i = 0; i < 256; i++, p++)
		 *p = nesCpuRead(address + i);
}

void nesPpuProcessFrameStart()
{
	nesPpuScanline = 0;
	scanlineData = (QRgb *)nesPpuFrame.bits();
	if (nesPpuIsDisplayOn())
		nesPpuScroll.address = nesPpuScroll.latch;
}

static inline u16 fetchNameAddress()
{
	return NesPpu::NameTableOffset | (nesPpuScroll.address & 0x0fff);
}

void nesPpuProcessScanlineStart()
{
	if (nesPpuIsDisplayOn())
		nesPpuScroll.resetX();
}

void nesPpuProcessScanlineNext()
{
	if (nesPpuIsDisplayOn())
		nesPpuScroll.clockY();
}

static void fillScanline(int color, int count)
{
	QRgb pen = currentPens()[color];
	for (int i = 0; i < count; i++)
		scanlineData[i] = pen;
}

static inline bool sprite0HitOccurred()
{
	return nesPpuRegs[NesPpu::Status] & NesPpu::Sprite0HitSRBit;
}

static void drawSprites()
{
	setSpriteMax(false);
	if (nesPpuScanline >= NesPpu::VisibleScreenHeight || !nesPpuIsSpriteVisible())
		return;

	u8 spWritten[33+3]; // 3 bytes pad
	memset(spWritten, 0, sizeof(spWritten));
	if (!(nesPpuRegs[NesPpu::Control1] & NesPpu::SpriteClipDisableCR1Bit))
		spWritten[0] = 0xff;

	QRgb *currPens = currentPens();
	int count = 0;
	const NesPpuSprite *sprite = (const NesPpuSprite *)spriteMem;
	for (int spriteIndex = 0; spriteIndex < NumSprites; spriteIndex++, sprite++) {
		// compute the character's line to draw
		uint spriteLine = nesPpuScanline - sprite->y();
		// if the sprite isn't visible, skip it
		if (spriteLine != (spriteLine & (spriteSize-1)))
			continue;
		// compute character pattern address
		int tile = sprite->tileIndex();
		if (spriteSize == 16 && (tile & 1)) {
			// if it's 8x16 and odd-numbered, draw the other half instead
			tile &= ~1;
			tile |= 0x100;
		}
		if (sprite->flipVertically())
			spriteLine = (spriteSize-1) - spriteLine;
		// if it's 8x16 and line >= 8 move to next tile
		if (spriteLine & 8) {
			tile++;
			spriteLine &= ~8;
		}
		int index1 = tile * 16;
		if (spriteSize == 8)
			index1 |= spritePageOffset;
		u16 spriteAddress = index1 | spriteLine;
		// read character pattern
		u8 plane1 = nesPpuRead(spriteAddress + 0);
		u8 plane2 = nesPpuRead(spriteAddress + 8);
		// character latch (for MMC2/MMC4)
		if (nesMapper->hasCharacterLatch())
			nesMapper->characterLatch(spriteAddress);
		if (sprite->flipHorizontally()) {
			plane1 = bit2Rev[plane1];
			plane2 = bit2Rev[plane2];
		}
		u8 pixelData = plane1 | plane2;
		// set the "sprite 0 hit" flag if appropriate
		if (!spriteIndex && !sprite0HitOccurred()) {
			int backgroundPos = ((sprite->x()&0xf8)+((nesPpuScroll.xFine+(sprite->x()&0x07))&8))>>3;
			int backgroundShift = 8-((nesPpuScroll.xFine+sprite->x())&7);
			u8 backgroundMask = ((bgWritten[backgroundPos+0]<<8)|bgWritten[backgroundPos+1]) >> backgroundShift;
			if (pixelData & backgroundMask)
				nesPpuRegs[NesPpu::Status] |= NesPpu::Sprite0HitSRBit;
		}
		// sprite mask
		int spritePos = sprite->x()/8;
		int spriteShift = 8-(sprite->x()&7);
		u8 spriteMask = ((spWritten[spritePos+0]<<8)|spWritten[spritePos+1]) >> spriteShift;
		u16 toWrite = pixelData << spriteShift;
		spWritten[spritePos+0] |= toWrite >> 8;
		spWritten[spritePos+1] |= toWrite & 0xff;
		pixelData &= ~spriteMask;

		if (sprite->isBehindBackground()) {
			// BG > SP priority
			int backgroundPos = ((sprite->x()&0xf8)+((nesPpuScroll.xFine+(sprite->x()&0x07))&8))>>3;
			int backgroundShift = 8-((nesPpuScroll.xFine+sprite->x())&7);
			u8 backgroundMask = ((bgWritten[backgroundPos+0]<<8)|bgWritten[backgroundPos+1]) >> backgroundShift;
			pixelData &= ~backgroundMask;
		}
		// blit
		QRgb *dst = scanlineData + sprite->x() + 8;
		QRgb *pens = currPens + (sprite->paletteHighBits() | 0x10);
		register int c1 = ((plane1>>1)&0x55) | (plane2&0xaa);
		register int c2 = (plane1&0x55) | ((plane2<<1)&0xaa);
		if (pixelData&0x80) dst[0] = pens[(c1>>6)];
		if (pixelData&0x08) dst[4] = pens[(c1>>2)&3];
		if (pixelData&0x40) dst[1] = pens[(c2>>6)];
		if (pixelData&0x04) dst[5] = pens[(c2>>2)&3];
		if (pixelData&0x20) dst[2] = pens[(c1>>4)&3];
		if (pixelData&0x02) dst[6] = pens[c1&3];
		if (pixelData&0x10) dst[3] = pens[(c2>>4)&3];
		if (pixelData&0x01) dst[7] = pens[c2&3];

		if (++count == 8) {
			setSpriteMax(true);
			if (ppuSpriteLimit)
				break;
		}
	}
}

static QRgb *bgDst;

static u16 bgNameTableAddress;
static u16 bgAttributeAddress;
static  u8 bgNameTableX;
static  u8 bgAttributeShift;
static u8 *bgNameTable;
static u8 *bgWr;
static QRgb *bgCurrPens;
static u16 bgTileAddress;
static NesMapper::ExtensionLatchResult (*bgExtLatch)(int i, u16 addr);

static inline void fetchTileAddress()
{
	bgTileAddress = bgNameTable[bgNameTableAddress & 0x03ff] * 0x10;
	bgTileAddress |= nesPpuTilePageOffset | nesPpuScroll.yFine();
}

bool nesPpuDrawPrologue()
{
	memset(bgWritten, 0, sizeof(bgWritten));
	if (!nesPpuIsBackgroundVisible()) {
		fillScanline(0, 8+NesPpu::VisibleScreenWidth);
		return false;
	}
	bgDst = scanlineData + (8 - nesPpuScroll.xFine);

	bgNameTableAddress = fetchNameAddress();
	bgAttributeAddress = NesPpu::AttributeTableOffset |
			((bgNameTableAddress&0x0380)>>4);
	bgNameTableX = bgNameTableAddress & 0x001f;
	bgAttributeShift = (bgNameTableAddress & 0x0040) >> 4;
	bgNameTable = nesPpuBank1KData(bgNameTableAddress >> 10);

	bgWr = bgWritten;
	bgCurrPens = currentPens();
	bgExtLatch = nesMapper->extensionLatch;

	fetchTileAddress();
	return true;
}

void nesPpuDrawEpilogue()
{
	// if the left 8 pixels for the background are off, blank them
	if (!(nesPpuRegs[NesPpu::Control1] & NesPpu::BackgroundClipDisableCR1Bit))
		fillScanline(0, 8+8); // +8 because left 8 pixels are for internal use

	drawSprites();
}

void nesPpuDrawBackgroundTile(int i)
{
	Q_ASSERT(i >= 0 && i < 33);
	u8 attribute;
	u8 plane1;
	u8 plane2;
	if (!bgExtLatch) {
		attribute = bgNameTable[bgAttributeAddress + (bgNameTableX >> 2)];
		attribute >>= (bgNameTableX & 2) + bgAttributeShift;
		attribute = (attribute & 3) << 2;
		plane1 = nesPpuRead(bgTileAddress + 0);
		plane2 = nesPpuRead(bgTileAddress + 8);
	} else {
		NesMapper::ExtensionLatchResult r = (*bgExtLatch)(i, bgNameTableAddress);
		attribute = r.attribute & 0x0c;
		plane1 = r.plane1;
		plane2 = r.plane2;
	}
	*bgWr = plane1 | plane2;

	QRgb *pens = bgCurrPens + attribute;
	int c1 = ((plane1>>1)&0x55) | (plane2&0xaa);
	int c2 = (plane1&0x55) | ((plane2<<1)&0xaa);

	bgDst[0] = pens[(c1>>6)&3];
	bgDst[4] = pens[(c1>>2)&3];
	bgDst[1] = pens[(c2>>6)&3];
	bgDst[5] = pens[(c2>>2)&3];
	bgDst[2] = pens[(c1>>4)&3];
	bgDst[6] = pens[(c1>>0)&3];
	bgDst[3] = pens[(c2>>4)&3];
	bgDst[7] = pens[(c2>>0)&3];

	bgDst += 8;
	bgWr++;

	// character latch (for MMC2/MMC4)
	if (nesMapper->hasCharacterLatch())
		nesMapper->characterLatch(bgTileAddress);

	if (++bgNameTableX == 32) {
		bgNameTableX = 0;
		bgNameTableAddress ^= 0x41f;
		bgAttributeAddress = NesPpu::AttributeTableOffset |
				((bgNameTableAddress&0x0380)>>4);
		bgNameTable = nesPpuBank1KData(bgNameTableAddress >> 10);
	} else {
		bgNameTableAddress++;
	}
	fetchTileAddress();
}

void nesPpuDrawBackgroundLine()
{
	for (int i = 0; i < 33; i++)
		nesPpuDrawBackgroundTile(i);
}

void nesPpuProcessDummyScanline()
{
	if (nesPpuScanline >= NesPpu::VisibleScreenHeight || !nesPpuIsSpriteVisible())
		return;
	setSpriteMax(false);
	int count = 0;
	const NesPpuSprite *sprite = (const NesPpuSprite *)spriteMem;
	for (int spriteIndex = 0; spriteIndex < NumSprites; spriteIndex++, sprite++) {
		// compute the character's line to draw
		uint spriteLine = nesPpuScanline - sprite->y();
		// if the sprite isn't visible, skip it
		if (spriteLine != (spriteLine & (spriteSize-1)))
			continue;
		if (++count == 8) {
			setSpriteMax(true);
			break;
		}
	}
}

bool nesPpuCheckSprite0HitHere()
{
	if (sprite0HitOccurred())
		return false;
	if (!nesPpuIsBackgroundVisible() || !nesPpuIsSpriteVisible())
		return false;
	const NesPpuSprite *sprite = (const NesPpuSprite *)spriteMem;
	// compute the character's line to draw
	uint spriteLine = nesPpuScanline - sprite->y();
	// if the sprite isn't visible, skip it
	if (spriteLine != (spriteLine & (spriteSize-1)))
		return false;
	return true;
}

void NesSyncCompiler::mPpuProcessScanlineFunction()
{
	Label draw;
	Label exit;
	Label backgroundHidden;
	Label epilogue;

	__ str(lr, MemOperand(m_dataBase,
						  offsetof(NesSyncData,returnFromPpuProcessing)));

	// check if drawing is required
	__ ldr(r0, MemOperand(m_dataBase, offsetof(NesSyncData,drawEnabled)));
	__ mov(r0, r0, SetCC);
	__ b(&draw, ne);

	// force drawing if sprite 0 hit can occur here
	mCallCFunction(offsetof(NesSyncData,ppuCheckSprite0));
	__ mov(r0, r0, SetCC);
	__ b(&draw, ne);

	// call nesPpuProcessDummyScanline() if drawing is not required ...
	mCallCFunction(offsetof(NesSyncData,ppuDummyScanline));
	// ... and clock cpu if we TileRender is set
	if (m_renderTile)
		mClock(NesPpu::HDrawCycles);
	__ b(&exit);

	__ bind(&draw);
	mCallCFunction(offsetof(NesSyncData,ppuDrawPrologue));
	__ mov(r0, r0, SetCC);
	__ b(&backgroundHidden, eq);

	// render tiles
	if (m_renderTile) {
		for (int i = 0; i < 33; i++) {
			if (i)
				mClock(NesPpu::FetchCycles*4);
			__ mov(r0, Operand(i));
			mCallCFunction(offsetof(NesSyncData,ppuDrawBackgroundTile));
		}
		__ b(&epilogue);
	} else {
		mCallCFunction(offsetof(NesSyncData,ppuDrawBackgroundLine));
	}

	__ bind(&backgroundHidden);
	if (m_renderTile)
		mClock(NesPpu::HDrawCycles);

	__ bind(&epilogue);
	mCallCFunction(offsetof(NesSyncData,ppuDrawEpilogue));

	__ bind(&exit);
	// return to main loop
	__ ldr(pc, MemOperand(m_dataBase,
						  offsetof(NesSyncData,returnFromPpuProcessing)));
}

/*!
	Function returns color of a pixel at the given postion (\a x, \a y).
	It is used in zapper emulation to check if pointed pixel has appropriate
	color.
 */
QRgb nesPpuGetPixel(int x, int y)
{
	Q_ASSERT(x >= 0 && x < NesPpu::VisibleScreenWidth);
	Q_ASSERT(y >= 0 && y < NesPpu::VisibleScreenHeight);
	return nesPpuFrame.pixel(x+8, y);
}

static void fillPens()
{
	/*
		This routine builds a palette using a transformation from
		the YUV (Y, B-Y, R-Y) to the RGB color space. The NES has a 64 color
		palette 16 colors, with 4 luminance levels for each color.
	 */

	int entry = 0;
	qreal tint = 0.25f;	// adjust to taste
	qreal hue = 287.0f;

	qreal Kr = 0.2989f;
	qreal Kb = 0.1145f;
	qreal Ku = 2.029f;
	qreal Kv = 1.140f;

	static const qreal brightness[3][4] = {
		{ 0.50f, 0.75f,  1.0f,  1.0f },
		{ 0.29f, 0.45f, 0.73f,  0.9f },
		{ 0.0f,  0.24f, 0.47f, 0.77f }
	};
	// Loop through the emphasis modes (8 total)
	for (int colorEmphasis = 0; colorEmphasis < 8; colorEmphasis++) {
		// loop through the 4 intensities
		for (int colorIntensity = 0; colorIntensity < 4; colorIntensity++) {
			// loop through the 16 colors
			for (int colorNum = 0; colorNum < 16; colorNum++) {
				qreal sat;
				qreal y, u, v;
				qreal rad;

				switch (colorNum) {
				case 0:
					sat = 0.0f; rad = 0.0f;
					y = brightness[0][colorIntensity];
					break;
				case 13:
					sat = 0.0f; rad = 0.0f;
					y = brightness[2][colorIntensity];
					break;
				case 14:
				case 15:
					sat = 0.0f; rad = 0.0f; y = 0.0f;
					break;
				default:
					sat = tint;
					rad = M_PI * (qreal(qreal(colorNum) * 30.0f + hue) / 180.0f);
					y = brightness[1][colorIntensity];
					break;
				}
				u = sat * qCos(rad);
				v = sat * qSin(rad);
				// Transform to RGB
				qreal R = (y + Kv * v) * 255.0f;
				qreal G = (y - (Kb * Ku * u + Kr * Kv * v) / (1 - Kb - Kr)) * 255.0f;
				qreal B = (y + Ku * u) * 255.0f;
				// Clipping, in case of saturation
				R = qMax(qreal(0.0f), qMin(R, qreal(255.0f)));
				G = qMax(qreal(0.0f), qMin(G, qreal(255.0f)));
				B = qMax(qreal(0.0f), qMin(B, qreal(255.0f)));
				// emphasis
				R = ((colorEmphasis & 1) ? 255.0f : R);
				G = ((colorEmphasis & 2) ? 255.0f : G);
				B = ((colorEmphasis & 4) ? 255.0f : B);
				// Round, and set the value
				palettePens[entry++] = qRgb(qFloor(R + 0.5f), qFloor(G + 0.5f), qFloor(B + 0.5f));
			}
		}
	}
}

void NesPpu::setSpriteLimit(bool on)
{
	if (ppuSpriteLimit != on) {
		ppuSpriteLimit = on;
		emConf.setValue(spriteLimitConfName, ppuSpriteLimit);
		emit spriteLimitChanged();
	}
}

bool NesPpu::spriteLimit() const
{
	return ppuSpriteLimit;
}

int NesPpu::scanlineCount() const
{
	return (nesSystemType == NES_NTSC) ? (240+1+20+1) : (240+1+70+1);
}

void nesPpuSl()
{
	emsl.begin("ppu");
	emsl.var("scroll.address", nesPpuScroll.address);
	emsl.var("scroll.latch", nesPpuScroll.latch);
	emsl.var("scroll.toggle", nesPpuScroll.toggle);
	emsl.var("scroll.xFine", nesPpuScroll.xFine);
	emsl.var("scrollAddressIncrement", scrollAddressIncrement);

	emsl.var("vBlankOut", vBlankOut);

	emsl.array("regs", nesPpuRegs, sizeof(nesPpuRegs));
	emsl.var("dataLatch", dataLatch);
	emsl.var("bufferedData", bufferedData);
	emsl.var("securityValue", securityValue);

	emsl.array("spriteMem", spriteMem, sizeof(spriteMem));
	emsl.array("paletteMem", paletteMem, sizeof(paletteMem));
	emsl.end();

	if (!emsl.save) {
		updateCachedControl0Bits();
		updateColorEmphasisAndMask();
		palettePenLutNeedsRebuild = true;
		ppuSpriteLimit = emConf.value(spriteLimitConfName, true).toBool();
	}
}
