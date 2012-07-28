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

#include "hostaudio.h"
#include "emu.h"

#include <QtMultimedia/QAudioDeviceInfo>
#include <QtMultimedia/QAudioFormat>
#include <QtCore/QBuffer>
#include <QtCore/QDebug>

/*!
	\class HostAudio
	HostAudio class controls audio streaming to the host device.
 */

/*! Creates a HostAudio object. */
HostAudio::HostAudio(Emu *emu) :
	m_emu(emu)
{
    m_audioBuffer.resize(8192);
    m_audioBuffer.fill('\0');
}

/*! Destroys HostAudio object.*/
HostAudio::~HostAudio()
{
	close();
}

/*! Starts up audio streaming to the host. */
void HostAudio::open()
{
    QAudioFormat format;
    format.setSampleRate(44100);
    format.setChannelCount(2);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(format)) {
        qWarning() << "audio format not supported by backend, cannot play audio.";
    }

    m_audioOutput = new QAudioOutput(format, this);
    connect(m_audioOutput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(stateChanged(QAudio::State)));

    m_audioStream = m_audioOutput->start();
}

/*! Stops audio streaming. */
void HostAudio::close()
{
    m_audioOutput->stop();
}

/*! Streams a frame of audio from emulated system to the host. */
void HostAudio::sendFrame()
{
    if ((m_audioOutput->state() == QAudio::ActiveState) ||
            (m_audioOutput->state() == QAudio::IdleState)) {
        int size = m_audioOutput->periodSize() * 2;
        size = m_emu->fillAudioBuffer(m_audioBuffer.data(), size);
        //qDebug() << "writing " << size << "bytes to audio buffer";
        m_audioStream->write(m_audioBuffer, size);
    }
}

void HostAudio::stateChanged(QAudio::State state)
{
    switch (state) {
    case QAudio::ActiveState:
        //Audio data is being processed, this state is set after start() is called and while audio data is available to be processed.
        //qWarning() << "QAudio::ActiveState";
        break;
    case QAudio::SuspendedState:
        //The audio device is in a suspended state, this state will only be entered after suspend() is called.
        //qWarning() << "QAudio::SuspendedState";
        break;
    case QAudio::StoppedState:
        //The audio device is closed, and is not processing any audio data
        if (m_audioOutput->error() != QAudio::NoError) {
            // Perform error handling
            switch (m_audioOutput->error()) {
            case QAudio::OpenError:
                qWarning() << "QAudio::OpenError";
                break;
            case QAudio::IOError:
                qWarning() << "QAudio::IOError";
                break;
            case QAudio::UnderrunError:
                qWarning() << "QAudio::UnderrunError";
                break;
            case QAudio::FatalError:
                qWarning() << "QAudio::FatalError";
                m_audioStream = m_audioOutput->start();
                break;
            default:
                qWarning() << "QAudio::UnknownError";
                break;
            }
        } else {
            // Normal stop
            //qWarning() << "QAudio::StoppedState";
        }
        break;
    case QAudio::IdleState:
        //The QIODevice passed in has no data and audio system's buffer is empty, this state is set after start() is called and while no audio data is available to be processed.
        //qWarning() << "QAudio::IdleState";
        break;
    default:
        break;

    }
}
