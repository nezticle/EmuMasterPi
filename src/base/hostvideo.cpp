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

#include "hostvideo.h"
#include "emu.h"
#include "emuthread.h"
#include "pathmanager.h"
#include "hostinput.h"
#include "configuration.h"
#include <QtGui/QPainter>
#include <QtGui/QKeyEvent>
#include <QtCore/QDir>
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlContext>
#include <QtCore/QDebug>

struct QGLRect
{
	QGLRect(const QRectF &r)
		:  left(r.left()), top(r.top()), right(r.right()), bottom(r.bottom()) {}

	QGLRect(GLfloat l, GLfloat t, GLfloat r, GLfloat b)
		: left(l), top(t), right(r), bottom(b) {}

	GLfloat left;
	GLfloat top;
	GLfloat right;
	GLfloat bottom;

	operator QRectF() const {return QRectF(left, top, right-left, bottom-top);}
};

static const char *defaultVertexShader =
		"attribute highp vec4 a_vertex;           \n"
		"attribute vec2 a_texCoord;               \n"
		"varying vec2 v_texCoord;                 \n"
		"uniform highp mat4 u_pvmMatrix;          \n"
		"void main()                              \n"
		"{                                        \n"
		"    gl_Position = u_pvmMatrix * a_vertex;\n"
		"    v_texCoord = a_texCoord;             \n"
		"}                                        \n";

static const char *defaultFragmentShader =
		"precision mediump float;                            \n"
		"varying vec2 v_texCoord;                            \n"
		"uniform sampler2D s_texture;                        \n"
		"void main()                                         \n"
		"{                                                   \n"
		"    gl_FragColor = texture2D(s_texture, v_texCoord);\n"
		"}                                                   \n";

HostVideo::HostVideo(HostInput *hostInput, Emu *emu)
    : QQuickView(),
	m_hostInput(hostInput),
	m_emu(emu),
	m_program(0),
    m_programIndex(0),
    m_programDirty(true)
{
	m_fpsVisible = emConf.defaultValue("fpsVisible").toInt();
	m_fpsCount = 0;
	m_fpsCounter = 0;
	m_fpsCounterTime.start();

	m_keepAspectRatio = emConf.defaultValue("keepAspectRatio").toBool();

	setupProgramList();

    setResizeMode(QQuickView::SizeRootObjectToView);
    setClearBeforeRendering(false);
    rootContext()->setContextProperty("hostVideo", this);
    //TODO: dont hardcode location of this file
    setSource(QUrl::fromLocalFile("/opt/apps/emumaster/data/qml/main.qml"));

	QObject::connect(m_emu, SIGNAL(videoSrcRectChanged()), SLOT(updateRects()));
    QObject::connect(this, SIGNAL(widthChanged(int)), this, SLOT(updateRects()));
    QObject::connect(this, SIGNAL(heightChanged(int)), this, SLOT(updateRects()));
    QObject::connect(this, SIGNAL(beforeRendering()), this, SLOT(repaint()), Qt::DirectConnection);
}

HostVideo::~HostVideo()
{
}

static inline void setCoords(GLfloat *coords, const QGLRect &rect)
{
	coords[0] = rect.left;
	coords[1] = rect.top;
	coords[2] = rect.right;
	coords[3] = rect.top;
	coords[4] = rect.right;
	coords[5] = rect.bottom;
	coords[6] = rect.left;
	coords[7] = rect.bottom;
}

//This is called in the render thread only
void HostVideo::paintEmuFrame()
{
    //TODO: Make thread safe
	const QImage &tex = m_emu->frame();
//    static int debugSnapshotCounter = 1;
//    if (!(debugSnapshotCounter % 120)) {
//        QString filename = "snapshot" + QString::number(debugSnapshotCounter) + ".png";
//        tex.save(filename);
//    }
//    debugSnapshotCounter++;
	QSizeF textureSize = tex.size();
	QGLRect src = m_srcRect;
	GLfloat dx = 1.0 / textureSize.width() ;
	GLfloat dy = 1.0 / textureSize.height();
	QGLRect srcTextureRect(src.left*dx, src.top*dy, src.right*dx, src.bottom*dy);
	setCoords(m_texCoordArray, srcTextureRect);

	if (m_keepAspectRatio) {
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
	}
	m_program->bind();
	if (m_u_displaySizeLocation != -1) {
		qreal w = m_dstRect.width() * textureSize.width() / m_srcRect.width();
		qreal h = m_dstRect.height() * textureSize.height() / m_srcRect.height();
        m_program->setUniformValue(m_u_displaySizeLocation, QVector2D(w, h));
	}

    //Enable Vertex Attribute Arrays:
    glEnableVertexAttribArray(m_a_vertexLocation);
    glEnableVertexAttribArray(m_a_texCoordLocation);

    //Set Locations for Attribute Arrays
    glVertexAttribPointer(m_a_vertexLocation, 2, GL_FLOAT, GL_FALSE, 0, m_vertexArray);
    glVertexAttribPointer(m_a_texCoordLocation, 2, GL_FLOAT, GL_FALSE, 0, m_texCoordArray);

    //upload texture
	glActiveTexture(GL_TEXTURE0);
    uint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    if (tex.format() == QImage::Format_RGB32) {
        //Fix byte order for texture upload
        QImage *fixedImage = byteSwapImage(tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, fixedImage->constBits());
        delete fixedImage;
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex.width(), tex.height(), 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, tex.constBits());
    }


    //Draw everything
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    //delete texture
    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteTextures(1, &textureID);

    //Disable Vertex Attribute Arrays
    glDisableVertexAttribArray(m_a_vertexLocation);
    glDisableVertexAttribArray(m_a_texCoordLocation);

    updateFps();
}

/*! \internal */
void HostVideo::updateFps()
{
	// calculate fps
	m_fpsCounter++;
	if (m_fpsCounterTime.elapsed() >= 1000) {
		m_fpsCount = m_fpsCounter;
		m_fpsCounter = 0;
		m_fpsCounterTime.restart();
        emit fpsCountChanged();
	}
}

/*! Sets if fps should be drawn (\a on=true) or not. */
void HostVideo::setFpsVisible(bool on)
{
	m_fpsVisible = on;
}

/*! \internal */
void HostVideo::updateRects()
{
	// update rects if size of source rect changes
	m_srcRect = m_emu->videoSrcRect();
	Q_ASSERT_X(m_srcRect.width() != 0.0f && m_srcRect.height() != 0.0f, "HostVideo", "Define source rect!");
    qreal ww = this->width();
    qreal wh = this->height();
	if (m_keepAspectRatio) {
		// calculate proportionally scaled rect
		qreal scale = qMin(ww/m_srcRect.width(), wh/m_srcRect.height());
		qreal w = m_srcRect.width() * scale;
		qreal h = m_srcRect.height() * scale;
		qreal x = ww/2.0f-w/2.0f;
		qreal y = wh/2.0f-h/2.0f;
		m_dstRect = QRectF(x, y, w, h);
	} else {
		// take screen size
		m_dstRect = QRectF(QPointF(), QSizeF(ww, wh));
	}
	QGLRect dst = m_dstRect;
	setCoords(m_vertexArray, dst);
}

/*!
	Sets if aspect ratio must be kept (\a on = true) or frame
	can be displayed on fullscreen.
*/
void HostVideo::setKeepAspectRatio(bool on)
{
	m_keepAspectRatio = on;
	if (m_srcRect.width() != 0.0f)
		updateRects();
}

/*!
	Converts position from host coordinates to emulation coordinates and
	returns as a point.
 */
QPoint HostVideo::convertCoordHostToEmu(const QPoint &hostPos)
{
	QPoint rel = hostPos - m_dstRect.topLeft().toPoint();
	int x = rel.x()*m_srcRect.width()/m_dstRect.width();
	int y = rel.y()*m_srcRect.height()/m_dstRect.height();
	return QPoint(x, y);
}

void HostVideo::setShader(const QString &shaderName)
{
	int index = m_programList.indexOf(shaderName);
	if (index < 0)
		return;

	if (index != m_programIndex) {
		m_programIndex = index;
		m_programDirty = true;
		emit shaderChanged();
	}
}

QString HostVideo::shaderDir() const
{
    return pathManager.installationDirPath() + "/data/shader";
}

void HostVideo::setupProgramList()
{
	m_programList.append("none");
	QStringList list = QDir(shaderDir()).entryList(QStringList() << "*.vsh");
	foreach (QString s, list)
		m_programList.append(s.left(s.size()-4));
}

QString HostVideo::shader() const
{
	return m_programList.at(m_programIndex);
}

QStringList HostVideo::shaderList() const
{
    return m_programList;
}

//This is called in the render thread only
void HostVideo::repaint()
{
    if (m_programDirty) {
        if (!loadShaderProgram())
            return;
    }

    paintEmuFrame();
}

bool HostVideo::loadShaderProgram()
{
	Q_ASSERT(m_programIndex >= 0 && m_programIndex < m_programList.size());

	if (m_programIndex > 0) {
		qDebug("Loading custom shader %s", qPrintable(shader()));
		QDir dir(shaderDir());
		QFile vshFile(dir.filePath(shader() + ".vsh"));
		QFile fshFile(dir.filePath(shader() + ".fsh"));

		if (vshFile.open(QIODevice::ReadOnly) &&
			fshFile.open(QIODevice::ReadOnly)) {
			QByteArray vshSource = vshFile.readAll();
			QByteArray fshSource = fshFile.readAll();
			if (configureShaderProgram(vshSource.constData(),
									   fshSource.constData())) {
				m_programDirty = false;
				return true;
			}
		}
		qDebug("Failed to load custom shader %s", qPrintable(shader()));
	}
	if (!configureShaderProgram(defaultVertexShader, defaultFragmentShader)) {
		qDebug("Could not compile default shader, terminating!!!");
		qApp->exit(-1);
		return false;
	}
	m_programDirty = false;
	if (m_programIndex > 0) {
		m_programIndex = 0;
		emit shaderChanged();
	}
	return true;
}

bool HostVideo::configureShaderProgram(const char *vsh, const char *fsh)
{
	if (m_program)
		delete m_program;

    m_program = new QOpenGLShaderProgram;
    if (!m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vsh)) {
    	qDebug("Vertex shader failed to compile");
        qDebug(m_program->log().toLocal8Bit());
		return false;
	}
    if (!m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fsh)) {
    	qDebug("Fragment shader failed to compile");
        qDebug(m_program->log().toLocal8Bit());
		return false;
	}
    if (!m_program->link()) {
    	qDebug("Shader program failed to link");
        qDebug(m_program->log().toLocal8Bit());
        return false;
    }

    m_program->bind();


    m_a_vertexLocation = m_program->attributeLocation("a_vertex");
    m_a_texCoordLocation = m_program->attributeLocation("a_texCoord");
    m_u_pvmMatrixLocation = m_program->uniformLocation("u_pvmMatrix");
    m_s_textureLocation = m_program->uniformLocation("s_texture");
	if (m_a_vertexLocation < 0 ||
		m_a_texCoordLocation < 0 ||
		m_u_pvmMatrixLocation < 0 ||
		m_s_textureLocation < 0) {
		qDebug("Location not found in the shader program.");
		return false;
	}
    m_u_displaySizeLocation = m_program->uniformLocation("u_displaySize");

    m_program->setUniformValue(m_s_textureLocation, 0);

    QMatrix4x4 u_pvmMatrix;
    u_pvmMatrix.ortho(QRectF(0.0f, 0.0f, (float)width(), (float)height()));

    m_program->setUniformValue(m_u_pvmMatrixLocation, u_pvmMatrix);
    return true;
}

QImage *HostVideo::byteSwapImage(const QImage &img)
{
    const int width = img.width();
    const int height = img.height();
    QImage *fixedImage = new QImage(img);

    for (int i = 0; i < height; ++i) {
        uint *p = (uint *) fixedImage->scanLine(i);
        for (int x = 0; x < width; ++x)
            p[x] = ((p[x] << 16) & 0xff0000) | ((p[x] >> 16) & 0xff) | (p[x] & 0xff00ff00);
    }
    return fixedImage;
}
