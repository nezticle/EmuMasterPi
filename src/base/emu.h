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

#ifndef EMU_H
#define EMU_H

#include "emuinput.h"
#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtCore/QRectF>
#include <QtCore/QDataStream>
class QImage;

typedef qint8 s8;
typedef quint8 u8;
typedef qint16 s16;
typedef quint16 u16;
typedef qint32 s32;
typedef quint32 u32;
typedef qint64 s64;
typedef quint64 u64;

#define S8_MIN SCHAR_MIN
#define S8_MAX SCHAR_MAX
#define U8_MAX UCHAR_MAX
#define S16_MIN SHRT_MIN
#define S16_MAX SHRT_MAX
#define U16_MAX USHRT_MAX
#define S32_MIN INT_MIN
#define S32_MAX INT_MAX
#define U32_MAX UINT_MAX

static const int KB = 1024;
static const int MB = KB * KB;
static const int GB = KB * KB * KB;

#define EM_MSG_DISK_LOAD_FAILED QObject::tr("Could not load the disk")
#define EM_MSG_OPEN_FILE_FAILED QObject::tr("Unable to open the file")
#define EM_MSG_FILE_CORRUPTED QObject::tr("File is corrupted")
#define EM_MSG_STATE_DIFFERS QObject::tr("Configuration of loaded state differs from the current one. Mismatch in")
#define EM_MSG_MMAP_FAILED QObject::tr("Fatal error: mmap() failed")

#define DISABLE_IMPLICIT_CONSTRUCTORS(Class) \
	Class(); \
	Q_DISABLE_COPY(Class)

#define UNREACHABLE() Q_ASSERT(false)

class BASE_EXPORT Emu : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QString name READ name CONSTANT)
public:
	explicit Emu(const QString &name, QObject *parent = 0);
	~Emu();

	QString name() const;

	qreal frameRate() const;
	QRectF videoSrcRect() const;

	virtual bool init(const QString &diskPath, QString *error) = 0;
	virtual void shutdown() = 0;
	Q_INVOKABLE virtual void reset();

	virtual void emulateFrame(bool drawEnabled) = 0;
	virtual const QImage &frame() const = 0;

	void setAudioEnabled(bool on);
	bool isAudioEnabled() const;

	virtual int fillAudioBuffer(char *stream, int streamSize) = 0;

	bool saveState(const QString &statePath);
	bool loadState(const QString &statePath);

	void setRunning(bool running);
	bool isRunning() const;

	EmuInput *input();
	const EmuInput *input() const;
signals:
	void videoSrcRectChanged();
protected:
	virtual void sl() = 0;
	void setFrameRate(qreal rate);
	void setVideoSrcRect(const QRectF &rect);

	virtual void pause() {}
	virtual void resume() {}
private:
	bool saveInternal(QDataStream *stream);
	bool loadInternal(QDataStream *stream);

	EmuInput m_input;
	QString m_name;
	qreal m_frameRate;
	QRectF m_videoSrcRect;
	bool m_audioEnabled;
	bool m_running;
};

inline QString Emu::name() const
{ return m_name; }
inline qreal Emu::frameRate() const
{ return m_frameRate; }
inline QRectF Emu::videoSrcRect() const
{ return m_videoSrcRect; }

inline void Emu::setAudioEnabled(bool on)
{ m_audioEnabled = on; }
inline bool Emu::isAudioEnabled() const
{ return m_audioEnabled; }

inline bool Emu::isRunning() const
{ return m_running; }

inline EmuInput *Emu::input()
{ return &m_input; }
inline const EmuInput *Emu::input() const
{ return &m_input; }

// emumaster save/load functionality

class BASE_EXPORT EMSL
{
public:
	void begin(const QString &groupName);
	void end();

	void push();
	void pop();

	template <typename T> void var(const QString &name, T &t);
	void array(const QString &name, void *data, int size);

	QHash<QString, QHash<QString, int> > allAddr;

	QString currGroup;
	QHash<QString, int> currAddr;
	QDataStream *stream;
	bool save;

	bool abortIfLoadFails;
	bool loadConfOnly;

	QString error;
private:
	void varNotExist(const QString &name);
	void ioError();

	QList<QString> groupStack;
};

BASE_EXPORT extern EMSL emsl;

template <typename T>
inline void EMSL::var(const QString &name, T &t)
{
	if (save) {
		currAddr.insert(name, stream->device()->pos());
		*stream << t;
	} else {
		int addr = currAddr.value(name, -1);
		if (addr < 0) {
			varNotExist(name);
			return;
		}
		if (!stream->device()->seek(addr)) {
			ioError();
			return;
		}
		*stream >> t;
	}
}

#endif // EMU_H
