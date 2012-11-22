/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "emuview.h"
#include "emu.h"
#include "configuration.h"
#include "emuthread.h"
#include "hostvideo.h"
#include "hostaudio.h"
#include "hostinput.h"
#include "statelistmodel.h"
#include "pathmanager.h"
#include "hostinputdevice.h"
#include "stringlistproxy.h"
#include <QtGui/QCloseEvent>
#include <QtGui/QGuiApplication>
#include <QtCore/QTimer>
#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtGui/QColor>
#include <QtCore/QDebug>

EmuView::EmuView(Emu *emu, const QString &diskFileName) :
	m_emu(emu),
	m_diskFileName(diskFileName),
	m_running(false),
	m_backgroundCounter(qAbs(qrand())/2),
	m_quit(false),
	m_pauseRequested(false),
	m_slotToBeLoadedOnStart(StateListModel::InvalidSlot),
	m_audioEnable(true),
	m_autoSaveLoadEnable(true),
	m_safetyTimerDisabled(false),
	m_runInBackground(false),
    m_lrButtonsVisible(false),
    m_frameSkip(0)
{
	Q_ASSERT(m_emu != 0);

	Configuration::setupAppInfo();
	pathManager.setCurrentEmu(m_emu->name());

	m_hostInput = new HostInput(m_emu);
	m_hostAudio = new HostAudio(m_emu);
    m_hostVideo = new HostVideo(m_hostInput, m_emu);
	m_hostVideo->resize(HostVideo::Width, HostVideo::Height);
	m_hostVideo->installEventFilter(m_hostInput);
	QObject::connect(m_hostInput, SIGNAL(quit()), SLOT(close()));
	QObject::connect(m_hostInput, SIGNAL(pause()), SLOT(pause()));
	QObject::connect(m_hostInput, SIGNAL(devicesChanged()),
					 SIGNAL(inputDevicesChanged()));
	QObject::connect(m_hostVideo, SIGNAL(shaderChanged()),
					 SLOT(hostVideoShaderChanged()));

	m_stateListModel = new StateListModel(m_emu, m_diskFileName);

	if (!loadConfiguration())
		m_error = constructSlErrorString();

	// TODO detect if USB mass storage is used

	if (m_error.isEmpty()) {
		QString diskPath = QString("%1/%2")
				.arg(pathManager.diskDirPath())
				.arg(m_diskFileName);
		bool ok = m_emu->init(diskPath, &m_error);
		if (!ok && m_error.isEmpty())
			m_error = tr("Unknown error!");
	}

	m_safetyTimer = new QTimer(this);
	m_safetyTimer->setInterval(10000);
	m_safetyTimer->setSingleShot(false);
	QObject::connect(m_safetyTimer, SIGNAL(timeout()), SLOT(onSafetyEvent()));

    if (m_emu->frameRate())
        m_updateTimer.setInterval(1000 / m_emu->frameRate());
    else
        m_updateTimer.setInterval(16);
    m_frameTime.start();
    connect(&m_updateTimer, SIGNAL(timeout()), SLOT(update()));

	const char *method = "showEmulationView";
	QMetaObject::invokeMethod(this, method, Qt::QueuedConnection);
}

EmuView::~EmuView()
{
	if (m_error.isEmpty()) {
		if (m_autoSaveLoadEnable)
			m_stateListModel->saveState(StateListModel::AutoSaveLoadSlot);
		// auto save screenshot
		saveScreenShotIfNotExists();
		m_emu->shutdown();
	}
	delete m_stateListModel;
	delete m_hostVideo;
	delete m_hostAudio;
	delete m_hostInput;
}

// two-stage pause preventing deadlocks
void EmuView::pause()
{
	if (!m_running || m_pauseRequested)
		return;
	m_closeTries = 0;
    m_running = false;
    m_updateTimer.stop();
    m_emu->setRunning(false);

	m_safetyTimer->stop();
}

void EmuView::resume()
{
    //Q_ASSERT(m_error.isEmpty());
    if (!m_error.isEmpty())
    {
        qDebug() << "Error: " << m_error;
    }

	if (m_running)
		return;

	if (!m_safetyTimerDisabled) {
		m_safetyCheck = false;
		m_safetyTimer->start();
	}

	if (m_slotToBeLoadedOnStart != StateListModel::InvalidSlot)
		QTimer::singleShot(1000, this, SLOT(pause()));

    m_updateTimer.start();
    m_emu->setRunning(true);
	m_running = true;
}

bool EmuView::close()
{
	m_quit = true;
	if (m_running) {
		pause();
		return false;
	} else {
		qApp->quit();
		return true;
	}
}

void EmuView::showEmulationView()
{
	if (!m_running) {
		m_hostVideo->setVisible(true);
		if (m_audioEnable)
			m_hostAudio->open();
		resume();
	}
}

void EmuView::saveScreenShotIfNotExists()
{
	QString diskTitle = QFileInfo(m_diskFileName).completeBaseName();
	QString path = pathManager.screenShotPath(diskTitle);
	if (!QFile::exists(path))
		saveScreenShot();
}

void EmuView::saveScreenShot()
{
	QImage img = m_emu->frame().copy(m_emu->videoSrcRect().toRect());
	img = img.convertToFormat(QImage::Format_ARGB32);
	QString diskTitle = QFileInfo(m_diskFileName).completeBaseName();
	img.save(pathManager.screenShotPath(diskTitle));
}

QString EmuView::extractArg(const QStringList &args,
								const QString &argName)
{
	int optionArgIndex = args.indexOf(argName);
	if (optionArgIndex >= 0) {
		if (args.size() > optionArgIndex+1)
			return args.at(optionArgIndex+1);
	}
	return QString();
}

void EmuView::parseConfArg(const QString &arg)
{
	QStringList lines = arg.split(',', QString::SkipEmptyParts);
	foreach (QString line, lines) {
		QStringList lineSplitted = line.split('=', QString::SkipEmptyParts);
		if (lineSplitted.size() != 2) {
			qDebug("Unknown conf option: %s", qPrintable(line));
		} else {
			QString key = lineSplitted.at(0);
			QString value = lineSplitted.at(1);
			emConf.setValue(key, value);
		}
	}
}

void EmuView::onFrameGenerated(bool videoOn)
{
    QTime benchmarkTime;
    benchmarkTime.start();
    m_emu->emulateFrame(videoOn);
    //qDebug("%dms to render frame", benchmarkTime.elapsed());
    benchmarkTime.restart();
	m_safetyCheck = true;
    if (m_audioEnable) {
		m_hostAudio->sendFrame();
   //     qDebug("%dms to process audio", benchmarkTime.elapsed());
        benchmarkTime.restart();
    }
    if (videoOn) {
        m_hostVideo->repaint();
    //    qDebug("%dms to paint to screen", benchmarkTime.elapsed());
        benchmarkTime.restart();
    }
	// sync input with the emulation
	m_hostInput->sync();
    //qDebug("%dms to process input", benchmarkTime.elapsed());
}

int EmuView::determineLoadSlot(const QStringList &args)
{
	int slot = StateListModel::InvalidSlot;

	// check if auto load if forced to the state specified by an argument
	if (args.contains("-autoSaveLoadEnable"))
		m_autoSaveLoadEnable = true;
	else if (args.contains("-autoSaveLoadDisable"))
		m_autoSaveLoadEnable = false;

	if (m_autoSaveLoadEnable)
		slot = StateListModel::AutoSaveLoadSlot;

	QString stateArg = extractArg(args, "-state");
	if (!stateArg.isEmpty()) {
		bool ok;
		slot = stateArg.toInt(&ok);
		if (!ok) {
			qDebug("invalid -state arg passed: %s", qPrintable(stateArg));
			slot = StateListModel::InvalidSlot;
		}
	}
	if (slot != StateListModel::InvalidSlot) {
		if (!m_stateListModel->exists(slot)) {
			if (slot != StateListModel::AutoSaveLoadSlot)
				qDebug("slot passed by -state arg not found: %s", qPrintable(stateArg));
			slot = StateListModel::InvalidSlot;
		}
	}
	return slot;
}

bool EmuView::loadConfiguration()
{
	QSettings s;
	m_autoSaveLoadEnable = s.value("autoSaveLoadEnable").toBool();

	emConf.setValue("version", QCoreApplication::applicationVersion());

	QStringList args = QCoreApplication::arguments();

	m_slotToBeLoadedOnStart = determineLoadSlot(args);

	// load conf from state
	if (m_slotToBeLoadedOnStart != StateListModel::InvalidSlot) {
		emsl.loadConfOnly = true;
		if (!m_stateListModel->loadState(m_slotToBeLoadedOnStart))
			return false;
		emsl.loadConfOnly = false;
	}

	// load conf from arg
	QString confArg = extractArg(args, "-conf");
	parseConfArg(confArg);

	// load conf from global settings
	finishSetupConfiguration();

	return true;
}

void EmuView::onSlFailed()
{
	if (!emsl.save && emsl.abortIfLoadFails) {
		fatalError(constructSlErrorString());
	} else {
		emit faultOccured(constructSlErrorString());
	}
}

QString EmuView::constructSlErrorString() const
{
	QString result;
	if (emsl.save)
		result = tr("Saving state failed: ");
	else
		result = tr("Loading state failed: ");
	result += emsl.error;
	return result;
}

void EmuView::fatalError(const QString &errorStr)
{
    m_error = errorStr;
}

QList<QObject *> EmuView::inputDevices() const
{
	QList<QObject *> ret;
	ret.reserve(m_hostInput->devices().size());
	foreach (HostInputDevice *dev, m_hostInput->devices())
		ret.append(dev);
	return ret;
}

void EmuView::onSafetyEvent()
{
	if (!m_safetyCheck) {
		fatalError(tr("Emulated system is not responding"));
	}
	m_safetyCheck = false;
}

void EmuView::onStateLoaded()
{
	m_hostInput->loadFromConf();
}


//-------------------------------SETTINGS SECTION-------------------------------

void EmuView::finishSetupConfiguration()
{
	QSettings s;
	m_hostVideo->setFpsVisible(loadOptionFromSettings(s, "fpsVisible").toBool());
	m_hostVideo->setKeepAspectRatio(loadOptionFromSettings(s, "keepAspectRatio").toBool());
    m_hostVideo->setShader(loadOptionFromSettings(s, "videoFilter").toString());
	setAudioEnabled(loadOptionFromSettings(s, "audioEnable").toBool());
	m_runInBackground = loadOptionFromSettings(s, "runInBackground").toBool();
	setLRButtonsVisible(loadOptionFromSettings(s, "lrButtonsVisible").toBool());
    m_hostInput->loadFromConf();
}

QVariant EmuView::loadOptionFromSettings(QSettings &s, const QString &name) const
{
	QVariant option = emConf.value(name);
	if (option.isNull())
		option = s.value(name, emConf.defaultValue(name));
	return option;
}

void EmuView::setFpsVisible(bool on)
{
	if (m_hostVideo->isFpsVisible() != on) {
		m_hostVideo->setFpsVisible(on);
		emConf.setValue("fpsVisible", on);
		emit fpsVisibleChanged();
	}
}

bool EmuView::isFpsVisible() const
{
	return m_hostVideo->isFpsVisible();
}

void EmuView::setFrameSkip(int n)
{
    m_frameSkip = n;
    emConf.setValue("frameSkip", n);
    emit frameSkipChanged();
}

int EmuView::frameSkip() const
{
    return m_frameSkip;
}

void EmuView::setAudioEnabled(bool on)
{
	if (m_audioEnable != on) {
		m_audioEnable = on;
		m_emu->setAudioEnabled(on);
		emConf.setValue("audioEnable", on);
		emit audioEnableChanged();
	}
}

bool EmuView::isAudioEnabled() const
{
	return m_audioEnable;
}

void EmuView::setKeepAspectRatio(bool on)
{
	if (m_hostVideo->keepApsectRatio() != on) {
		m_hostVideo->setKeepAspectRatio(on);
		emConf.setValue("keepAspectRatio", on);
		emit keepAspectRatioChanged();
	}
}

bool EmuView::keepAspectRatio() const
{
	return m_hostVideo->keepApsectRatio();
}

void EmuView::setLRButtonsVisible(bool on)
{
	if (m_lrButtonsVisible != on) {
		if (m_emu->name() != "nes" && m_emu->name() != "amiga") {
			m_lrButtonsVisible = on;
			emConf.setValue("lrButtonsVisible", on);
			emit lrButtonsVisibleChanged();
		}
	}
}

bool EmuView::areLRButtonsVisible() const
{
	return m_lrButtonsVisible;
}

void EmuView::setVideoFilter(const QString &name)
{
	m_hostVideo->setShader(name);
}

QString EmuView::videoFilter() const
{
	return m_hostVideo->shader();
}

QStringList EmuView::availableVideoFilters() const
{
	return m_hostVideo->shaderList();
}

void EmuView::disableSafetyTimer()
{
	m_safetyTimerDisabled = true;
	m_safetyTimer->stop();
}

void EmuView::hostVideoShaderChanged()
{
	emConf.setValue("videoFilter", m_hostVideo->shader());
    emit videoFilterChanged();
}

void EmuView::update()
{
    static int frameCounter = 0;
    //qDebug("%dms since last frame", m_frameTime.elapsed());
    m_frameTime.restart();
    //If not paused
    if(!m_running)
        return;
    //If framecount < frameskip
    QTime benchmarkTime;
    benchmarkTime.start();
    onFrameGenerated(frameCounter == 0);
    //qDebug("%dms to update", benchmarkTime.elapsed());

    if (++frameCounter > m_frameSkip)
        frameCounter = 0;
}
