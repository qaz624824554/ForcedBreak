#include "trayicon.h"

#include "appsettings.h"
#include "breakscheduler.h"

#include <QAction>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QSignalBlocker>

namespace {
//! 把秒数格式化为 mm:ss。
QString formatDuration(int seconds)
{
    return QStringLiteral("%1:%2")
        .arg(seconds / 60, 2, 10, QLatin1Char('0'))
        .arg(seconds % 60, 2, 10, QLatin1Char('0'));
}
}  // namespace

TrayIcon::TrayIcon(AppSettings *settings, BreakScheduler *scheduler, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_scheduler(scheduler)
{
    m_menu = new QMenu();

    // 只读状态行：显示剩余时间，不响应点击
    m_statusAction = m_menu->addAction(QString());
    m_statusAction->setEnabled(false);

    m_enabledAction = m_menu->addAction(tr("启用强制休息"));
    m_enabledAction->setCheckable(true);
    connect(m_enabledAction, &QAction::toggled, this, &TrayIcon::enabledToggled);

    m_menu->addSeparator();

    m_breakNowAction = m_menu->addAction(tr("立即休息"));
    connect(m_breakNowAction, &QAction::triggered, this, &TrayIcon::breakNowRequested);

    m_resetAction = m_menu->addAction(tr("重置计时"));
    connect(m_resetAction, &QAction::triggered, this, &TrayIcon::resetRequested);

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

    connect(m_scheduler, &BreakScheduler::enabledChanged, this, &TrayIcon::updateStatus);
    connect(m_scheduler, &BreakScheduler::breakingChanged, this, &TrayIcon::updateStatus);
    connect(m_scheduler, &BreakScheduler::workRemainingSecondsChanged, this, &TrayIcon::updateStatus);
    connect(m_scheduler, &BreakScheduler::breakRemainingSecondsChanged, this, &TrayIcon::updateStatus);
    updateStatus();
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

QString TrayIcon::statusText() const
{
    if (!m_scheduler->isEnabled())
        return tr("未启用");
    if (m_scheduler->isBreaking())
        return tr("休息剩余 %1").arg(formatDuration(m_scheduler->breakRemainingSeconds()));
    return tr("距下次休息 %1").arg(formatDuration(m_scheduler->workRemainingSeconds()));
}

void TrayIcon::updateStatus()
{
    const bool enabled = m_scheduler->isEnabled();
    const bool breaking = m_scheduler->isBreaking();

    m_statusAction->setText(statusText());
    // setChecked 会再次发出 toggled，这里只做状态回显，需屏蔽信号
    {
        const QSignalBlocker blocker(m_enabledAction);
        m_enabledAction->setChecked(enabled);
    }
    // 休息中禁止关总开关，否则它就成了绕过密码的逃逸出口
    m_enabledAction->setEnabled(!breaking);
    m_breakNowAction->setEnabled(enabled && !breaking);
    m_resetAction->setEnabled(enabled && !breaking);

    m_tray.setToolTip(tr("ForcedBreak — %1").arg(statusText()));
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
