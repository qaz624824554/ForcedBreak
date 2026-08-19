#include "inputblocker.h"

#include <QLoggingCategory>

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

Q_LOGGING_CATEGORY(lcInputBlocker, "forcedbreak.inputblocker")

namespace {

#ifdef Q_OS_WIN

HHOOK g_keyboardHook = nullptr;

//! 判断某个按键事件是否属于需要拦截的切换类快捷键。
bool shouldBlock(const KBDLLHOOKSTRUCT *info)
{
    const bool altDown = (info->flags & LLKHF_ALTDOWN) != 0;
    const bool ctrlDown = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;

    switch (info->vkCode) {
    case VK_LWIN:
    case VK_RWIN:
        // 拦截 Win 键即同时拦下 Win+D、Win+E 等一整族快捷键
        return true;
    case VK_TAB:
        return altDown;             // Alt+Tab
    case VK_ESCAPE:
        return altDown || ctrlDown; // Alt+Esc / Ctrl+Esc
    case VK_F4:
        return altDown;             // Alt+F4
    default:
        return false;
    }
}

LRESULT CALLBACK keyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION) {
        const auto *info = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);
        if (shouldBlock(info))
            return 1; // 非零返回值表示吞掉该按键
    }
    return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
}

#endif // Q_OS_WIN

} // namespace

InputBlocker::InputBlocker(QObject *parent)
    : QObject(parent)
{
    m_focusTimer.setInterval(500);
    connect(&m_focusTimer, &QTimer::timeout, this, &InputBlocker::focusStealRequested);
}

InputBlocker::~InputBlocker()
{
    removeHook();
}

void InputBlocker::engage()
{
    if (m_engaged)
        return;
    m_engaged = true;

    if (!installHook())
        qCWarning(lcInputBlocker) << "键盘钩子安装失败，遮罩仍照常显示（降级运行）";

    m_focusTimer.start();
}

void InputBlocker::disengage()
{
    if (!m_engaged)
        return;
    m_engaged = false;
    m_focusTimer.stop();
    removeHook();
}

bool InputBlocker::installHook()
{
#ifdef Q_OS_WIN
    if (g_keyboardHook)
        return true;
    g_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboardProc, GetModuleHandle(nullptr), 0);
    return g_keyboardHook != nullptr;
#else
    return true;
#endif
}

void InputBlocker::removeHook()
{
#ifdef Q_OS_WIN
    if (g_keyboardHook) {
        UnhookWindowsHookEx(g_keyboardHook);
        g_keyboardHook = nullptr;
    }
#endif
}
