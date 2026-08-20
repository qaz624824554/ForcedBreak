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
        // 暂停期间 m_phaseStart 尚未后移，此时重算会把暂停时长算进已工作时间
        if (m_enabled && !m_breaking && !m_paused)
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
        clearPaused();
        setWorkRemaining(0);
        setBreakRemaining(0);
    }
    emit enabledChanged();
}

void BreakScheduler::startWorkCycle()
{
    m_breaking = false;
    m_preNoticeFired = false;
    clearPaused();
    m_phaseStart = QDateTime::currentDateTimeUtc();
    setWorkRemaining(m_settings->workMinutes() * 60);
    setBreakRemaining(0);
    m_timer.start();
    emit breakingChanged();
}

void BreakScheduler::setPaused(bool paused)
{
    if (m_paused == paused)
        return;
    // 未启用时没有计时可暂停；休息中暂停等同于免密码解锁，必须拒绝
    if (!m_enabled || m_breaking)
        return;

    m_paused = paused;
    if (m_paused) {
        m_pauseStart = QDateTime::currentDateTimeUtc();
        m_timer.stop();
    } else {
        // 把阶段起点整体后移暂停时长：已工作时间保持不变，剩余时间从冻结处接着走
        m_phaseStart = m_phaseStart.addSecs(m_pauseStart.secsTo(QDateTime::currentDateTimeUtc()));
        m_timer.start();
    }
    emit pausedChanged();

    // 恢复后立刻刷新一次剩余时间，不必空等到下一秒
    if (!m_paused)
        onTimeout();
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

void BreakScheduler::resumeWork()
{
    // 只认等待态：倒计时未满时按钮不出现，从别处调用也不该放行
    if (m_awaitingResume)
        endBreak();
}

void BreakScheduler::onTimeout()
{
    const qint64 elapsed = m_phaseStart.secsTo(QDateTime::currentDateTimeUtc());

    if (m_breaking) {
        const int remain = static_cast<int>(m_breakTotal - elapsed);
        setBreakRemaining(qMax(0, remain));
        emit tick(m_breakRemaining);
        // 倒计时归零不自动结束，改为等待用户点击「开始下一轮」
        if (remain <= 0)
            beginAwaitingResume();
    } else {
        const int remain = static_cast<int>(m_settings->workMinutes() * 60 - elapsed);
        setWorkRemaining(qMax(0, remain));
        // 休眠唤醒后 elapsed 可能已远超工作时长，这里同样能立即触发休息
        if (remain <= 0) {
            beginBreak();
            return;
        }
        maybeEmitPreNotice(remain);
    }
}

void BreakScheduler::beginBreak()
{
    m_breaking = true;
    setAwaitingResume(false);
    clearPaused();
    m_phaseStart = QDateTime::currentDateTimeUtc();
    m_breakTotal = m_settings->breakSeconds();
    setWorkRemaining(0);
    setBreakRemaining(m_breakTotal);
    m_timer.start();
    emit breakingChanged();
    emit breakStarted(m_breakTotal);
    emit tick(m_breakRemaining);
}

void BreakScheduler::beginAwaitingResume()
{
    // 停表即可：遮罩与键盘钩子的拆除只由 breakEnded 触发，此处不动它们
    m_timer.stop();
    setAwaitingResume(true);
}

void BreakScheduler::endBreak()
{
    m_breaking = false;
    setAwaitingResume(false);
    emit breakEnded();
    // 休息结束（无论自然结束还是密码解锁）都重新开始完整的工作周期
    startWorkCycle();
}

void BreakScheduler::maybeEmitPreNotice(int remainSec)
{
    if (m_preNoticeFired)
        return;

    const int threshold = m_settings->preNotifySeconds();
    // 0 表示关闭；提前量不小于工作总时长时提醒会在周期一开始就弹出，没有意义
    if (threshold <= 0 || threshold >= m_settings->workMinutes() * 60)
        return;
    if (remainSec > threshold)
        return;

    m_preNoticeFired = true;
    emit preBreakNotice(remainSec);
}

void BreakScheduler::setAwaitingResume(bool awaiting)
{
    if (m_awaitingResume == awaiting)
        return;
    m_awaitingResume = awaiting;
    emit awaitingResumeChanged();
}

void BreakScheduler::clearPaused()
{
    if (!m_paused)
        return;
    m_paused = false;
    emit pausedChanged();
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
