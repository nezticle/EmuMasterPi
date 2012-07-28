#ifndef SNESEMU_H
#define SNESEMU_H

#include <base/emu.h>
#include <QtGui/QImage>

typedef u8 bool8;
typedef u32 bool8_32;

class SnesEmu : public Emu {
    Q_OBJECT
public:
	explicit SnesEmu(QObject *parent = 0);
	bool init(const QString &diskPath, QString *error);
	void shutdown();

	void reset();

	void emulateFrame(bool drawEnabled);
	const QImage &frame() const;
	int fillAudioBuffer(char *stream, int streamSize);
	int gamePad(int i) const;

	void sync(int width, int height);

	QImage m_frame;
protected:
	void sl();
	QString setDisk(const QString &path);

	static const int m_keyMapping[];
};

extern SnesEmu snesEmu;

#endif // SNESEMU_H
