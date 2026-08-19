#pragma once

#include <QObject>
#include <QPointer>

class QQmlEngine;
class QQmlComponent;
class QQuickWindow;

/*!
 * 管理唯一的设置窗口。三个"设置…"菜单项打开的是同一个窗口，
 * 仅自动切换到对应的 Tab，避免多屏环境下多窗口互相遮挡与状态同步问题。
 */
class SettingsWindowManager : public QObject
{
    Q_OBJECT

public:
    explicit SettingsWindowManager(QQmlEngine *engine, QObject *parent = nullptr);
    ~SettingsWindowManager() override;

public slots:
    //! 打开设置窗口并切到指定 Tab（窗口已打开时只切 Tab 并提到前台）。
    void open(int tab);

private:
    QQmlEngine *m_engine = nullptr;
    QQmlComponent *m_component = nullptr;
    QPointer<QQuickWindow> m_window;
};
