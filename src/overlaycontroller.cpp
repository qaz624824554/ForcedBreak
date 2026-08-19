#include "overlaycontroller.h"

#include <QGuiApplication>
#include <QLoggingCategory>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QScreen>

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

Q_LOGGING_CATEGORY(lcOverlay, "forcedbreak.overlay")

namespace {

constexpr auto kOverlayUrl = "qrc:/qt/qml/ForcedBreak/qml/Overlay.qml";

//! Windows 下强行把窗口提到前台。Qt 的 requestActivate() 会受前台锁定限制，
//! 这里附加线程输入队列后再 SetForegroundWindow，成功率更高。
void forceForeground(QQuickWindow *window)
{
#ifdef Q_OS_WIN
    const auto hwnd = reinterpret_cast<HWND>(window->winId());
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    const DWORD foregroundThread = GetWindowThreadProcessId(GetForegroundWindow(), nullptr);
    const DWORD currentThread = GetCurrentThreadId();
    if (foregroundThread != currentThread)
        AttachThreadInput(foregroundThread, currentThread, TRUE);
    SetForegroundWindow(hwnd);
    if (foregroundThread != currentThread)
        AttachThreadInput(foregroundThread, currentThread, FALSE);
#else
    Q_UNUSED(window);
#endif
}

} // namespace

OverlayController::OverlayController(QQmlEngine *engine, QObject *parent)
    : QObject(parent)
    , m_engine(engine)
{
    m_component = new QQmlComponent(m_engine, QUrl(QString::fromLatin1(kOverlayUrl)), this);
    if (m_component->isError())
        qCCritical(lcOverlay) << "Overlay.qml 加载失败：" << m_component->errorString();

    connect(qApp, &QGuiApplication::screenAdded, this, [this](QScreen *screen) {
        if (m_visible)
            createOverlayFor(screen);
    });
    connect(qApp, &QGuiApplication::screenRemoved, this, [this](QScreen *screen) {
        destroyOverlayFor(screen);
    });
}

OverlayController::~OverlayController()
{
    hideOverlays();
}

void OverlayController::showOverlays()
{
    if (m_visible)
        return;
    m_visible = true;
    const auto screens = QGuiApplication::screens();
    for (QScreen *screen : screens)
        createOverlayFor(screen);
}

void OverlayController::hideOverlays()
{
    m_visible = false;
    for (auto it = m_windows.begin(); it != m_windows.end(); ++it) {
        if (it.value())
            it.value()->deleteLater();
    }
    m_windows.clear();
}

void OverlayController::raiseOverlays()
{
    for (auto it = m_windows.begin(); it != m_windows.end(); ++it) {
        QQuickWindow *window = it.value();
        if (!window)
            continue;
        window->raise();
        forceForeground(window);
    }
}

void OverlayController::createOverlayFor(QScreen *screen)
{
    if (!screen || m_windows.contains(screen))
        return;
    if (!m_component->isReady()) {
        qCWarning(lcOverlay) << "Overlay 组件不可用，跳过屏幕" << screen->name();
        return;
    }

    QObject *object = m_component->create(m_engine->rootContext());
    auto *window = qobject_cast<QQuickWindow *>(object);
    if (!window) {
        // 单块屏幕失败不影响其余屏幕
        qCWarning(lcOverlay) << "屏幕" << screen->name() << "的遮罩创建失败：" << m_component->errorString();
        delete object;
        return;
    }

    window->setScreen(screen);
    // 直接指定几何而非 FullScreen：多显示器下行为更可控，且能正常盖住任务栏
    window->setGeometry(screen->geometry());
    window->show();
    window->raise();
    forceForeground(window);

    m_windows.insert(screen, window);
}

void OverlayController::destroyOverlayFor(QScreen *screen)
{
    auto it = m_windows.find(screen);
    if (it == m_windows.end())
        return;
    if (it.value())
        it.value()->deleteLater();
    m_windows.erase(it);
}
