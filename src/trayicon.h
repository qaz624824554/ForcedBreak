#pragma once

#include <QObject>
#include <QString>
#include <QSystemTrayIcon>

class AppSettings;
class BreakScheduler;
class QAction;
class QMenu;
class QSpinBox;
class QWidgetAction;

/*!
 * 托盘图标与右键菜单。应用没有主窗口，全部交互都从这里发起。
 * 菜单首项是不可点击的状态行、次项是总开关，tooltip 同步显示相同状态。
 *
 * 开关类菜单项与工作时长调节行点击后**不关闭菜单**（见 trayicon.cpp 的 StayOpenMenu），
 * 方便连续调整；跳转类菜单项（立即休息 / 设置… / 退出）仍照常关闭。
 */
class TrayIcon : public QObject
{
    Q_OBJECT

public:
    //! 设置窗口的 Tab 序号，与 SettingsWindow.qml 中的 TabBar 顺序一致。
    enum SettingsTab { TimeTab = 0, MessageTab = 1, PasswordTab = 2 };
    Q_ENUM(SettingsTab)

    TrayIcon(AppSettings *settings, BreakScheduler *scheduler, QObject *parent = nullptr);
    ~TrayIcon() override;

    void show();
    void showMessage(const QString &title, const QString &text);

    //! 运行时绘制的应用图标（托盘与气泡提示共用）。
    static QIcon appIcon();

signals:
    void breakNowRequested();
    //! 用户点了「重置计时」，或从托盘快捷改了工作时长（改完重新开始计时）。
    void resetRequested();
    void enabledToggled(bool enabled);
    void settingsRequested(int tab);
    void quitRequested();

private:
    //! 按调度器当前状态刷新状态行、开关项与 tooltip。
    void updateStatus();
    //! 状态描述文案，状态行与 tooltip 共用。
    QString statusText() const;
    //! 构造菜单里内嵌的「工作时长 [−][ 45 分钟 ][+]」一行控件。
    QWidgetAction *createWorkMinutesAction();
    //! 落盘新的工作时长并重开工作周期；值未变时不做任何事。
    void applyWorkMinutes(int minutes);

    AppSettings *m_settings = nullptr;
    BreakScheduler *m_scheduler = nullptr;
    QSystemTrayIcon m_tray;
    QMenu *m_menu = nullptr;
    QAction *m_statusAction = nullptr;
    QAction *m_enabledAction = nullptr;
    QAction *m_breakNowAction = nullptr;
    QAction *m_resetAction = nullptr;
    QAction *m_autoStartAction = nullptr;
    QWidgetAction *m_workMinutesAction = nullptr;
    QSpinBox *m_workMinutesSpin = nullptr;
};
