#include "trayicon.h"

#include "appsettings.h"
#include "breakscheduler.h"

#include <QAction>
#include <QMenu>
#include <QPainter>
#include <QPixmap>

TrayIcon::TrayIcon(AppSettings *settings, BreakScheduler *scheduler, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_scheduler(scheduler)
{
    m_menu = new QMenu();

    QAction *breakNow = m_menu->addAction(tr("立即休息"));
    connect(breakNow, &QAction::triggered, this, &TrayIcon::breakNowRequested);

    m_menu->addSeparator();

    QAction *timeSettings = m_menu->addAction(tr("休息时间设置…"));
    connect(timeSettings, &QAction::triggered, this, [this] { emit settingsRequested(TimeTab); });
    QAction *messageSettings = m_menu->addAction(tr("显示文案设置…"));
    connect(messageSettings, &QAction::triggered, this, [this] { emit settingsRequested(MessageTab); });
    QAction *passwordSettings = m_menu->addAction(tr("解锁密码设置…"));
    connect(passwordSettings, &QAction::triggered, this, [this] { emit settingsRequested(PasswordTab); });

    m_menu->addSeparator();

    m_autoStartAction = m_menu->addAction(tr("开机自启动"));
    m_autoStartAction->setCheckable(true);
    m_autoStartAction->setChecked(m_settings->autoStart());
    connect(m_autoStartAction, &QAction::toggled, m_settings, &AppSettings::setAutoStart);
    connect(m_settings, &AppSettings::autoStartChanged, this,
            [this] { m_autoStartAction->setChecked(m_settings->autoStart()); });

    m_menu->addSeparator();

    QAction *quit = m_menu->addAction(tr("退出"));
    connect(quit, &QAction::triggered, this, &TrayIcon::quitRequested);

    m_tray.setContextMenu(m_menu);
    m_tray.setIcon(appIcon());

    connect(m_scheduler, &BreakScheduler::workRemainingSecondsChanged, this, &TrayIcon::updateToolTip);
    connect(m_scheduler, &BreakScheduler::breakingChanged, this, &TrayIcon::updateToolTip);
    updateToolTip();
}

TrayIcon::~TrayIcon()
{
    // QMenu 是 QWidget，无法挂在本对象下，需手动释放
    delete m_menu;
}

void TrayIcon::show()
{
    m_tray.show();
}

void TrayIcon::showMessage(const QString &title, const QString &text)
{
    m_tray.showMessage(title, text, appIcon());
}

void TrayIcon::updateToolTip()
{
    if (m_scheduler->isBreaking()) {
        m_tray.setToolTip(tr("ForcedBreak — 休息中"));
        return;
    }
    const int remain = m_scheduler->workRemainingSeconds();
    m_tray.setToolTip(tr("ForcedBreak — 距下次休息 %1:%2")
                          .arg(remain / 60, 2, 10, QLatin1Char('0'))
                          .arg(remain % 60, 2, 10, QLatin1Char('0')));
}

QIcon TrayIcon::appIcon()
{
    // 运行时绘制图标，省去二进制资源文件
    QPixmap pixmap(64, 64);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0x2d, 0x7d, 0xd2));
    painter.drawEllipse(2, 2, 60, 60);

    // 中间一个白色暂停符号，表意"休息"
    painter.setBrush(Qt::white);
    painter.drawRoundedRect(20, 18, 8, 28, 3, 3);
    painter.drawRoundedRect(36, 18, 8, 28, 3, 3);
    painter.end();

    return QIcon(pixmap);
}
