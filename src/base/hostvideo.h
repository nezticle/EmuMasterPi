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

#ifndef HOSTVIDEO_H
#define HOSTVIDEO_H

class Emu;
class EmuThread;
class HostInput;
#include "base_global.h"
#include <QtGui/QWindow>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLShaderProgram>
#include <QtCore/QTime>

#include <GLES2/gl2.h>

class HostVideo : public QWindow
{
    Q_OBJECT

public:
    static const int Width;
    static const int Height;

    explicit HostVideo(HostInput *hostInput,
                       Emu *emu,
                       EmuThread *thread);
	~HostVideo();

	bool isFpsVisible() const;
	void setFpsVisible(bool on);

	bool keepApsectRatio() const;
	void setKeepAspectRatio(bool on);

	QRectF dstRect() const;

	QPoint convertCoordHostToEmu(const QPoint &hostPos);

	void setShader(const QString &shaderName);
	QString shader() const;
	QStringList shaderList() const;
public slots:
    void repaint(); //called when new frame is ready

signals:
	void shaderChanged();
private slots:
	void updateRects();
private:
	void paintEmuFrame();
    void paintFps();
	QString shaderDir() const;
    void setupOpenGLWindow();
	void setupProgramList();
	bool loadShaderProgram();
	bool configureShaderProgram(const char *vsh, const char *fsh);

    QImage *byteSwapImage(const QImage &img);

	HostInput *m_hostInput;
	Emu *m_emu;
	EmuThread *m_thread;

	QRectF m_srcRect;
	QRectF m_dstRect;

	bool m_fpsVisible;
	int m_fpsCount;
	int m_fpsCounter;
	QTime m_fpsCounterTime;

	bool m_keepAspectRatio;

    QOpenGLContext *m_openglContext;
    QOpenGLShaderProgram *m_program;
	int m_u_pvmMatrixLocation;
	int m_u_displaySizeLocation;
	int m_a_vertexLocation;
	int m_a_texCoordLocation;
	int m_s_textureLocation;
	GLfloat m_vertexArray[8];
	GLfloat m_texCoordArray[8];

	int m_programIndex;
	bool m_programDirty;
	QStringList m_programList;
};

inline QRectF HostVideo::dstRect() const
{ return m_dstRect; }
inline bool HostVideo::isFpsVisible() const
{ return m_fpsVisible; }
inline bool HostVideo::keepApsectRatio() const
{ return m_keepAspectRatio; }

#endif // HOSTVIDEO_H
