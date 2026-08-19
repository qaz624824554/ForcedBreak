#include "settingswindowmanager.h"

#include <QLoggingCategory>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickWindow>

Q_LOGGING_CATEGORY(lcSettingsWindow, "forcedbreak.settingswindow")

namespace {
constexpr auto kSettingsUrl = "qrc:/qt/qml/ForcedBreak/qml/SettingsWindow.qml";
}

SettingsWindowManager::SettingsWindowManager(QQmlEngine *engine, QObject *parent)
    : QObject(parent)
    , m_engine(engine)
{
    m_component = new QQmlComponent(m_engine, QUrl(QString::fromLatin1(kSettingsUrl)), this);
    if (m_component->isError())
        qCCritical(lcSettingsWindow) << "SettingsWindow.qml 加载失败：" << m_component->errorString();
}

SettingsWindowManager::~SettingsWindowManager()
{
    delete m_window.data();
}

void SettingsWindowManager::open(int tab)
{
    if (!m_window) {
        if (!m_component->isReady())
            return;
        QObject *object = m_component->create(m_engine->rootContext());
        m_window = qobject_cast<QQuickWindow *>(object);
        if (!m_window) {
            qCWarning(lcSettingsWindow) << "设置窗口创建失败：" << m_component->errorString();
            delete object;
            return;
        }
    }

    m_window->setProperty("currentTab", tab);
    m_window->show();
    m_window->raise();
    m_window->requestActivate();
}
