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

#include "mapper005.h"
#include "disk.h"
#include "ppu.h"

enum Irq {
	IrqDefault = 0,
	IrqMetal
};
static u8	sram_size;

static u8	prg_size;		// $5100
static u8	chr_size;		// $5101
static u8	sram_we_a, sram_we_b;	// $5102-$5103
static u8	graphic_mode;		// $5104
static u8	nametable_mode;		// $5105
static u8	nametable_type[4];	// $5105 use

static u8	sram_page;		// $5113

static u8	fill_chr, fill_pal;	// $5106-$5107
static u8	split_control;		// $5200
static u8	split_scroll;		// $5201
static u8	split_page;		// $5202

static u8	split_x;
static u16	split_addr;
static u16	split_yofs;

static u8	chr_type;
static u8	chr_mode;		// $5120-$512B use
static u8	chr_page[2][8];		// $5120-$512B
static u8 * bg_mem_bank[8];

static u8	irq_status;		// $5204(R)
static u8	irq_enable;		// $5204(W)
static u8	irq_line;		// $5203
static u8	irq_scanline;
static u8	irq_clear;		// HSync
static u8	irq_type;

static u8	mult_a, mult_b;		// $5205-$5206

static bool cpu_bank_wren[8];

static void updatePpuBanks()
{
	if (chr_mode == 0) {
		// PPU SP Bank
		switch (chr_size) {
		case 0:
			nesSetVrom8KBank(chr_page[0][7]);
			break;
		case 1:
			nesSetVrom4KBank(0, chr_page[0][3]);
			nesSetVrom4KBank(4, chr_page[0][7]);
			break;
		case 2:
			nesSetVrom2KBank(0, chr_page[0][1]);
			nesSetVrom2KBank(2, chr_page[0][3]);
			nesSetVrom2KBank(4, chr_page[0][5]);
			nesSetVrom2KBank(6, chr_page[0][7]);
			break;
		case 3:
			for (int i = 0; i < 8; i++)
				nesSetVrom1KBank(i, chr_page[0][i]);
			break;
		}
	} else if (chr_mode == 1) {
		// PPU BG Bank
		switch (chr_size) {
		case 0:
			for (int i = 0; i < 8; i++) {
				bg_mem_bank[i] = nesVrom+0x2000*(chr_page[1][7]%nesVromSize8KB)+0x0400*i;
			}
			break;
		case 1:
			for (int i = 0; i < 4; i++) {
				bg_mem_bank[i+0] = nesVrom+0x1000*(chr_page[1][3]%nesVromSize4KB)+0x0400*i;
				bg_mem_bank[i+4] = nesVrom+0x1000*(chr_page[1][7]%nesVromSize4KB)+0x0400*i;
			}
			break;
		case 2:
			for (int i = 0; i < 2; i++) {
				bg_mem_bank[i+0] = nesVrom+0x0800*(chr_page[1][1]%nesVromSize2KB)+0x0400*i;
				bg_mem_bank[i+2] = nesVrom+0x0800*(chr_page[1][3]%nesVromSize2KB)+0x0400*i;
				bg_mem_bank[i+4] = nesVrom+0x0800*(chr_page[1][5]%nesVromSize2KB)+0x0400*i;
				bg_mem_bank[i+6] = nesVrom+0x0800*(chr_page[1][7]%nesVromSize2KB)+0x0400*i;
			}
			break;
		case 3:
			for (int i = 0; i < 8; i++) {
				bg_mem_bank[i] = nesVrom+0x0400*(chr_page[1][i]%nesVromSize1KB);
			}
			break;
		}
	}
}

static void setBankSram(u8 page, u8 data)
{
	if (sram_size == 0) data = (data > 3) ? 8 : 0;
	if (sram_size == 1) data = (data > 3) ? 1 : 0;
	if (sram_size == 2) data = (data > 3) ? 8 : data;
	if (sram_size == 3) data = (data > 3) ? 4 : data;

	if (data != 8) {
		nesSetWram8KBank(page, data);
		cpu_bank_wren[page] = true;
	} else {
		cpu_bank_wren[page] = false;
	}
}

static void setBankCpu(u16 addr, u8 data)
{
	if (data & 0x80) {
		// PROM Bank
		switch (addr & 7) {
		case 4:
			if (prg_size == 3) {
				nesSetRom8KBank(4, data&0x7f);
				cpu_bank_wren[4] = false;
			}
			break;
		case 5:
			if (prg_size == 1 || prg_size == 2) {
				nesSetRom16KBank(4, (data&0x7f)>>1);
				cpu_bank_wren[4] = false;
				cpu_bank_wren[5] = false;
			} else if (prg_size == 3) {
				nesSetRom8KBank(5, (data&0x7f));
				cpu_bank_wren[5] = false;
			}
			break;
		case 6:
			if (prg_size == 2 || prg_size == 3) {
				nesSetRom8KBank(6, (data&0x7f));
				cpu_bank_wren[6] = false;
			}
			break;
		case 7:
			if (prg_size == 0) {
				nesSetRom32KBank((data&0x7f)>>2);
				cpu_bank_wren[4] = false;
				cpu_bank_wren[5] = false;
				cpu_bank_wren[6] = false;
				cpu_bank_wren[7] = false;
			} else if (prg_size == 1) {
				nesSetRom16KBank(6, (data&0x7f)>>1);
				cpu_bank_wren[6] = false;
				cpu_bank_wren[7] = false;
			} else if (prg_size == 2 || prg_size == 3) {
				nesSetRom8KBank(7, (data&0x7f));
				cpu_bank_wren[7] = false;
			}
			break;
		}
	} else {
		// WRAM Bank
		switch (addr & 7) {
		case 4:
			if (prg_size == 3) {
				setBankSram(4, data&0x07);
			}
			break;
		case 5:
			if (prg_size == 1 || prg_size == 2) {
				setBankSram(4, (data&0x06)+0);
				setBankSram(5, (data&0x06)+1);
			} else if (prg_size == 3) {
				setBankSram(5, data&0x07);
			}
			break;
		case 6:
			if (prg_size == 2 || prg_size == 3) {
				setBankSram(6, data&0x07);
			}
			break;
		}
	}
}

static u8 readLow(u16 addr)
{
	u8 data = addr >> 8;

	switch (addr) {
	case 0x5015:
		// TODO ex souund data = nes->apu->ExRead( address);
		break;

	case 0x5204:
		data = irq_status;
//		irq_status = 0;
		irq_status &= ~0x80;
		nesMapperSetIrqSignalOut(false);
		break;
	case 0x5205:
		data = mult_a*mult_b;
		break;
	case 0x5206:
		data = (u8)(((u16)mult_a*(u16)mult_b)>>8);
		break;
	}

	if (addr >= 0x5c00 && addr <= 0x5fff) {
		if (graphic_mode >= 2) { // ExRAM mode
			data = nesVram[0x0800+(addr&0x3ff)];
		}
	} else if (addr >= 0x6000 && addr <= 0x7fff) {
		data = nesDefaultCpuReadLow(addr);
	}
	return data;
}

static void writeLow(u16 addr, u8 data)
{
	switch (addr) {
	case 0x5100:
		prg_size = data & 0x03;
		break;
	case 0x5101:
		chr_size = data & 0x03;
		break;

	case 0x5102:
		sram_we_a = data & 0x03;
		break;
	case 0x5103:
		sram_we_b = data & 0x03;
		break;

	case 0x5104:
		graphic_mode = data & 0x03;
		break;
	case 0x5105:
		nametable_mode = data;
		for (int i = 0; i < 4; i++) {
			nametable_type[i] = data&0x03;
			nesSetVram1KBank(8+i, nametable_type[i]);
			data >>= 2;
		}
		break;

	case 0x5106:
		fill_chr = data;
		break;
	case 0x5107:
		fill_pal = data & 0x03;
		break;

	case 0x5113:
		setBankSram(3, data&0x07);
		break;

	case 0x5114:
	case 0x5115:
	case 0x5116:
	case 0x5117:
		setBankCpu(addr, data);
		break;

	case 0x5120:
	case 0x5121:
	case 0x5122:
	case 0x5123:
	case 0x5124:
	case 0x5125:
	case 0x5126:
	case 0x5127:
		chr_mode = 0;
		chr_page[0][addr&0x07] = data;
		updatePpuBanks();
		break;

	case 0x5128:
	case 0x5129:
	case 0x512a:
	case 0x512b:
		chr_mode = 1;
		chr_page[1][(addr&0x03)+0] = data;
		chr_page[1][(addr&0x03)+4] = data;
		updatePpuBanks();
		break;

	case 0x5200:
		split_control = data;
		break;
	case 0x5201:
		split_scroll = data;
		break;
	case 0x5202:
		split_page = data&0x3f;
		break;

	case 0x5203:
		irq_line = data;
		nesMapperSetIrqSignalOut(false);
		break;
	case 0x5204:
		irq_enable = data;
		nesMapperSetIrqSignalOut(false);
		break;

	case 0x5205:
		mult_a = data;
		break;
	case 0x5206:
		mult_b = data;
		break;

	default:
		if (addr >= 0x5000 && addr <= 0x5015) {
			// TODO ex apu nes->apu->ExWrite( address, data);
		} else if (addr >= 0x5c00 && addr <= 0x5fff) {
			if (graphic_mode == 2) {		// ExRAM
				nesVram[0x0800+(addr&0x3ff)] = data;
			} else if (graphic_mode != 3) {	// Split,ExGraphic
				if (irq_status&0x40) {
					nesVram[0x0800+(addr&0x3ff)] = data;
				} else {
					nesVram[0x0800+(addr&0x3ff)] = 0;
				}
			}
		} else if (addr >= 0x6000 && addr <= 0x7fff) {
			if ((sram_we_a == 0x02) && (sram_we_b == 0x01)) {
				if (cpu_bank_wren[3]) {
					nesCpuWriteDirect(addr, data);
				}
			}
		}
		break;
	}
}

static void writeHigh(u16 addr, u8 data)
{
	if (sram_we_a == 0x02 && sram_we_b == 0x01) {
		if (addr >= 0x8000 && addr < 0xe000) {
			if (cpu_bank_wren[addr >> 13]) {
				nesCpuWriteDirect(addr, data);
			}
		}
	}
}

static void horizontalSync()
{
	if (irq_type & IrqMetal) {
		if (irq_scanline == irq_line) {
			irq_status |= 0x80;
		}
	}
	if (nesPpuIsDisplayOn() && nesPpuScanline < NesPpu::VisibleScreenHeight) {
		irq_scanline++;
		irq_status |= 0x40;
		irq_clear = 0;
	} else if (irq_type & IrqMetal) {
		irq_scanline = 0;
		irq_status &= ~0x80;
		irq_status &= ~0x40;
	}

	if (!(irq_type & IrqMetal)) {
		if (irq_scanline == irq_line) {
			irq_status |= 0x80;
		}

		if (++irq_clear > 2) {
			irq_scanline = 0;
			irq_status &= ~0x80;
			irq_status &= ~0x40;
			nesMapperSetIrqSignalOut(false);
		}
	}

	if ((irq_enable & 0x80) && (irq_status & 0x80) && (irq_status & 0x40)) {
		nesMapperSetIrqSignalOut(true);
	}

	// For Split mode!
	if (nesPpuScanline == 0) {
		split_yofs = split_scroll&0x07;
		split_addr = ((split_scroll&0xF8)<<2);
	} else if (nesPpuIsDisplayOn()) {
		if (split_yofs == 7) {
			split_yofs = 0;
			if ((split_addr & 0x03E0) == 0x03A0) {
				split_addr &= 0x001F;
			} else {
				if ((split_addr & 0x03E0) == 0x03E0) {
					split_addr &= 0x001F;
				} else {
					split_addr += 0x0020;
				}
			}
		} else {
			split_yofs++;
		}
	}
}

static NesMapper::ExtensionLatchResult extensionLatch(int i, u16 addr)
{
	NesMapper::ExtensionLatchResult result;
	split_x = i;

	u16	ntbladr, attradr, tileadr;
	uint tilebank;
	bool bSplit;

	bSplit = false;
	if (split_control & 0x80) {
		if (!(split_control&0x40)) {
			// Left side
			if ((split_control&0x1F) > split_x) {
				bSplit = true;
			}
		} else {
			// Right side
			if ((split_control&0x1F) <= split_x) {
				bSplit = true;
			}
		}
	}

	if (!bSplit) {
		if (nametable_type[(addr&0x0C00)>>10] == 3) {
			// Fill mode
			if (graphic_mode == 1) {
				// ExGraphic mode
				ntbladr = 0x2000+(addr&0x0FFF);
				// Get Nametable
				tileadr = fill_chr*0x10+nesPpuScroll.yFine();
				// Get TileBank
				tilebank = 0x1000*((nesVram[0x0800+(ntbladr&0x03FF)]&0x3F)%nesVromSize4KB);
				// Attribute
				result.attribute = (fill_pal<<2)&0x0C;
				// Get Pattern
				result.plane1 = nesVrom[tilebank+tileadr  ];
				result.plane2 = nesVrom[tilebank+tileadr+8];
			} else {
				// Normal
				tileadr = nesPpuTilePageOffset+fill_chr*0x10+nesPpuScroll.yFine();
				result.attribute = (fill_pal<<2)&0x0C;
				// Get Pattern
				if (chr_type) {
					result.plane1 = nesPpuRead(tileadr + 0);
					result.plane2 = nesPpuRead(tileadr + 8);
				} else {
					result.plane1 = bg_mem_bank[tileadr>>10][ tileadr&0x03FF   ];
					result.plane2 = bg_mem_bank[tileadr>>10][(tileadr&0x03FF)+8];
				}
			}
		} else if (graphic_mode == 1) {
			// ExGraphic mode
			ntbladr = 0x2000+(addr&0x0FFF);
			// Get Nametable
			tileadr = (u16)nesPpuRead(ntbladr)*0x10+nesPpuScroll.yFine();
			// Get TileBank
			tilebank = 0x1000*((nesVram[0x0800+(ntbladr&0x03FF)]&0x3F)%nesVromSize4KB);
			// Get Attribute
			result.attribute = (nesVram[0x0800+(ntbladr&0x03FF)]&0xC0)>>4;
			// Get Pattern
			result.plane1 = nesVrom[tilebank+tileadr  ];
			result.plane2 = nesVrom[tilebank+tileadr+8];
		} else {
			// Normal or ExVRAM
			ntbladr = 0x2000+(addr&0x0FFF);
			attradr = 0x23C0+(addr&0x0C00)+((addr&0x0380)>>4)+((addr&0x001C)>>2);
			// Get Nametable
			tileadr = nesPpuTilePageOffset+nesPpuRead(ntbladr)*0x10+nesPpuScroll.yFine();
			// Get Attribute
			result.attribute = nesPpuRead(attradr);
			if (ntbladr & 0x0002) result.attribute >>= 2;
			if (ntbladr & 0x0040) result.attribute >>= 4;
			result.attribute = (result.attribute&0x03)<<2;
			// Get Pattern
			if (chr_type) {
				result.plane1 = nesPpuRead(tileadr + 0);
				result.plane2 = nesPpuRead(tileadr + 8);
			} else {
				result.plane1 = bg_mem_bank[tileadr>>10][ tileadr&0x03FF   ];
				result.plane2 = bg_mem_bank[tileadr>>10][(tileadr&0x03FF)+8];
			}
		}
	} else {
		ntbladr = ((split_addr&0x03E0)|(split_x&0x1F))&0x03FF;
		// Get Split TileBank
		tilebank = 0x1000*((int)split_page%nesVromSize4KB);
		tileadr  = (u16)nesVram[0x0800+ntbladr]*0x10+split_yofs;
		// Get Attribute
		attradr = 0x03C0+((ntbladr&0x0380)>>4)+((ntbladr&0x001C)>>2);
		result.attribute = nesVram[0x0800+attradr];
		if (ntbladr & 0x0002) result.attribute >>= 2;
		if (ntbladr & 0x0040) result.attribute >>= 4;
		result.attribute = (result.attribute&0x03)<<2;
		// Get Pattern
		result.plane1 = nesVrom[tilebank+tileadr  ];
		result.plane2 = nesVrom[tilebank+tileadr+8];
	}
	return result;
}

void Mapper005::reset()
{
	NesMapper::reset();
	readLow = ::readLow;
	writeLow = ::writeLow;
	writeHigh = ::writeHigh;
	horizontalSync = ::horizontalSync;
	extensionLatch = ::extensionLatch;

	cpu_bank_wren[0] = true;
	cpu_bank_wren[1] = true;
	cpu_bank_wren[2] = true;
	cpu_bank_wren[3] = true;
	cpu_bank_wren[4] = false;
	cpu_bank_wren[5] = false;
	cpu_bank_wren[6] = false;
	cpu_bank_wren[7] = false;

	prg_size = 3;
	chr_size = 3;

	sram_we_a = 0x00;
	sram_we_b = 0x00;

	graphic_mode = 0;
	nametable_mode = 0;

	for (uint i = 0; i < sizeof(nametable_type); i++)
		nametable_type[i] = 0;

	fill_chr = fill_pal = 0;
	split_control = split_scroll = split_page = 0;

	irq_enable = 0;
	irq_status = 0;
	irq_scanline = 0;
	irq_line = 0;
	irq_clear = 0;

	irq_type = IrqDefault;

	mult_a = mult_b = 0;

	chr_type = 0;
	chr_mode = 0;

	for (int i = 0; i < 8; i++) {
		chr_page[0][i] = i;
		chr_page[1][i] = 4+(i&0x03);
	}
	for (int i = 4; i < 8; i++)
		nesSetRom8KBank(i, nesRomSize8KB-1);
	nesSetVrom8KBank(0);

	for (int i = 0; i < 8; i++) {
		bg_mem_bank[i] = nesVrom+0x0400*i;
	}

	setBankSram(3, 0);

	sram_size = 0;

	u32 crc = nesDiskCrc;

	if (crc == 0x2b548d75	// Bandit Kings of Ancient China(U)
	 || crc == 0xf4cd4998	// Dai Koukai Jidai(J)
	 || crc == 0x8fa95456	// Ishin no Arashi(J)
	 || crc == 0x98c8e090	// Nobunaga no Yabou - Sengoku Gunyuu Den(J)
	 || crc == 0x8e9a5e2f	// L'Empereur(Alt)(U)
	 || crc == 0x57e3218b	// L'Empereur(U)
	 || crc == 0x2f50bd38	// L'Empereur(J)
	 || crc == 0xb56958d1	// Nobunaga's Ambition 2(U)
	 || crc == 0xe6c28c5f	// Suikoden - Tenmei no Chikai(J)
	 || crc == 0xcd35e2e9) {	// Uncharted Waters(U)
		sram_size = 1;
	} else
	if (crc == 0xf4120e58	// Aoki Ookami to Shiroki Mejika - Genchou Hishi(J)
	 || crc == 0x286613d8	// Nobunaga no Yabou - Bushou Fuuun Roku(J)
	 || crc == 0x11eaad26	// Romance of the Three Kingdoms 2(U)
	 || crc == 0x95ba5733) {	// Sangokushi 2(J)
		sram_size = 2;
	}

	if (crc == 0x95ca9ec7) { // Castlevania 3 - Dracula's Curse(U)
		nesEmuSetRenderMethod(NesEmu::TileRender);
	}

	if (crc == 0xcd9acf43) { // Metal Slader Glory(J)
		irq_type = IrqMetal;
	}

	if (crc == 0xe91548d8) { // Shin 4 Nin Uchi Mahjong - Yakuman Tengoku(J)
		chr_type = 1;
	}

// TODO ex sound	nes->apu->SelectExSound( 8);
}

void Mapper005::extSl()
{
	emsl.var("prg_size", prg_size);
	emsl.var("chr_size", chr_size);
	emsl.var("sram_we_a", sram_we_a);
	emsl.var("sram_we_b", sram_we_b);
	emsl.var("graphic_mode", graphic_mode);
	emsl.var("nametable_mode", nametable_mode);
	emsl.array("nametable_type", nametable_type, sizeof(nametable_type));
	emsl.var("sram_page", sram_page);
	emsl.var("fill_chr", fill_chr);
	emsl.var("fill_pal", fill_pal);
	emsl.var("split_control", split_control);
	emsl.var("split_scroll", split_scroll);
	emsl.var("split_page", split_page);
	emsl.var("chr_mode", chr_mode);
	emsl.var("irq_status", irq_status);
	emsl.var("irq_enable", irq_enable);
	emsl.var("irq_line", irq_line);
	emsl.var("irq_scanline", irq_scanline);
	emsl.var("irq_clear", irq_clear);
	emsl.var("mult_a", mult_a);
	emsl.var("mult_b", mult_b);
	emsl.array("chr_page", chr_page, 2*8);
	emsl.array("cpu_bank_wren", cpu_bank_wren, sizeof(cpu_bank_wren));
	updatePpuBanks();
}
