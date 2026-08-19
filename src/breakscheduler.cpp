#include "breakscheduler.h"

#include "appsettings.h"

#include <QQmlEngine>

BreakScheduler::BreakScheduler(AppSettings *settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
{
    m_timer.setInterval(1000);
    m_timer.setTimerType(Qt::CoarseTimer);
    connect(&m_timer, &QTimer::timeout, this, &BreakScheduler::onTimeout);

    // 工作时长在计时途中被修改时，立刻按新时长重新计算剩余时间
    connect(m_settings, &AppSettings::workMinutesChanged, this, [this] {
        if (m_enabled && !m_breaking)
            onTimeout();
    });
}

BreakScheduler *BreakScheduler::create(QQmlEngine *engine, QJSEngine *)
{
    auto *settings = engine->singletonInstance<AppSettings *>("ForcedBreak", "AppSettings");
    return new BreakScheduler(settings);
}

void BreakScheduler::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;
    // 休息进行中不允许关闭：那等同于免密码解锁
    if (!enabled && m_breaking)
        return;

    m_enabled = enabled;
    if (m_enabled) {
        startWorkCycle();
    } else {
        m_timer.stop();
        setWorkRemaining(0);
        setBreakRemaining(0);
    }
    emit enabledChanged();
}

void BreakScheduler::startWorkCycle()
{
    m_breaking = false;
    m_phaseStart = QDateTime::currentDateTimeUtc();
    setWorkRemaining(m_settings->workMinutes() * 60);
    setBreakRemaining(0);
    m_timer.start();
    emit breakingChanged();
}

void BreakScheduler::triggerBreakNow()
{
    if (m_enabled && !m_breaking)
        beginBreak();
}

void BreakScheduler::resetTimer()
{
    // 休息中重置等同于免密码解锁，必须拒绝
    if (m_enabled && !m_breaking)
        startWorkCycle();
}

void BreakScheduler::unlock()
{
    if (m_breaking)
        endBreak();
}

void BreakScheduler::onTimeout()
{
    const qint64 elapsed = m_phaseStart.secsTo(QDateTime::currentDateTimeUtc());

    if (m_breaking) {
        const int remain = static_cast<int>(m_breakTotal - elapsed);
        setBreakRemaining(qMax(0, remain));
        emit tick(m_breakRemaining);
        if (remain <= 0)
            endBreak();
    } else {
        const int remain = static_cast<int>(m_settings->workMinutes() * 60 - elapsed);
        setWorkRemaining(qMax(0, remain));
        // 休眠唤醒后 elapsed 可能已远超工作时长，这里同样能立即触发休息
        if (remain <= 0)
            beginBreak();
    }
}

void BreakScheduler::beginBreak()
{
    m_breaking = true;
    m_phaseStart = QDateTime::currentDateTimeUtc();
    m_breakTotal = m_settings->breakSeconds();
    setWorkRemaining(0);
    setBreakRemaining(m_breakTotal);
    m_timer.start();
    emit breakingChanged();
    emit breakStarted(m_breakTotal);
    emit tick(m_breakRemaining);
}

void BreakScheduler::endBreak()
{
    m_breaking = false;
    emit breakEnded();
    // 休息结束（无论自然结束还是密码解锁）都重新开始完整的工作周期
    startWorkCycle();
}

void BreakScheduler::setBreakRemaining(int seconds)
{
    if (m_breakRemaining == seconds)
        return;
    m_breakRemaining = seconds;
    emit breakRemainingSecondsChanged();
}

void BreakScheduler::setWorkRemaining(int seconds)
{
    if (m_workRemaining == seconds)
        return;
    m_workRemaining = seconds;
    emit workRemainingSecondsChanged();
}
