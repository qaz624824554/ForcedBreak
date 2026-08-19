#pragma once

#include <QObject>
#include <QSystemTrayIcon>

class AppSettings;
class BreakScheduler;
class QAction;
class QMenu;

/*!
 * 托盘图标与右键菜单。应用没有主窗口，全部交互都从这里发起。
 * tooltip 持续显示距下次休息的剩余时间。
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
    void settingsRequested(int tab);
    void quitRequested();

private:
    void updateToolTip();

    AppSettings *m_settings = nullptr;
    BreakScheduler *m_scheduler = nullptr;
    QSystemTrayIcon m_tray;
    QMenu *m_menu = nullptr;
    QAction *m_autoStartAction = nullptr;
};
