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

#ifndef EMUVIEW_H
#define EMUVIEW_H

class Emu;
class EmuThread;
class HostVideo;
class HostAudio;
class HostInput;
class SettingsView;
class StateListModel;
class HostInputDevice;
#include "base_global.h"
#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QStringList>
#include <QtCore/QTime>
class QThread;
class QSettings;

class EmuView : public QObject
{
	Q_OBJECT
	Q_PROPERTY(bool fpsVisible READ isFpsVisible WRITE setFpsVisible NOTIFY fpsVisibleChanged)
	Q_PROPERTY(int frameSkip READ frameSkip WRITE setFrameSkip NOTIFY frameSkipChanged)
	Q_PROPERTY(bool audioEnable READ isAudioEnabled WRITE setAudioEnabled NOTIFY audioEnableChanged)
	Q_PROPERTY(bool keepAspectRatio READ keepAspectRatio WRITE setKeepAspectRatio NOTIFY keepAspectRatioChanged)
	Q_PROPERTY(bool lrButtonsVisible READ areLRButtonsVisible WRITE setLRButtonsVisible NOTIFY lrButtonsVisibleChanged)
	Q_PROPERTY(QString error READ error CONSTANT)
	Q_PROPERTY(QList<QObject *> inputDevices READ inputDevices NOTIFY inputDevicesChanged)
	Q_PROPERTY(QString videoFilter READ videoFilter WRITE setVideoFilter NOTIFY videoFilterChanged)
	Q_PROPERTY(QStringList availableVideoFilters READ availableVideoFilters CONSTANT)
public:
	explicit EmuView(Emu *emu, const QString &diskFileName);
	~EmuView();

	void setFpsVisible(bool on);
	bool isFpsVisible() const;

	void setFrameSkip(int n);
	int frameSkip() const;

	void setAudioEnabled(bool on);
	bool isAudioEnabled() const;

	void setKeepAspectRatio(bool on);
	bool keepAspectRatio() const;

	void setLRButtonsVisible(bool on);
	bool areLRButtonsVisible() const;

	QList<QObject *> inputDevices() const;

	QString error() const;

	void setVideoFilter(const QString &name);
	QString videoFilter() const;
	QStringList availableVideoFilters() const;

	void disableSafetyTimer();
public slots:
	bool close();

	void saveScreenShot();
	void showEmulationView();
signals:
	void fpsVisibleChanged();
	void frameSkipChanged();
	void audioEnableChanged();
	void keepAspectRatioChanged();
	void lrButtonsVisibleChanged();
	void faultOccured(QString faultMessage);
	void inputDevicesChanged();
	void videoFilterChanged();

private slots:
	void pause();
    //void pauseStage2();
	void resume();
	void onFrameGenerated(bool videoOn);
	void onSlFailed();
	void onSafetyEvent();
	void onStateLoaded();
	void hostVideoShaderChanged();
    void update();
private:
	bool loadConfiguration();
	void finishSetupConfiguration();
	QVariant loadOptionFromSettings(QSettings &s, const QString &name) const;
	QString extractArg(const QStringList &args, const QString &argName);
	void parseConfArg(const QString &arg);
	void saveScreenShotIfNotExists();
	int determineLoadSlot(const QStringList &args);
	QString constructSlErrorString() const;
	void fatalError(const QString &faultStr);

	Emu *m_emu;
	QString m_diskFileName;

	HostInput *m_hostInput;
	HostAudio *m_hostAudio;
	HostVideo *m_hostVideo;
	StateListModel *m_stateListModel;

	bool m_running;
	int m_backgroundCounter;
	bool m_quit;
	bool m_pauseRequested;
	int m_closeTries;
	int m_slotToBeLoadedOnStart;

	bool m_audioEnable;
	bool m_autoSaveLoadEnable;
	QString m_error;

	bool m_safetyCheck;
	QTimer *m_safetyTimer;
    QTimer m_updateTimer;
	bool m_safetyTimerDisabled;
	bool m_runInBackground;
	bool m_lrButtonsVisible;
    int m_frameSkip;
    QTime m_frameTime;
};

inline QString EmuView::error() const
{ return m_error; }

#endif // EMUVIEW_H
