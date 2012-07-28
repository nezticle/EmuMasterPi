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

#ifndef MAPPER_H
#define MAPPER_H

#include "nes.h"

enum NesMirroring {
	VerticalMirroring = 0,
	HorizontalMirroring,
	SingleLow,
	SingleHigh,
	FourScreenMirroring
};

enum NesPpuBankType {
	VromBank = 0,
	CramBank,
	VramBank
};

class NesMapper
{
public:
	static NesMapper *create(u8 type);
	QString name() const { return m_name; }

	virtual void reset();
	void sl();

	// -------- CPU section -----------

	// 0x4100-0x7fff lower memory read/write
	void (*writeLow)(u16 addr, u8 data);
	  u8 (*readLow)(u16 addr);

	// 0x8000-0xffff memory write
	void (*writeHigh)(u16 addr, u8 data);
	bool hasWriteHigh() const { return writeHigh != 0; }

	// 0x4018-0x40ff extention register read/write
	void (*writeEx)(u16 addr, u8 data);
	  u8 (*readEx)(u16 addr);

	void (*clock)(int cycles);
	bool hasClock() const { return clock != 0; }

	// ------- PPU section ------

	void (*horizontalSync)();
	bool hasHorizontalSync() const { return horizontalSync != 0; }
	void (*verticalSync)();
	bool hasVerticalSync() const { return verticalSync != 0; }

	void (*characterLatch)(u16 addr);
	bool hasCharacterLatch() const { return characterLatch != 0; }

	class ExtensionLatchResult {
	public:
		u8 plane1;
		u8 plane2;
		u8 attribute;
	} Q_PACKED;
	ExtensionLatchResult (*extensionLatch)(int i, u16 addr);
	bool hasExtensionLatch() const { return extensionLatch != 0; }
protected:
	virtual void extSl() {}
private:
	QString m_name;
};

extern uint nesRomSizeInBytes;
extern uint nesRomSize16KB;
extern uint nesRomSize8KB;

extern uint nesVromSizeInBytes;
extern uint nesVromSize8KB;
extern uint nesVromSize4KB;
extern uint nesVromSize2KB;
extern uint nesVromSize1KB;

extern u8 nesTrainer[512];
extern NesMirroring nesMirroring;
extern NesMirroring nesDefaultMirroring;

extern int nesMapperType;
extern NesMapper *nesMapper;

extern u8 *nesRom;
extern u8 nesRam[8*1024];
extern u8 nesWram[128*1024];
extern u8 nesXram[8*1024];

extern u8 *nesVrom;
extern u8 nesVram[4*1024];
extern u8 nesCram[32*1024];

extern u8 *nesPpuBanks[16];
extern NesPpuBankType nesPpuBanksType[16];
extern u8 *nesCpuBanks[8]; // 8K banks 0x0000-0xffff

static const int NesNumOfCpuPages	= 8;
static const int NesCpuBankSize = 0x2000;
static const int NesCpuBankMask = NesCpuBankSize-1;

static inline int nesCpuPageByAddr(u16 address)
{ return address >> 13; }

extern void nesCpuWrite(u16 addr, u8 data);
extern   u8 nesCpuRead(u16 addr);

extern void nesCpuWrite40xx(u16 addr, u8 data);
extern   u8 nesCpuRead40xx(u16 addr);

extern void nesDefaultCpuWriteLow(u16 addr, u8 data);
extern  u8 nesDefaultCpuReadLow(u16 addr);
extern void nesDefaultCpuWriteEx(u16 addr, u8 data);
extern  u8 NnesDefaultCpuReadEx(u16 addr);

static inline void nesCpuWriteDirect(u16 addr, u8 data)
{ nesCpuBanks[addr >> 13][addr & 0x1fff] = data; }
static inline u8 nesCpuReadDirect(u16 addr)
{ return nesCpuBanks[addr >> 13][addr & 0x1fff]; }

static inline void nesPpuWrite(u16 addr, u8 data)
{ Q_ASSERT((addr >> 10) < 16); nesPpuBanks[addr >> 10][addr & 0x3ff] = data; }
static inline u8 nesPpuRead(u16 addr)
{ Q_ASSERT((addr >> 10) < 16); return nesPpuBanks[addr >> 10][addr & 0x3ff]; }

static inline NesPpuBankType nesPpuBank1KType(uint bank)
{ Q_ASSERT(bank < 16); return nesPpuBanksType[bank]; }
static inline u8 *nesPpuBank1KData(uint bank)
{ Q_ASSERT(bank < 16); return nesPpuBanks[bank]; }

extern void nesSetMirroring(uint bank0, uint bank1, uint bank2, uint bank3);
extern void nesSetMirroring(NesMirroring mirroring);

extern void nesSetRom8KBank(uint page, uint romBank8K);
extern void nesSetRom16KBank(uint page, uint romBank16K);
extern void nesSetRom32KBank(uint romBank32K);
extern void nesSetRom8KBanks(uint bank4, uint bank5, uint bank6, uint bank7);

extern void nesSetWram8KBank(uint page, uint wramBank8K);

extern void nesSetVrom1KBank(uint page, uint vromBank1K);
extern void nesSetVrom2KBank(uint page, uint vromBank2K);
extern void nesSetVrom4KBank(uint page, uint vromBank4K);
extern void nesSetVrom8KBank(uint vromBank8K);

extern void nesSetCram1KBank(uint page, uint cramBank1K);
extern void nesSetCram2KBank(uint page, uint cramBank2K);
extern void nesSetCram4KBank(uint page, uint cramBank4K);
extern void nesSetCram8KBank(uint cramBank8K);
extern void nesSetCram8KBank(uint cramBank8K);

extern void nesSetVram1KBank(uint page, uint vramBank1K);

extern void nesMapperSetIrqSignalOut(bool on);

#endif // MAPPER_H
