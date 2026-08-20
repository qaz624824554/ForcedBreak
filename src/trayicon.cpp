#include "trayicon.h"

#include "appsettings.h"
#include "breakscheduler.h"

#include <QAction>
#include <QApplication>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QToolButton>
#include <QWidget>
#include <QWidgetAction>

namespace {
//! 工作时长快捷调节的跳步（分钟）。
constexpr int kWorkMinutesStep = 5;

//! 动态属性名：带此标记的 QAction 点击后菜单保持打开。
constexpr char kStayOpenProperty[] = "forcedBreakStayOpen";

//! 把秒数格式化为 mm:ss。
QString formatDuration(int seconds)
{
    return QStringLiteral("%1:%2")
        .arg(seconds / 60, 2, 10, QLatin1Char('0'))
        .arg(seconds % 60, 2, 10, QLatin1Char('0'));
}

/*!
 * 托盘右键菜单：标了 kStayOpenProperty 的菜单项点击后不关闭菜单，
 * 这样连续切开关、连续加减工作时长都不必反复重开菜单。
 *
 * 另外把键盘事件转发给内嵌控件（工作时长输入框），
 * 否则 QMenu 会把数字键当成助记符吞掉，导致输入不进去。
 */
class StayOpenMenu : public QMenu
{
public:
    using QMenu::QMenu;

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        // 内嵌控件自己会消费落在它上面的鼠标事件，菜单能收到就说明点在控件之外。
        // 菜单项不是 widget、抢不走焦点，这里必须主动收回，
        // 否则输入框永远不失焦，输入的时长也就一直不提交
        clearEmbeddedFocus();
        QMenu::mousePressEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        QAction *action = activeAction();
        if (action && action->isEnabled() && action->property(kStayOpenProperty).toBool()) {
            // 手动触发并吃掉事件：不调基类实现，菜单便不会收起
            action->trigger();
            update();
            return;
        }
        QMenu::mouseReleaseEvent(event);
    }

    void keyPressEvent(QKeyEvent *event) override
    {
        // 控件没处理的按键（如回车）会 ignore 并冒泡回菜单，若再转发就是无限递归；
        // 这里直接丢弃冒泡回来的事件，顺带避免回车被菜单当成"触发当前项"
        if (m_forwardingKey) {
            event->accept();
            return;
        }

        QWidget *focused = QApplication::focusWidget();
        // Esc 留给菜单自己处理，保证任何时候都能关掉
        if (focused && focused != this && isAncestorOf(focused)
            && event->key() != Qt::Key_Escape) {
            m_forwardingKey = true;
            QApplication::sendEvent(focused, event);
            m_forwardingKey = false;
            return;
        }
        QMenu::keyPressEvent(event);
    }

    void hideEvent(QHideEvent *event) override
    {
        // 菜单收起时同样要让输入框提交内容
        clearEmbeddedFocus();
        QMenu::hideEvent(event);
    }

private:
    //! 若焦点停在菜单内嵌控件上则收回，触发其 focusOut（输入框借此提交）。
    void clearEmbeddedFocus()
    {
        QWidget *focused = QApplication::focusWidget();
        if (focused && focused != this && isAncestorOf(focused))
            focused->clearFocus();
    }

    bool m_forwardingKey = false;
};
}  // namespace

TrayIcon::TrayIcon(AppSettings *settings, BreakScheduler *scheduler, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_scheduler(scheduler)
{
    m_menu = new StayOpenMenu();

    // 只读状态行：显示剩余时间，不响应点击
    m_statusAction = m_menu->addAction(QString());
    m_statusAction->setEnabled(false);

    m_enabledAction = m_menu->addAction(tr("启用强制休息"));
    m_enabledAction->setCheckable(true);
    m_enabledAction->setProperty(kStayOpenProperty, true);
    connect(m_enabledAction, &QAction::toggled, this, &TrayIcon::enabledToggled);

    m_menu->addSeparator();

    m_workMinutesAction = createWorkMinutesAction();
    m_menu->addAction(m_workMinutesAction);

    m_menu->addSeparator();

    m_breakNowAction = m_menu->addAction(tr("立即休息"));
    connect(m_breakNowAction, &QAction::triggered, this, &TrayIcon::breakNowRequested);

    m_resetAction = m_menu->addAction(tr("重置计时"));
    connect(m_resetAction, &QAction::triggered, this, &TrayIcon::resetRequested);

    m_pauseAction = m_menu->addAction(tr("暂停计时"));
    m_pauseAction->setCheckable(true);
    m_pauseAction->setProperty(kStayOpenProperty, true);
    connect(m_pauseAction, &QAction::toggled, this, &TrayIcon::pauseToggled);

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
    m_autoStartAction->setProperty(kStayOpenProperty, true);
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
    connect(m_scheduler, &BreakScheduler::pausedChanged, this, &TrayIcon::updateStatus);
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
    if (m_scheduler->isPaused())
        return tr("已暂停 · 距下次休息 %1").arg(formatDuration(m_scheduler->workRemainingSeconds()));
    return tr("距下次休息 %1").arg(formatDuration(m_scheduler->workRemainingSeconds()));
}

QWidgetAction *TrayIcon::createWorkMinutesAction()
{
    auto *row = new QWidget(m_menu);
    auto *layout = new QHBoxLayout(row);
    // 左边距对齐菜单项文字，右边距与菜单留一点空隙
    layout->setContentsMargins(24, 2, 8, 2);
    layout->setSpacing(4);

    layout->addWidget(new QLabel(tr("工作时长"), row));
    layout->addStretch();

    auto *decrease = new QToolButton(row);
    decrease->setText(QStringLiteral("−"));
    decrease->setAutoRepeat(true);  // 长按连减
    decrease->setToolTip(tr("减少 %1 分钟").arg(kWorkMinutesStep));
    layout->addWidget(decrease);

    m_workMinutesSpin = new QSpinBox(row);
    m_workMinutesSpin->setRange(AppSettings::kWorkMinutesMin, AppSettings::kWorkMinutesMax);
    m_workMinutesSpin->setSingleStep(kWorkMinutesStep);
    m_workMinutesSpin->setAlignment(Qt::AlignRight);
    // 单位放在框外的 QLabel 上：若用 setSuffix，后缀与数字同属一个编辑框，
    // 光标能移到"分钟"上，看起来像是可以在那里打字
    m_workMinutesSpin->setFixedWidth(56);
    // 加减由两侧按钮承担，去掉自带的小箭头
    m_workMinutesSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    // 手动输入时逐字符提交会中途落盘（如输入 120 时先落 1），改为按回车或失焦才提交
    m_workMinutesSpin->setKeyboardTracking(false);
    m_workMinutesSpin->setValue(m_settings->workMinutes());
    layout->addWidget(m_workMinutesSpin);
    layout->addWidget(new QLabel(tr("分钟"), row));

    auto *increase = new QToolButton(row);
    increase->setText(QStringLiteral("+"));
    increase->setAutoRepeat(true);
    increase->setToolTip(tr("增加 %1 分钟").arg(kWorkMinutesStep));
    layout->addWidget(increase);

    connect(decrease, &QToolButton::clicked, m_workMinutesSpin, &QSpinBox::stepDown);
    connect(increase, &QToolButton::clicked, m_workMinutesSpin, &QSpinBox::stepUp);
    connect(m_workMinutesSpin, &QSpinBox::valueChanged, this, &TrayIcon::applyWorkMinutes);
    // 设置窗口里改了时长时回显到这里
    connect(m_settings, &AppSettings::workMinutesChanged, this, [this] {
        const QSignalBlocker blocker(m_workMinutesSpin);
        m_workMinutesSpin->setValue(m_settings->workMinutes());
    });

    auto *action = new QWidgetAction(m_menu);
    action->setDefaultWidget(row);
    return action;
}

void TrayIcon::applyWorkMinutes(int minutes)
{
    if (minutes == m_settings->workMinutes())
        return;

    // 必须先重开工作周期再落盘：setWorkMinutes 会让调度器立即按新时长重算剩余时间，
    // 若先落盘，把时长调到小于已工作时间就会当场弹出遮罩
    emit resetRequested();
    m_settings->setWorkMinutes(minutes);

    // setWorkMinutes 会 clamp，越界输入需回显真实值
    if (m_workMinutesSpin->value() != m_settings->workMinutes()) {
        const QSignalBlocker blocker(m_workMinutesSpin);
        m_workMinutesSpin->setValue(m_settings->workMinutes());
    }
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
    // 同上，setChecked 只做状态回显，不应回灌信号
    {
        const QSignalBlocker blocker(m_pauseAction);
        m_pauseAction->setChecked(m_scheduler->isPaused());
    }
    // 休息中暂停等同于免密码解锁，调度器会拒绝，这里直接禁用避免误解
    m_pauseAction->setEnabled(enabled && !breaking);
    // 休息中改工作时长既无效果又会被 resetTimer 拒绝，直接禁用避免误解
    m_workMinutesAction->setEnabled(!breaking);
    m_workMinutesAction->defaultWidget()->setEnabled(!breaking);

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
