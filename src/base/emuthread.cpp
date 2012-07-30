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

#include "emuthread.h"
#include "emu.h"
#include "hostaudio.h"
#include "statelistmodel.h"
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/QDateTime>
#include <Qt/qmath.h>

/*!
	\class EmuThread
	EmuThread class manages the execution of the emulation.
 */

/*! Creates a new object with the given \a emu. */
EmuThread::EmuThread(Emu *emu) :
	m_emu(emu),
	m_running(false),
	m_inFrameGenerated(false),
	m_frameSkip(1)
{
}

/*! Destroys the thread. */
EmuThread::~EmuThread()
{
}

/*! Resumes the execution of the emulation. */
void EmuThread::resume()
{
	m_running = true;
    start(QThread::TimeCriticalPriority);
    //start();
}

/*! Pauses the execution of the emulation. */
void EmuThread::pause()
{
	m_running = false;
}

static void sleepMs(uint msecs)
{
	QMutex mutex;
	mutex.lock();
	QWaitCondition waitCondition;
	waitCondition.wait(&mutex, msecs);
	mutex.unlock();
}

/*! \internal */
void EmuThread::run()
{
    QTime emulateFrameBenchmark;
    emulateFrameBenchmark.start();

	// resume the emulation
	m_emu->setRunning(true);
	qreal frameTime = 1000.0 / m_emu->frameRate();
	QTime time;
	time.start();
	qreal currentFrameTime = 0;
	int frameCounter = 0;
	while (m_running) {
		qreal currentTime = time.elapsed();
		currentFrameTime += frameTime;
//        if (currentTime < currentFrameTime && frameCounter == 0) {
        if (frameCounter == 0) {
            //qDebug("%dms since last frame", emulateFrameBenchmark.elapsed());
            QTime tempTime;
            tempTime.start();
			m_emu->emulateFrame(true);
            qDebug("%dms to render frame", tempTime.elapsed());
            emulateFrameBenchmark.restart();
			m_inFrameGenerated = true;
			emit frameGenerated(true);
			m_inFrameGenerated = false;

//            qreal currentTime = time.addMSecs(5).elapsed();
//            if (currentTime < currentFrameTime)
//                sleepMs(qFloor(currentFrameTime - currentTime));
		} else {
			m_emu->emulateFrame(false);
			emit frameGenerated(false);

			if (frameCounter != 0) {
                qreal currentTime = time.addMSecs(5).elapsed();
                if (currentTime < currentFrameTime)
                    sleepMs(qFloor(currentFrameTime - currentTime));
			} else {
				currentFrameTime = 0;
				time.restart();
			}
		}
		if (++frameCounter > m_frameSkip)
			frameCounter = 0;
	}
	// pause the emulation
	m_emu->setRunning(false);
}

/*! Sets frameskip to \a n. */
void EmuThread::setFrameSkip(int n)
{
	m_frameSkip = n;
}

/*!
	\fn int EmuThread::frameSkip() const
	Returns current frameskip.
 */
