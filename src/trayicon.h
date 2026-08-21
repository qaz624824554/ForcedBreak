#pragma once

#include <QObject>
#include <QString>
#include <QSystemTrayIcon>

#include <optional>

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
 * 「暂停计时」是可勾选项：勾上即暂停、取消勾选即继续，与「启用强制休息」的交互一致。
 *
 * 开关类菜单项与工作时长调节行点击后**不关闭菜单**（见 trayicon.cpp 的 StayOpenMenu），
 * 方便连续调整；跳转类菜单项（立即休息 / 设置… / 退出）仍照常关闭。
 */
class TrayIcon : public QObject
{
    Q_OBJECT

public:
    //! 托盘图标的状态配色，一眼能看出计时器眼下在干什么。
    enum class TrayState {
        Disabled,  //!< 未启用
        Working,   //!< 工作计时中
        Paused,    //!< 已暂停
        Breaking,  //!< 休息中
    };

    //! 设置窗口的 Tab 序号，与 SettingsWindow.qml 中的 TabBar 顺序一致。
    enum SettingsTab { TimeTab = 0, MessageTab = 1, PasswordTab = 2 };
    Q_ENUM(SettingsTab)

    TrayIcon(AppSettings *settings, BreakScheduler *scheduler, QObject *parent = nullptr);
    ~TrayIcon() override;

    void show();
    void showMessage(const QString &title, const QString &text);

    //! 应用图标（设置窗口、气泡提示与单实例提示共用），由 :/icons/app.svg 渲染。
    static QIcon appIcon();
    //! 对应状态的托盘图标，由 :/icons/tray-cup.svg 渲染后整体着色。
    static QIcon trayIcon(TrayState state);

signals:
    void breakNowRequested();
    //! 用户点了「重置计时」，或从托盘快捷改了工作时长（改完重新开始计时）。
    void resetRequested();
    void enabledToggled(bool enabled);
    //! 用户勾选/取消勾选了「暂停计时」。
    void pauseToggled(bool paused);
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

    //! 当前托盘图标状态；每秒刷新一次状态，图标只在真正变化时重设，避免托盘闪烁。
    std::optional<TrayState> m_trayState;

    AppSettings *m_settings = nullptr;
    BreakScheduler *m_scheduler = nullptr;
    QSystemTrayIcon m_tray;
    QMenu *m_menu = nullptr;
    QAction *m_statusAction = nullptr;
    QAction *m_enabledAction = nullptr;
    QAction *m_breakNowAction = nullptr;
    QAction *m_resetAction = nullptr;
    QAction *m_pauseAction = nullptr;
    QAction *m_autoStartAction = nullptr;
    QWidgetAction *m_workMinutesAction = nullptr;
    QSpinBox *m_workMinutesSpin = nullptr;
};
