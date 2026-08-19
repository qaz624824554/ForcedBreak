#pragma once

#include <QObject>
#include <QTimer>

/*!
 * Windows 低级键盘钩子 + 周期性抢夺前台焦点。
 *
 * 拦截 Alt+Tab / Win / Alt+F4 / Alt+Esc / Ctrl+Esc 等切换类快捷键。
 * Ctrl+Alt+Del 属于系统安全桌面，任何用户态程序都无法拦截。
 * 非 Windows 平台上 engage()/disengage() 为空操作（仅焦点定时器仍工作）。
 *
 * 钩子安装失败（例如被安全软件拦截）时只记录日志并继续，属于降级而非中止。
 */
class InputBlocker : public QObject
{
    Q_OBJECT

public:
    explicit InputBlocker(QObject *parent = nullptr);
    ~InputBlocker() override;

    bool isEngaged() const { return m_engaged; }

public slots:
    void engage();
    void disengage();

signals:
    //! 每 500ms 发出一次，由 OverlayController 把遮罩窗口重新提到前台。
    void focusStealRequested();

private:
    bool installHook();
    void removeHook();

    QTimer m_focusTimer;
    bool m_engaged = false;
};
