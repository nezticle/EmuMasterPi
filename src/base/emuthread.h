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

#ifndef EMUTHREAD_H
#define EMUTHREAD_H

class Emu;
#include <QtCore/QThread>

class EmuThread : public QThread
{
	Q_OBJECT
public:
	explicit EmuThread(Emu *emu);
	~EmuThread();

	void setFrameSkip(int n);
	int frameSkip() const;
public slots:
	void resume();
	void pause();
signals:
	void frameGenerated(bool videoOn);
protected:
	void run();
private:
	Emu *m_emu;

	volatile bool m_running;
	volatile bool m_inFrameGenerated;

	int m_frameSkip;
	friend class HostVideo;
};

inline int EmuThread::frameSkip() const
{ return m_frameSkip; }

#endif // EMUTHREAD_H
