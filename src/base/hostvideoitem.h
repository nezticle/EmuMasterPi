#ifndef HOSTVIDEOITEM_H
#define HOSTVIDEOITEM_H

class Emu;
class EmuThread;
class HostInput;
class HostVideoTextureProvider;

#include "base_global.h"

#include <QtQuick/QQuickItem>
#include <QtQuick/QSGSimpleTextureNode>

class BASE_EXPORT HostVideoItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(bool keepAspectRatio READ keepAspectRatio WRITE setKeepAspectRatio)

public:
    HostVideoItem(QQuickItem *parent = 0);

    //expose fps as a qreal property instead
    qreal fps() const;

    bool keepAspectRatio() const { return m_keepAspectRatio; }
    void setKeepAspectRatio(bool on);

    void setEmu(Emu *emu);

    bool isTextureProvider() const { return true; }
    QSGTextureProvider *textureProvider() const;

    bool paintEnabled() const;

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *);

    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);

public slots:
    void takeFocus();
    void setPaintEnabled(bool paintEnabled);

private slots:
    void updateSize();
    void updatePosition();

signals:
    void textureChanged();
    void emulatorChanged();

private:

    void updateTexture();
    void ensureProvider();

    Emu *m_emu;
    EmuThread *m_thread;

    HostVideoTextureProvider *m_provider;
    QSGSimpleTextureNode *m_videoTextureNode;

    bool m_keepAspectRatio;
    bool m_paintEnabled;
};

#endif // HOSTVIDEOITEM_H
