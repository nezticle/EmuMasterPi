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

#include "disk.h"
#include "mapper.h"
#include <base/crc32.h>
#include <QtCore/QFile>
#include <QtCore/QDebug>

class NesDiskHeader
{
public:
	static const int FourScreenFlagA		= 0x08;
	static const int TrainerFlagA			= 0x04;
	static const int BatteryBackedRamFlagA	= 0x02;
	static const int VerticalFlagA			= 0x01;

	static const int VSSystemFlagB			= 0x01;

	char magic[4];
	u8 num16KBRomBanks;
	u8 num8KBVromBanks;
	u8 flagsA;
	u8 flagsB;
	u8 num8KBRamBanks;
	u8 flagsC;
} Q_PACKED;

u32 nesDiskCrc = 0;

static NesDiskHeader header;

static void patchRom();

bool nesDiskHasTrainer()
{
	return header.flagsA & NesDiskHeader::TrainerFlagA;
}

static void computeChecksum(QFile *file)
{
	if (nesDiskHasTrainer()) {
		file->seek(sizeof(NesDiskHeader));
		QByteArray ba = file->read(nesRomSizeInBytes+512);
		nesDiskCrc = qChecksum32(ba.constData(), nesRomSizeInBytes+512);
	} else {
		nesDiskCrc = qChecksum32((const char *)nesRom, nesRomSizeInBytes);
	}
}

static bool loadHeaderAndMemory(QFile *file)
{
	if (file->read((char *)&header, sizeof(header)) != sizeof(header))
		return false;
	if (qstrncmp(header.magic, "NES\x1A", 4))
		return false;

	file->seek(16);
	if (nesDiskHasTrainer()) {
		if (file->read((char *)nesTrainer, 512) != 512)
			return false;
	}
	nesRomSize16KB = header.num16KBRomBanks;
	nesRomSize8KB = nesRomSize16KB << 1;
	nesRomSizeInBytes = nesRomSize16KB * 0x4000;
	nesRom = new u8[nesRomSizeInBytes];
	if (file->read((char *)nesRom, nesRomSizeInBytes) != nesRomSizeInBytes)
		return false;

	nesVromSize8KB = header.num8KBVromBanks;
	nesVromSize4KB = nesVromSize8KB << 1;
	nesVromSize2KB = nesVromSize4KB << 1;
	nesVromSize1KB = nesVromSize2KB << 1;
	nesVromSizeInBytes = nesVromSize8KB * 0x2000;
	nesVrom = new u8[nesVromSizeInBytes];
	if (file->read((char *)nesVrom, nesVromSizeInBytes) != nesVromSizeInBytes)
		return false;

	return true;
}

bool nesDiskLoad(const QString &fileName, QString *error)
{
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly)) {
		*error = EM_MSG_OPEN_FILE_FAILED;
		return false;
	}
	if (!loadHeaderAndMemory(&file)) {
		*error = EM_MSG_FILE_CORRUPTED;
		return false;
	}

	computeChecksum(&file);
	patchRom();

	nesMapperType = (header.flagsA >> 4) | (header.flagsB & 0xF0);
	// PAL/NTSC is rarely provided, so we don't rely on this flag
	// nesSystemType = (header.flagsC & 0x01) ? NES_PAL : NES_NTSC;

	if (header.flagsA & NesDiskHeader::FourScreenFlagA)
		nesMirroring = FourScreenMirroring;
	else if (header.flagsA & NesDiskHeader::VerticalFlagA)
		nesMirroring = VerticalMirroring;
	else
		nesMirroring = HorizontalMirroring;
	nesDefaultMirroring = nesMirroring;
	return true;
}

void nesDiskShutdown()
{
	delete[] nesRom;
	delete[] nesVrom;
}

bool nesDiskHasBatteryBackedRam()
{
	return header.flagsA & NesDiskHeader::BatteryBackedRamFlagA;
}
//bool NesDisk::isVSSystem() const
//{
//	return header.flagsB & NesDiskHeader::VSSystemFlagB;
//}

static void patchRom()
{
	// Mapper 000
	if (nesDiskCrc == 0x57970078) {	// F-1 Race(J)
		nesRom[0x078C] = 0x6C;
		nesRom[0x3FE1] = 0xFF;
		nesRom[0x3FE6] = 0x00;
	}
	if (nesDiskCrc == 0xaf2bbcbc		// Mach Rider(JU)
	 || nesDiskCrc == 0x3acd4bf1		// Mach Rider(Alt)(JU)
	 || nesDiskCrc == 0x8bbe9bec) {
		nesRom[0x090D] = 0x6E;
		nesRom[0x7FDF] = 0xFF;
		nesRom[0x7FE4] = 0x00;

		header.flagsA = NesDiskHeader::VerticalFlagA;
	}

	if (nesDiskCrc == 0xe16bb5fe) {	// Zippy Race(J)
		header.flagsA &= 0xf6;
	}
	if (nesDiskCrc == 0x85534474) {	// Lode Runner(J)
		nesRom[0x29E9] = 0xEA;
		nesRom[0x29EA] = 0xEA;
		nesRom[0x29F8] = 0xEA;
		nesRom[0x29F9] = 0xEA;
	}

	// Mapper 001
	if (nesDiskCrc == 0x7831b2ff		// America Daitouryou Senkyo(J)
	 || nesDiskCrc == 0x190a3e11		// Be-Bop-Highschool - Koukousei Gokuraku Densetsu(J)
	 || nesDiskCrc == 0x52449508		// Home Run Nighter - Pennant League!!(J)
	 || nesDiskCrc == 0x0973f714		// Jangou(J)
	 || nesDiskCrc == 0x7172f3d4		// Kabushiki Doujou(J)
	 || nesDiskCrc == 0xa5781280		// Kujaku Ou 2(J)
	 || nesDiskCrc == 0x8ce9c87b		// Money Game, The(J)
	 || nesDiskCrc == 0xec47296d		// Morita Kazuo no Shougi(J)
	 || nesDiskCrc == 0xcee5857b		// Ninjara Hoi!(J)
	 || nesDiskCrc == 0xe63d9193		// Tanigawa Kouji no Shougi Shinan 3(J)
	 || nesDiskCrc == 0xd54f5da9		// Tsuppari Wars(J)
	 || nesDiskCrc == 0x1e0c7ea3) {	// AD&D Dragons of Flame(J)
		header.flagsA |= NesDiskHeader::BatteryBackedRamFlagA;
	}
	if (nesDiskCrc == 0x1995ac4e) {	// Ferrari Grand Prix Challenge(J)
		nesRom[0x1F7AD] = 0xFF;
		nesRom[0x1F7BC] = 0x00;
	}

	if (nesDiskCrc == 0x20d22251) {	// Top rider(J)
		nesRom[0x1F17E] = 0xEA;
		nesRom[0x1F17F] = 0xEA;
	}

	if (nesDiskCrc == 0x11469ce3) {	// Viva! Las Vegas(J)
		nesVrom[0x0000] = 0x01;
	}

	if (nesDiskCrc == 0x3fccdc7b) {	// Baseball Star - Mezase Sankanou!!(J)
		nesRom[0x0F666] = 0x9D;
	}

	if (nesDiskCrc == 0xdb564628) {	// Mario Open Golf(J)
		nesRom[0x30195] = 0xC0;
	}

	// Mapper 002
	if (nesDiskCrc == 0x63af202f) {	// JJ - Tobidase Daisakusen Part 2(J)
		header.flagsA &= 0xf6;
		header.flagsA |= NesDiskHeader::VerticalFlagA;
	}

	if (nesDiskCrc == 0x99a62e47) {	// Black Bass 2, The(J)
		header.flagsA &= 0xf6;
		header.flagsA |= NesDiskHeader::VerticalFlagA;
	}

	if (nesDiskCrc == 0x0eaa7515		// Rod Land(J)
	 || nesDiskCrc == 0x22ab9694) {	// Rod Land(E)
		header.flagsA &= 0xf6;
		header.flagsA |= NesDiskHeader::VerticalFlagA;
	}

	if (nesDiskCrc == 0x2061772a) {	// Tantei Jinguji Taburou Tokino Sugiyukumamani (J)
		header.flagsA &= 0xf6;
		header.flagsA |= NesDiskHeader::VerticalFlagA;
	}

	// Mapper 003
	if (nesDiskCrc == 0x29401686) {	// Minna no Taabou no Nakayoshi Dai Sakusen(J)
	//	nesRom[0x2B3E] = 0x60;
	}
	if (nesDiskCrc == 0x932a077a) {	// TwinBee(J)
		nesMapperType = 87;
	}
	if (nesDiskCrc == 0x8218c637) {	// Space Hunter(J)
	//	header.m_flagsA &= 0xf6;
	//	header.m_flagsA |= NesDiskHeader::FourScreenFlagA;
		header.flagsA = NesDiskHeader::VerticalFlagA;
	}
	if (nesDiskCrc == 0x2bb6a0f8		// Sherlock Holmes - Hakushaku Reijou Yuukai Jiken(J)
	 || nesDiskCrc == 0x28c11d24		// Sukeban Deka 3(J)
	 || nesDiskCrc == 0x02863604) {	// Sukeban Deka 3(J)(Alt)
		header.flagsA &= 0xf6;
		header.flagsA |= NesDiskHeader::VerticalFlagA;
	}

	// Mapper 004
	if (nesDiskCrc == 0x58581770) {	// Rasaaru Ishii no Childs Quest(J)
		header.flagsA &= 0xf6;
		header.flagsA |= NesDiskHeader::VerticalFlagA;
	}
	if (nesDiskCrc == 0xf3feb3ab		// Kunio Kun no Jidaigeki Dayo Zenin Shuugou! (J)
	 || nesDiskCrc == 0xa524ae9b		// Otaku no Seiza - An Adventure in the Otaku Galaxy (J)
	 || nesDiskCrc == 0x46dc6e57		// SD Gundam - Gachapon Senshi 2 - Capsule Senki (J)
	 || nesDiskCrc == 0x66b2dec7		// SD Gundam - Gachapon Senshi 3 - Eiyuu Senki (J)
	 || nesDiskCrc == 0x92b07fd9		// SD Gundam - Gachapon Senshi 4 - New Type Story (J)
	 || nesDiskCrc == 0x8ee6463a		// SD Gundam - Gachapon Senshi 5 - Battle of Universal Century (J)
	 || nesDiskCrc == 0xaf754426		// Ultraman Club 3 (J)
	 || nesDiskCrc == 0xfe4e5b11		// Ushio to Tora - Shinen no Daiyou (J)
	 || nesDiskCrc == 0x57c12c17) {	// Yamamura Misa Suspense - Kyouto Zaiteku Satsujin Jiken (J)
		header.flagsA |= NesDiskHeader::BatteryBackedRamFlagA;
	}
	if (nesDiskCrc == 0x42e03e4a) {	// RPG Jinsei Game (J)
		nesMapperType = 118;
		header.flagsA |= NesDiskHeader::BatteryBackedRamFlagA;
	}
	if (nesDiskCrc == 0xfd0299c3) {	// METAL MAX(J)
		nesRom[0x3D522] = 0xA9;
		nesRom[0x3D523] = 0x19;
	}
	if (nesDiskCrc == 0x1d2e5018		// Rockman 3(J)
	 || nesDiskCrc == 0x6b999aaf) {	// Mega Man 3(U)
	//	nesRom[0x3C179] = 0xBA;//
	//	nesRom[0x3C9CC] = 0x9E;
	}

	// Mapper 005
	if (nesDiskCrc == 0xe91548d8) {	// Shin 4 Nin Uchi Mahjong - Yakuman Tengoku (J)
		header.flagsA |= NesDiskHeader::BatteryBackedRamFlagA;
	}

	if (nesDiskCrc == 0x255b129c) {	// Gun Sight (J) / Gun Sight (J)[a1]
		nesRom[0x02D0B] = 0x01;
		nesRom[0x0BEC0] = 0x01;
	}


	// Mapper 010
	if (nesDiskCrc == 0xc9cce8f2) {	// Famicom Wars (J)
		header.flagsA |= NesDiskHeader::BatteryBackedRamFlagA;
	}

	// Mapper 016
	if (nesDiskCrc == 0x983d8175		// Datach - Battle Rush - Build Up Robot Tournament (J)
	 || nesDiskCrc == 0x894efdbc		// Datach - Crayon Shin Chan - Ora to Poi Poi (J)
	 || nesDiskCrc == 0x19e81461		// Datach - Dragon Ball Z - Gekitou Tenkaichi Budou Kai (J)
	 || nesDiskCrc == 0xbe06853f		// Datach - J League Super Top Players (J)
	 || nesDiskCrc == 0x0be0a328		// Datach - SD Gundam - Gundam Wars (J)
	 || nesDiskCrc == 0x5b457641		// Datach - Ultraman Club - Supokon Fight! (J)
	 || nesDiskCrc == 0xf51a7f46		// Datach - Yuu Yuu Hakusho - Bakutou Ankoku Bujutsu Kai (J)
	 || nesDiskCrc == 0x31cd9903		// Dragon Ball Z - Kyoushuu! Saiya Jin (J)
	 || nesDiskCrc == 0xe49fc53e		// Dragon Ball Z 2 - Gekishin Freeza!! (J)
	 || nesDiskCrc == 0x09499f4d		// Dragon Ball Z 3 - Ressen Jinzou Ningen (J)
	 || nesDiskCrc == 0x2e991109		// Dragon Ball Z Gaiden - Saiya Jin Zetsumetsu Keikaku (J)
	 || nesDiskCrc == 0x170250de) {	// Rokudenashi Blues(J)
		header.flagsA |= NesDiskHeader::BatteryBackedRamFlagA;
	}

	// Mapper 019
	if (nesDiskCrc == 0x3296ff7a		// Battle Fleet (J)
	 || nesDiskCrc == 0x429fd177		// Famista '90 (J)
	 || nesDiskCrc == 0xdd454208		// Hydlide 3 - Yami Kara no Houmonsha (J)
	 || nesDiskCrc == 0xb1b9e187		// Kaijuu Monogatari (J)
	 || nesDiskCrc == 0xaf15338f) {	// Mindseeker (J)
		header.flagsA |= NesDiskHeader::BatteryBackedRamFlagA;
	}

	// Mapper 026
	if (nesDiskCrc == 0x836cc1ab) {	// Mouryou Senki Madara (J)
		header.flagsA |= NesDiskHeader::BatteryBackedRamFlagA;
	}

	// Mapper 033
	if (nesDiskCrc == 0x547e6cc1) {	// Flintstones - The Rescue of Dino & Hoppy, The(J)
		nesMapperType = 48;
	}

	// Mapper 065
	if (nesDiskCrc == 0xfd3fc292) {	// Ai Sensei no Oshiete - Watashi no Hoshi (J)
		nesMapperType = 32;
	}

	// Mapper 068
	if (nesDiskCrc == 0xfde79681) {	// Maharaja (J)
		header.flagsA |= NesDiskHeader::BatteryBackedRamFlagA;
	}

	// Mapper 069
	if (nesDiskCrc == 0xfeac6916		// Honoo no Toukyuuji - Dodge Danpei 2(J)
	 || nesDiskCrc == 0x67898319) {	// Barcode World(J)
		header.flagsA |= NesDiskHeader::BatteryBackedRamFlagA;
	}

	// Mapper 080
	if (nesDiskCrc == 0x95aaed34		// Mirai Shinwa Jarvas (J)
	 || nesDiskCrc == 0x17627d4b) {	// Taito Grand Prix - Eikou heno License (J)
		header.flagsA |= NesDiskHeader::BatteryBackedRamFlagA;
	}

	// Mapper 082
	if (nesDiskCrc == 0x4819a595) {	// Kyuukyoku Harikiri Stadium - Heisei Gannen Ban (J)
		header.flagsA |= NesDiskHeader::BatteryBackedRamFlagA;
	}

	// Mapper 086
	if (nesDiskCrc == 0xe63f7d0b) {	// Urusei Yatsura - Lum no Wedding Bell(J)
		nesMapperType = 101;
	}

	// Mapper 118
	if (nesDiskCrc == 0x3b0fb600) {	// Ys 3 - Wonderers From Ys (J)
		header.flagsA |= NesDiskHeader::BatteryBackedRamFlagA;
	}

	// Mapper 180
	if (nesDiskCrc == 0xc68363f6) {	// Crazy Climber(J)
		header.flagsA &= 0xf6;
	}

	// VS-Unisystem
	if (nesDiskCrc == 0x70901b25) {	// VS Slalom
		nesMapperType = 99;
	}

	if (nesDiskCrc == 0xd5d7eac4) {	// VS Dr. Mario
		nesMapperType = 1;
		header.flagsB |= NesDiskHeader::VSSystemFlagB;
	}

	if (nesDiskCrc == 0xffbef374		// VS Castlevania
	 || nesDiskCrc == 0x8c0c2df5) {	// VS Top Gun
		nesMapperType = 2;
		header.flagsB |= NesDiskHeader::VSSystemFlagB;
	}

	if (nesDiskCrc == 0xeb2dba63		// VS TKO Boxing
	 || nesDiskCrc == 0x98cfe016		// VS TKO Boxing (Alt)
	 || nesDiskCrc == 0x9818f656) {	// VS TKO Boxing (f1)
		nesMapperType = 4;
		header.flagsB |= NesDiskHeader::VSSystemFlagB;
	}

	if (nesDiskCrc == 0x135adf7c) {	// VS Atari RBI Baseball
		nesMapperType = 4;
		header.flagsB |= NesDiskHeader::VSSystemFlagB;
	}

	if (nesDiskCrc == 0xf9d3b0a3		// VS Super Xevious
	 || nesDiskCrc == 0x9924980a		// VS Super Xevious (b1)
	 || nesDiskCrc == 0x66bb838f) {	// VS Super Xevious (b2)
		nesMapperType = 4;
		header.flagsA &= 0xF6;
		header.flagsB |= NesDiskHeader::VSSystemFlagB;
	}

	if (nesDiskCrc == 0x17ae56be) {	// VS Freedom Force
		nesMapperType = 4;
		header.flagsA &= 0xF6;
		header.flagsA |= NesDiskHeader::FourScreenFlagA;
		header.flagsB |= NesDiskHeader::VSSystemFlagB;
	}

	if (nesDiskCrc == 0xe2c0a2be) {	// VS Platoon
		nesMapperType = 68;
		header.flagsB |= NesDiskHeader::VSSystemFlagB;
	}

	if (nesDiskCrc == 0xcbe85490		// VS Excitebike
	 || nesDiskCrc == 0x29155e0c		// VS Excitebike (Alt)
	 || nesDiskCrc == 0xff5135a3) {	// VS Hogan's Alley
		header.flagsA &= 0xF6;
		header.flagsA |= NesDiskHeader::FourScreenFlagA;
	}

	if (nesDiskCrc == 0x0b65a917) {	// VS Mach Rider(Endurance Course)
		nesRom[0x7FDF] = 0xFF;
		nesRom[0x7FE4] = 0x00;
	}

	if (nesDiskCrc == 0x8a6a9848		// VS Mach Rider(Endurance Course)(Alt)
	 || nesDiskCrc == 0xae8063ef) {	// VS Mach Rider(Japan, Fighting Course)
		nesRom[0x7FDD] = 0xFF;
		nesRom[0x7FE2] = 0x00;
	}

	if (nesDiskCrc == 0x16d3f469) {	// VS Ninja Jajamaru Kun (J)
		header.flagsA &= 0xf6;
		header.flagsA |= NesDiskHeader::VerticalFlagA;
	}

	if (nesDiskCrc == 0xc99ec059) {	// VS Raid on Bungeling Bay(J)
		nesMapperType = 99;
		header.flagsA &= 0xF6;
		header.flagsA |= NesDiskHeader::FourScreenFlagA;
	}
	if (nesDiskCrc == 0xca85e56d) {	// VS Mighty Bomb Jack(J)
		nesMapperType = 99;
		header.flagsA &= 0xF6;
		header.flagsA |= NesDiskHeader::FourScreenFlagA;
	}


	if (nesDiskCrc == 0xeb2dba63		// VS TKO Boxing
	 || nesDiskCrc == 0x9818f656		// VS TKO Boxing
	 || nesDiskCrc == 0xed588f00		// VS Duck Hunt
	 || nesDiskCrc == 0x8c0c2df5		// VS Top Gun
	 || nesDiskCrc == 0x16d3f469		// VS Ninja Jajamaru Kun
	 || nesDiskCrc == 0x8850924b		// VS Tetris
	 || nesDiskCrc == 0xcf36261e		// VS Sky Kid
	 || nesDiskCrc == 0xe1aa8214		// VS Star Luster
	 || nesDiskCrc == 0xec461db9		// VS Pinball
	 || nesDiskCrc == 0xe528f651		// VS Pinball (alt)
	 || nesDiskCrc == 0x17ae56be		// VS Freedom Force
	 || nesDiskCrc == 0xe2c0a2be		// VS Platoon
	 || nesDiskCrc == 0xff5135a3		// VS Hogan's Alley
	 || nesDiskCrc == 0x70901b25		// VS Slalom
	 || nesDiskCrc == 0x0b65a917		// VS Mach Rider(Endurance Course)
	 || nesDiskCrc == 0x8a6a9848		// VS Mach Rider(Endurance Course)(Alt)
	 || nesDiskCrc == 0xae8063ef		// VS Mach Rider(Japan, Fighting Course)
	 || nesDiskCrc == 0xcc2c4b5d		// VS Golf
	 || nesDiskCrc == 0xa93a5aee		// VS Stroke and Match Golf
	 || nesDiskCrc == 0x86167220		// VS Lady Golf
	 || nesDiskCrc == 0xffbef374		// VS Castlevania
	 || nesDiskCrc == 0x135adf7c		// VS Atari RBI Baseball
	 || nesDiskCrc == 0xd5d7eac4		// VS Dr. Mario
	 || nesDiskCrc == 0x46914e3e		// VS Soccer
	 || nesDiskCrc == 0x70433f2c		// VS Battle City
	 || nesDiskCrc == 0x8d15a6e6		// VS bad .nes
	 || nesDiskCrc == 0x1e438d52		// VS Goonies
	 || nesDiskCrc == 0xcbe85490		// VS Excitebike
	 || nesDiskCrc == 0x29155e0c		// VS Excitebike (alt)
	 || nesDiskCrc == 0x07138c06		// VS Clu Clu Land
	 || nesDiskCrc == 0x43a357ef		// VS Ice Climber
	 || nesDiskCrc == 0x737dd1bf		// VS Super Mario Bros
	 || nesDiskCrc == 0x4bf3972d		// VS Super Mario Bros
	 || nesDiskCrc == 0x8b60cc58		// VS Super Mario Bros
	 || nesDiskCrc == 0x8192c804		// VS Super Mario Bros
	 || nesDiskCrc == 0xd99a2087		// VS Gradius
	 || nesDiskCrc == 0xf9d3b0a3		// VS Super Xevious
	 || nesDiskCrc == 0x9924980a		// VS Super Xevious
	 || nesDiskCrc == 0x66bb838f		// VS Super Xevious
	 || nesDiskCrc == 0xc99ec059		// VS Raid on Bungeling Bay(J)
	 || nesDiskCrc == 0xca85e56d) {	// VS Mighty Bomb Jack(J)
		header.flagsB |= NesDiskHeader::VSSystemFlagB;
	}

	if (nesMapperType == 99 || nesMapperType == 151) {
		header.flagsB |= NesDiskHeader::VSSystemFlagB;
	}
}
