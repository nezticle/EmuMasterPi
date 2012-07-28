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

#include "emu.h"
#include "configuration.h"
#include <QtCore/QSettings>
#include <QtCore/QFile>
#include <QtGui/QImage>
#include <QtCore/QCoreApplication>

EMSL emsl;

Emu::Emu(const QString &name, QObject *parent) :
	QObject(parent),
	m_name(name),
	m_frameRate(1),
    m_audioEnabled(false),
	m_running(true)
{
}

Emu::~Emu()
{
}

void Emu::reset()
{
}

void Emu::setFrameRate(qreal rate)
{
	m_frameRate = rate;
}

void Emu::setVideoSrcRect(const QRectF &rect)
{
	if (m_videoSrcRect != rect) {
		m_videoSrcRect = rect;
		emit videoSrcRectChanged();
	}
}

void EMSL::varNotExist(const QString &name)
{
	error = QObject::tr("\"%1->%2\" not exists").arg(currGroup).arg(name);
}

void EMSL::ioError()
{
	error = QObject::tr("IO error");
}

bool Emu::saveInternal(QDataStream *stream)
{
	QByteArray ba;
	ba.reserve(1024 * 1024 * 2);

	QDataStream baStream(&ba, QIODevice::WriteOnly);
	baStream.setByteOrder(QDataStream::LittleEndian);
	baStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

	emsl.stream = &baStream;
	emsl.currAddr.clear();
	emsl.currGroup.clear();
	emsl.allAddr.clear();
	emsl.error.clear();

	emConf.sl();
	sl();

	bool succeded = emsl.error.isEmpty();
	if (succeded) {
		*stream << emsl.allAddr;
		*stream << ba;
	}
	return succeded;
}

bool Emu::loadInternal(QDataStream *stream)
{
	QByteArray ba;
	*stream >> emsl.allAddr;
	*stream >> ba;

	QDataStream baStream(&ba, QIODevice::ReadOnly);
	baStream.setByteOrder(QDataStream::LittleEndian);
	baStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

	emsl.save = false;
	emsl.stream = &baStream;
	emsl.currGroup.clear();
	emsl.currAddr.clear();
	emsl.error.clear();

	emConf.sl();
	if (!emsl.loadConfOnly && emsl.error.isEmpty()) {
		emsl.abortIfLoadFails = true;
		sl();
	} else {
		emsl.abortIfLoadFails = false;
	}

	// "version" in conf could be replaced with old one, when loading old state
	// restore it to be current one
	emConf.setValue("version", QCoreApplication::applicationVersion());

	return emsl.error.isEmpty();
}

void EMSL::push()
{
	end();
	groupStack.append(currGroup);
}

void EMSL::pop()
{
	begin(groupStack.takeLast());
}

bool Emu::saveState(const QString &statePath)
{
	emsl.save = true;

	QByteArray data;
	data.reserve(10*1024*1024);

	QDataStream s(&data, QIODevice::WriteOnly);
	s.setByteOrder(QDataStream::LittleEndian);
	s.setFloatingPointPrecision(QDataStream::SinglePrecision);
	if (!saveInternal(&s))
		return false;

	QFile file(statePath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		emsl.error = tr("Could not open file for writing.");
		return false;
	}

	s.setDevice(&file);
	s << frame().copy(videoSrcRect().toRect());
	QByteArray compressed = qCompress(data);
	bool ok = (file.write(compressed) == compressed.size());
	file.close();
	if (!ok)
		file.remove();
	return ok;
}

bool Emu::loadState(const QString &statePath)
{
	emsl.save = false;
	emsl.abortIfLoadFails = false;

	QFile file(statePath);
	if (!file.open(QIODevice::ReadOnly)) {
		emsl.error = tr("Could not open file.");
		return false;
	}

	QDataStream sOmit(&file);
	sOmit.setByteOrder(QDataStream::LittleEndian);
	sOmit.setFloatingPointPrecision(QDataStream::SinglePrecision);
	QImage omitFrame;
	sOmit >> omitFrame;

	QByteArray compressed = file.read(file.size() - file.pos());
	QByteArray data = qUncompress(compressed);
	file.close();
	compressed.clear();

	QDataStream s(&data, QIODevice::ReadOnly);
	s.setByteOrder(QDataStream::LittleEndian);
	s.setFloatingPointPrecision(QDataStream::SinglePrecision);

	return loadInternal(&s);
}

void Emu::setRunning(bool running)
{
	if (running != m_running) {
		m_running = running;
		if (m_running)
			resume();
		else
			pause();
	}
}

void EMSL::begin(const QString &groupName)
{
	currGroup = groupName;
	currAddr = allAddr.value(groupName);
}

void EMSL::end()
{
	if (save)
		allAddr[currGroup] = currAddr;
}

void EMSL::array(const QString &name, void *data, int size)
{
	if (save) {
		currAddr.insert(name, stream->device()->pos());
		if (stream->writeRawData((const char *)data, size) != size) {
			ioError();
			return;
		}
	} else {
		int addr = currAddr.value(name, -1);
		if (addr < 0) {
			varNotExist(name);
			return;
		}
		stream->device()->seek(addr);
		if (stream->readRawData((char *)data, size) != size) {
			ioError();
			return;
		}
	}
}
