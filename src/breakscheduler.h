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
 */
class BreakScheduler : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool breaking READ isBreaking NOTIFY breakingChanged)
    Q_PROPERTY(int breakRemainingSeconds READ breakRemainingSeconds NOTIFY breakRemainingSecondsChanged)
    Q_PROPERTY(int workRemainingSeconds READ workRemainingSeconds NOTIFY workRemainingSecondsChanged)

public:
    explicit BreakScheduler(AppSettings *settings, QObject *parent = nullptr);

    //! QML 单例工厂：复用引擎中已有的 AppSettings 单例。
    static BreakScheduler *create(QQmlEngine *engine, QJSEngine *jsEngine);

    bool isEnabled() const { return m_enabled; }
    bool isBreaking() const { return m_breaking; }
    int breakRemainingSeconds() const { return m_breakRemaining; }
    int workRemainingSeconds() const { return m_workRemaining; }

public slots:
    /*!
     * 开启/关闭强制休息总开关。
     * 开启即从头开始一个完整工作周期；关闭则停止计时。
     * 休息进行中拒绝关闭——否则它就是绕过密码的逃逸出口。
     */
    void setEnabled(bool enabled);
    //! 立即进入休息（已启用且处于工作计时中才有效）。
    void triggerBreakNow();
    //! 重置计时：丢弃已累计的工作时间，从头开始一个完整工作周期（休息中无效）。
    void resetTimer();
    //! 密码校验通过后提前结束休息；本次休息作废，重新开始完整工作周期。
    void unlock();

signals:
    void breakStarted(int totalSec);
    void tick(int remainSec);
    void breakEnded();
    void enabledChanged();
    void breakingChanged();
    void breakRemainingSecondsChanged();
    void workRemainingSecondsChanged();

private:
    //! 开始（或重新开始）一个完整的工作计时周期。
    void startWorkCycle();
    void onTimeout();
    void beginBreak();
    void endBreak();
    void setBreakRemaining(int seconds);
    void setWorkRemaining(int seconds);

    AppSettings *m_settings = nullptr;
    QTimer m_timer;
    bool m_enabled = false;   //!< 总开关，不持久化，每次启动默认关闭
    bool m_breaking = false;
    QDateTime m_phaseStart;   //!< 当前阶段（工作或休息）的起始绝对时刻
    int m_breakTotal = 0;     //!< 本次休息的总时长，进入休息时锁定
    int m_breakRemaining = 0;
    int m_workRemaining = 0;
};
