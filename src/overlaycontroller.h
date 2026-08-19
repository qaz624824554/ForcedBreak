#pragma once

#include <QHash>
#include <QObject>
#include <QPointer>

class QQmlEngine;
class QQmlComponent;
class QQuickWindow;
class QScreen;

/*!
 * 按屏幕数量创建/销毁遮罩窗口，并处理显示器热插拔。
 *
 * 休息期间接入的新显示器会被立即补上遮罩——否则它就是一个免密码的逃逸出口。
 * 某块屏幕创建失败时只跳过该屏，其余屏幕照常显示。
 */
class OverlayController : public QObject
{
    Q_OBJECT

public:
    explicit OverlayController(QQmlEngine *engine, QObject *parent = nullptr);
    ~OverlayController() override;

public slots:
    void showOverlays();
    void hideOverlays();
    //! 把所有遮罩窗口重新提到前台（响应 InputBlocker 的周期性抢焦点请求）。
    void raiseOverlays();

private:
    void createOverlayFor(QScreen *screen);
    void destroyOverlayFor(QScreen *screen);

    QQmlEngine *m_engine = nullptr;
    QQmlComponent *m_component = nullptr;
    QHash<QScreen *, QPointer<QQuickWindow>> m_windows;
    bool m_visible = false;
};
