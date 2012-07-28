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

#include "nes.h"
#include "sync.h"
#include "cpuint.h"
#include "cpurec.h"
#include "ppu.h"
#include "apu.h"
#include "disk.h"
#include "input.h"
#include "inputzapper.h"
#include "mapper.h"
#include "cheats.h"
#if defined(ENABLE_DEBUGGING)
#include "debug.h"
#endif
#include <base/emuview.h>
#include <base/configuration.h>
#include <QtCore/QSettings>
#include <QtGui/QGuiApplication>

NesEmu nesEmu;
SystemType nesSystemType;
NesEmu::RenderMethod nesEmuRenderMethod;

static const char *tvSystemConfName = "nes.tvSystem";
static const char *renderMethodConfName = "nes.renderMethod";
static const char *extraInputConfName = "nes.extraInput";
static const char *cpuTypeConfName = "nes.cpuType";

static QVariant forcedRenderMethod;

static void setupTvEncodingSystem(const QString &path)
{
	// detect system name by checking file name
	if (path.contains("(E)"))
		nesSystemType = NES_PAL;
	else
		nesSystemType = NES_NTSC;

	// check for forced tv system
	QVariant forcedSystemType = emConf.value(tvSystemConfName);
	if (!forcedSystemType.isNull()) {
		QString typeStr = forcedSystemType.toString();
		if (typeStr == "NTSC")
			nesSystemType = NES_NTSC;
		else if (typeStr == "PAL")
			nesSystemType = NES_PAL;
		else
			qDebug("Unknown TV system set: %s", qPrintable(typeStr));
	}
	// set value in conf in case of auto-detection (needed for load check)
	if (nesSystemType == NES_NTSC)
		emConf.setValue(tvSystemConfName, "NTSC");
	else
		emConf.setValue(tvSystemConfName, "PAL");
}

static bool slCheckTvEncodingSystem()
{
	bool ok = false;
	// check if TV system used in the saved state is the same as current
	QVariant forcedSystemType = emConf.value(tvSystemConfName);
	if (!forcedSystemType.isNull()) {
		QString typeStr = forcedSystemType.toString();
		SystemType slSys = (typeStr == "PAL") ? NES_PAL : NES_NTSC;
		ok = (slSys == nesSystemType);
	}
	if (!ok) {
		emsl.error = QString("%1 \"%2\"").arg(EM_MSG_STATE_DIFFERS)
				.arg(tvSystemConfName);
	}
	return ok;
}

static void setupRenderMethod()
{
	nesEmuSetRenderMethod(NesEmu::PreRender);
	// check for forced tv system
	forcedRenderMethod = emConf.value(renderMethodConfName);
	if (!forcedRenderMethod.isNull()) {
		bool ok;
		int method = forcedRenderMethod.toInt(&ok);
		ok = (ok && method >= NesEmu::PostAllRender && method <= NesEmu::TileRender);
		if (!ok) {
			qDebug("Unknown render method passed");
			forcedRenderMethod = QVariant();
		} else {
			nesEmuRenderMethod = static_cast<NesEmu::RenderMethod>(method);
		}
	}
}

static void setupExtraInputDevice()
{
	// check for forced extra input device
	QVariant extraInput = emConf.value(extraInputConfName);
	if (!extraInput.isNull()) {
		bool ok;
		int device = extraInput.toInt(&ok);
		ok = (ok && device >= NesInput::Zapper && device <= NesInput::Paddle);
		if (!ok) {
			qDebug("Unknown extra input device passed");
		} else {
			nesInput.setExtraDevice(static_cast<NesInput::ExtraDevice>(device));
		}
	}
}

static void setupCpu()
{
	nesCpu = &nesCpuInterpreter;
	QVariant cpuType = emConf.value(cpuTypeConfName);
	if (!cpuType.isNull()) {
		QString cpuString = cpuType.toString();
		if (cpuString == "recompiler") {
			nesCpu = &nesCpuRecompiler;
		} else if (cpuString == "interpreter") {
			nesCpu = &nesCpuInterpreter;
		} else {
			qDebug("Unknown CPU type passed");
		}
	}
}

NesEmu::NesEmu() :
	Emu("nes")
{
}

bool NesEmu::init(const QString &diskPath, QString *error)
{
	setupRenderMethod();

	if (!nesDiskLoad(diskPath, error)) {
		*error = QString("%1 (%2)").arg(EM_MSG_DISK_LOAD_FAILED).arg(*error);
		return false;
	}

	setupTvEncodingSystem(diskPath);

	// set cpu to interpreter here to not handle bank switching by
	// the recompiler as it is not initialized yet
	nesCpu = &nesCpuInterpreter;
	nesMapper = NesMapper::create(nesMapperType);
	if (!nesMapper) {
		*error = QString("Mapper %1 is not supported").arg(nesMapperType);
		return false;
	}
	// reset mapper to fill it with functions and data needed for further
	// processing
	nesMapper->reset();

	setVideoSrcRect(QRect(8, 0, NesPpu::VisibleScreenWidth, NesPpu::VisibleScreenHeight));
	setFrameRate(systemFrameRate());
	setupExtraInputDevice();

	setupCpu();
	if (!nesCpu->init(error))
		return false;
	nesApuInit();
	nesPpuInit();
	if (!nesSyncInit(error))
		return false;
	reset();

#if defined(ENABLE_DEBUGGING)
	nesDebugInit();
#endif
	return true;
}

void NesEmu::shutdown()
{
	// sync shutdown must be first, because it needs to pass through
	// the semaphores and quit from cpu loop
	nesSyncShutdown();
	nesCpu->shutdown();
	delete nesMapper;
	nesDiskShutdown();
#if defined(ENABLE_DEBUGGING)
	nesDebugShutdown();
#endif
	nesPpuFrame = QImage();
}

void NesEmu::reset()
{
	nesSyncReset();
	nesMapper->reset();
	nesCpu->reset();
	nesApuReset();
	nesInputReset();
}

void NesEmu::resume()
{
	nesApuSetOutputEnabled(isAudioEnabled());
}

void nesEmuSetRenderMethod(NesEmu::RenderMethod renderMethod)
{
	if (forcedRenderMethod.isNull())
		nesEmuRenderMethod = renderMethod;
}

void NesEmu::emulateFrame(bool drawEnabled)
{
	nesSyncFrame(drawEnabled);
}

const QImage &NesEmu::frame() const
{
	return nesPpuFrame;
}

int NesEmu::fillAudioBuffer(char *stream, int streamSize)
{
	return nesApuFillBuffer(stream, streamSize);
}

QObject *NesEmu::ppu() const
{
	return &nesPpu;
}

QObject *NesEmu::cheats() const
{
	return &nesCheats;
}

QString NesEmu::diskInfo() const
{
	return QString("CRC: %1, Mapper %2 (%3), PROM %4KB, VROM %5KB").arg(nesDiskCrc, 8, 16)
			.arg(nesMapperType).arg(nesMapper->name()).arg(nesRomSize16KB*16)
			.arg(nesVromSize1KB);
}

qreal NesEmu::systemFrameRate() const
{
	return (nesSystemType == NES_NTSC) ? 60.0f : 50.0f;
}

qreal NesEmu::baseClock() const
{
	return (nesSystemType == NES_NTSC) ? 21477272.0f : 26601712.0f;
}

int NesEmu::clockDividerForCpu() const
{
	return (nesSystemType == NES_NTSC) ? 12 : 16;
}

int NesEmu::clockDividerForPpu() const
{
	return (nesSystemType == NES_NTSC) ? 4 : 5;
}

void NesEmu::sl()
{
	if (!slCheckTvEncodingSystem())
		return;
	nesMapper->sl();
	nesCpu->sl();
	nesPpuSl();
	nesApuSl();
	nesInput.sl();
	nesSyncSl();
	nesCheats.sl();
}

int main(int argc, char *argv[])
{
	if (argc < 2)
		return -1;
    QGuiApplication app(argc, argv);
	EmuView view(&nesEmu, argv[1]);
#if defined(ENABLE_DEBUGGING)
	view.disableSafetyTimer();
#endif
	return app.exec();
}
