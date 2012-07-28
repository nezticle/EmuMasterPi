#include "hostvideoitem.h"
#include "emu.h"
#include "emuthread.h"
#include "pathmanager.h"
#include "hostinput.h"
#include "configuration.h"
#include <QtGui/QKeyEvent>
#include <QtCore/QDir>
#include <QtGui/QGuiApplication>

class HostVideoTextureProvider : public QSGTextureProvider
{
    Q_OBJECT

public:
    HostVideoTextureProvider() : tex(0) { }
    ~HostVideoTextureProvider() { delete tex; }

    QSGTexture *texture() const {
        if (tex)
            tex->setFiltering(smooth ? QSGTexture::Linear : QSGTexture::Nearest);
        return tex;
    }

    bool smooth;
    QSGTexture *tex;

public slots:
    void invalidate()
    {
        delete tex;
        tex = 0;
    }
};

HostVideoItem::HostVideoItem(QQuickItem *parent)
    : QQuickItem(parent)
    , m_emu(0)
    , m_thread(0)
    , m_videoTextureNode(0)
    , m_provider(0)
    , m_keepAspectRatio(true)
    , m_paintEnabled(false)
{
}

QSGNode *HostVideoItem::updatePaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *)
{
}

void HostVideoItem::keyPressEvent(QKeyEvent *event)
{
}

void HostVideoItem::keyReleaseEvent(QKeyEvent *event)
{
}

void HostVideoItem::takeFocus()
{
}

void HostVideoItem::setPaintEnabled(bool paintEnabled)
{
}

void HostVideoItem::updateSize()
{
}

void HostVideoItem::updatePosition()
{
}

void HostVideoItem::setKeepAspectRatio(bool on)
{
}

void HostVideoItem::setEmu(Emu *emu)
{
}

QSGTextureProvider *HostVideoItem::textureProvider() const
{
}

bool HostVideoItem::paintEnabled() const
{
}

void HostVideoItem::updateTexture()
{
}

void HostVideoItem::ensureProvider()
{
}
