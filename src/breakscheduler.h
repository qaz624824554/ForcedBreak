#pragma once

#include <QDateTime>
#include <QObject>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

class AppSettings;
class QQmlEngine;
class QJSEngine;

/*!
 * 计时状态机：只管时间，不感知 UI 的存在。
 *
 * 计时基于 QDateTime 绝对时间戳而非累加 tick 计数，因此系统休眠/唤醒后
 * 不会漂移；若休眠期间已跨过休息触发点，唤醒后的第一次 tick 立即触发休息。
 *
 * 总开关 enabled 默认关闭且**不持久化**：每次启动程序都需手动开启，
 * 关闭状态下计时器停止，永远不会进入休息。
 *
 * paused 同样不持久化：暂停期间工作计时冻结，继续后从冻结处接着走
 * （实现上把 m_phaseStart 整体后移暂停时长，绝对时间戳的抗漂移特性不受影响）。
 * 休息进行中拒绝暂停——那等同于免密码解锁。
 *
 * 休息倒计时归零后不会自动结束，而是进入 awaitingResume 等待态：
 * 遮罩与键盘钩子保持不动，直到用户点击遮罩上的「开始下一轮」按钮
 * （resumeWork()）才结束休息、重开工作周期。
 */
class BreakScheduler : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool breaking READ isBreaking NOTIFY breakingChanged)
    Q_PROPERTY(bool paused READ isPaused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(bool awaitingResume READ isAwaitingResume NOTIFY awaitingResumeChanged)
    Q_PROPERTY(int breakRemainingSeconds READ breakRemainingSeconds NOTIFY breakRemainingSecondsChanged)
    Q_PROPERTY(int workRemainingSeconds READ workRemainingSeconds NOTIFY workRemainingSecondsChanged)

public:
    explicit BreakScheduler(AppSettings *settings, QObject *parent = nullptr);

    //! QML 单例工厂：复用引擎中已有的 AppSettings 单例。
    static BreakScheduler *create(QQmlEngine *engine, QJSEngine *jsEngine);

    bool isEnabled() const { return m_enabled; }
    bool isBreaking() const { return m_breaking; }
    bool isPaused() const { return m_paused; }
    bool isAwaitingResume() const { return m_awaitingResume; }
    int breakRemainingSeconds() const { return m_breakRemaining; }
    int workRemainingSeconds() const { return m_workRemaining; }

public slots:
    /*!
     * 开启/关闭强制休息总开关。
     * 开启即从头开始一个完整工作周期；关闭则停止计时。
     * 休息进行中拒绝关闭——否则它就是绕过密码的逃逸出口。
     */
    void setEnabled(bool enabled);
    /*!
     * 暂停/继续工作计时。
     * 暂停期间剩余时间冻结，继续后接着走；未启用或休息进行中一律无效
     * ——休息中暂停就是绕过密码的逃逸出口。
     */
    void setPaused(bool paused);
    //! 立即进入休息（已启用且处于工作计时中才有效）。
    void triggerBreakNow();
    //! 重置计时：丢弃已累计的工作时间，从头开始一个完整工作周期（休息中无效）。
    void resetTimer();
    //! 密码校验通过后提前结束休息；本次休息作废，重新开始完整工作周期。
    void unlock();
    /*!
     * 休息倒计时已归零、用户点击「开始下一轮」后结束休息并重开工作周期。
     * 仅在 awaitingResume 为真时有效——否则它就是绕过倒计时的逃逸出口。
     */
    void resumeWork();

signals:
    void breakStarted(int totalSec);
    //! 距本次休息还剩 remainSec 秒（每个工作周期最多发一次）。
    void preBreakNotice(int remainSec);
    void tick(int remainSec);
    void breakEnded();
    void enabledChanged();
    void breakingChanged();
    void pausedChanged();
    void awaitingResumeChanged();
    void breakRemainingSecondsChanged();
    void workRemainingSecondsChanged();

private:
    //! 开始（或重新开始）一个完整的工作计时周期。
    void startWorkCycle();
    void onTimeout();
    void beginBreak();
    //! 休息倒计时归零：停表进入等待态，遮罩与钩子保持不动。
    void beginAwaitingResume();
    void endBreak();
    void setAwaitingResume(bool awaiting);
    void setBreakRemaining(int seconds);
    //! 剩余时间进入提前提醒阈值时发一次 preBreakNotice。
    void maybeEmitPreNotice(int remainSec);
    //! 解除暂停状态（不调整计时），供开始休息/重开周期/关闭总开关复用。
    void clearPaused();
    void setWorkRemaining(int seconds);

    AppSettings *m_settings = nullptr;
    QTimer m_timer;
    bool m_enabled = false;   //!< 总开关，不持久化，每次启动默认关闭
    bool m_breaking = false;
    bool m_paused = false;        //!< 暂停标志，不持久化
    QDateTime m_pauseStart;       //!< 暂停开始的绝对时刻，继续时据此后移 m_phaseStart
    bool m_awaitingResume = false; //!< 休息已满、等待用户点击「开始下一轮」（休息中的子状态）
    bool m_preNoticeFired = false; //!< 本工作周期是否已发过提前提醒
    QDateTime m_phaseStart;   //!< 当前阶段（工作或休息）的起始绝对时刻
    int m_breakTotal = 0;     //!< 本次休息的总时长，进入休息时锁定
    int m_breakRemaining = 0;
    int m_workRemaining = 0;
};
