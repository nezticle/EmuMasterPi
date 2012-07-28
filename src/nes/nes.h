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

#ifndef NESEMU_H
#define NESEMU_H

#include <base/emu.h>
class NesPpu;

enum SystemType { NES_NTSC, NES_PAL };

class NesEmu : public Emu
{
	Q_OBJECT
	Q_PROPERTY(QObject *ppu READ ppu CONSTANT)
	Q_PROPERTY(QObject *cheats READ cheats CONSTANT)
	Q_PROPERTY(QString diskInfo READ diskInfo CONSTANT)
public:
	enum RenderMethod {
		PostAllRender,
		PreAllRender,
		PostRender,
		PreRender,
		TileRender
	};

	NesEmu();
	bool init(const QString &diskPath, QString *error);
	void shutdown();
	void reset();
	void resume();

	const QImage &frame() const;
	void emulateFrame(bool drawEnabled);
	int fillAudioBuffer(char *stream, int streamSize);

	QObject *ppu() const;
	QObject *pad() const;
	QObject *cheats() const;
	QString diskInfo() const;

	qreal systemFrameRate() const;
	qreal baseClock() const;
	int clockDividerForCpu() const;
	int clockDividerForPpu() const;
protected:
	void sl();
};

extern void nesEmuSetRenderMethod(NesEmu::RenderMethod renderMethod);

extern NesEmu nesEmu;
extern SystemType nesSystemType;
extern NesEmu::RenderMethod nesEmuRenderMethod;

#endif // NESEMU_H
