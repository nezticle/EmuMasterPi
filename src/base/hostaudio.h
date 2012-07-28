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

#ifndef HOSTAUDIO_H
#define HOSTAUDIO_H

class Emu;
#include "base_global.h"
#include <QtCore/QObject>
#include <QtCore/QBuffer>
#include <QtCore/QByteArray>
#include <QtMultimedia/QAudioOutput>
#include <QtMultimedia/qaudio.h>

class BASE_EXPORT HostAudio : public QObject
{
	Q_OBJECT
public:
	explicit HostAudio(Emu *emu);
	~HostAudio();

	void open();
	void close();

	void sendFrame();

private slots:
    void stateChanged(QAudio::State state);

private:
    QAudioOutput *m_audioOutput;
    QIODevice *m_audioStream;
    QByteArray m_audioBuffer;
	Emu *m_emu;
};

#endif // HOSTAUDIO_H
