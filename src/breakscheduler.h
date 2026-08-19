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
 */
class BreakScheduler : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(bool breaking READ isBreaking NOTIFY breakingChanged)
    Q_PROPERTY(int breakRemainingSeconds READ breakRemainingSeconds NOTIFY breakRemainingSecondsChanged)
    Q_PROPERTY(int workRemainingSeconds READ workRemainingSeconds NOTIFY workRemainingSecondsChanged)

public:
    explicit BreakScheduler(AppSettings *settings, QObject *parent = nullptr);

    //! QML 单例工厂：复用引擎中已有的 AppSettings 单例。
    static BreakScheduler *create(QQmlEngine *engine, QJSEngine *jsEngine);

    bool isBreaking() const { return m_breaking; }
    int breakRemainingSeconds() const { return m_breakRemaining; }
    int workRemainingSeconds() const { return m_workRemaining; }

public slots:
    //! 开始（或重新开始）一个完整的工作计时周期。
    void start();
    //! 立即进入休息（工作计时中才有效）。
    void triggerBreakNow();
    //! 密码校验通过后提前结束休息；本次休息作废，重新开始完整工作周期。
    void unlock();

signals:
    void breakStarted(int totalSec);
    void tick(int remainSec);
    void breakEnded();
    void breakingChanged();
    void breakRemainingSecondsChanged();
    void workRemainingSecondsChanged();

private:
    void onTimeout();
    void beginBreak();
    void endBreak();
    void setBreakRemaining(int seconds);
    void setWorkRemaining(int seconds);

    AppSettings *m_settings = nullptr;
    QTimer m_timer;
    bool m_breaking = false;
    QDateTime m_phaseStart;   //!< 当前阶段（工作或休息）的起始绝对时刻
    int m_breakTotal = 0;     //!< 本次休息的总时长，进入休息时锁定
    int m_breakRemaining = 0;
    int m_workRemaining = 0;
};
