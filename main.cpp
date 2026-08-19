#include "src/appsettings.h"
#include "src/breakscheduler.h"
#include "src/inputblocker.h"
#include "src/overlaycontroller.h"
#include "src/settingswindowmanager.h"
#include "src/trayicon.h"

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QSharedMemory>
#include <QSystemTrayIcon>
#include <QTimer>

int main(int argc, char *argv[])
{
    // 托盘依赖 QtWidgets（Qt Quick 无原生托盘支持），因此用 QApplication
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("ForcedBreak"));
    QApplication::setOrganizationName(QStringLiteral("ForcedBreak"));
    // 没有主窗口，设置窗口关闭后不应退出程序
    QApplication::setQuitOnLastWindowClosed(false);

    // 单实例保护：两份程序会产生两套互相冲突的计时器
    QSharedMemory singleInstance(QStringLiteral("ForcedBreak-SingleInstance"));
    if (!singleInstance.create(1)) {
        // 弹一条气泡提示后静默退出，不重复驻留
        QSystemTrayIcon notice(TrayIcon::appIcon());
        notice.show();
        notice.showMessage(QObject::tr("ForcedBreak"), QObject::tr("ForcedBreak 已在运行中。"));
        QTimer::singleShot(3000, &app, &QCoreApplication::quit);
        return QApplication::exec();
    }

    QQmlApplicationEngine engine;

    auto *settings = engine.singletonInstance<AppSettings *>("ForcedBreak", "AppSettings");
    auto *scheduler = engine.singletonInstance<BreakScheduler *>("ForcedBreak", "BreakScheduler");
    if (!settings || !scheduler) {
        qCritical("QML 单例注册失败，无法启动");
        return -1;
    }

    OverlayController overlays(&engine);
    InputBlocker blocker;
    SettingsWindowManager settingsWindow(&engine);
    TrayIcon tray(settings, scheduler, &app);

    // 休息开始：装遮罩 + 上钩子；结束：反向拆除
    QObject::connect(scheduler, &BreakScheduler::breakStarted, &app, [&](int) {
        overlays.showOverlays();
        blocker.engage();
    });
    QObject::connect(scheduler, &BreakScheduler::breakEnded, &app, [&] {
        blocker.disengage();
        overlays.hideOverlays();
    });
    QObject::connect(&blocker, &InputBlocker::focusStealRequested,
                     &overlays, &OverlayController::raiseOverlays);

    QObject::connect(&tray, &TrayIcon::breakNowRequested, scheduler, &BreakScheduler::triggerBreakNow);
    QObject::connect(&tray, &TrayIcon::resetRequested, scheduler, &BreakScheduler::resetTimer);
    QObject::connect(&tray, &TrayIcon::enabledToggled, scheduler, &BreakScheduler::setEnabled);
    QObject::connect(&tray, &TrayIcon::settingsRequested, &settingsWindow, &SettingsWindowManager::open);
    QObject::connect(&tray, &TrayIcon::quitRequested, &app, &QCoreApplication::quit);

    // 总开关默认关闭，不在此处启动计时——由用户从托盘菜单开启
    tray.show();

    return QApplication::exec();
}
