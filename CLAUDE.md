# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

ForcedBreak 是一个基于 Qt 6.10 Quick（CMake 构建，Windows 平台）的**强制休息提醒工具**：
工作计时到点后在**所有显示器**上弹出全屏黑色遮罩，同时安装键盘钩子阻止用户切走，
只有输入正确密码或等到倒计时结束才能恢复。程序**没有主窗口**，全部交互从系统托盘发起。

## 构建与运行

- 项目在 **Windows 的 Qt Creator** 中编译（MinGW 64-bit kit，Qt 6.11.1），Claude Code 运行在 WSL 中，
  **不要尝试在 WSL 里编译或运行**，改动完成后交由用户在 Windows 侧编译验证。
- 没有测试套件，也没有 lint 配置。

## 架构

`main.cpp` 是唯一的装配点：所有组件在这里创建，用 `QObject::connect` 串成信号流。
各个类之间**没有直接依赖**（`BreakScheduler` 不知道遮罩存在，`InputBlocker` 不知道谁在监听），
新增功能时请保持这个模式——在 `main.cpp` 里接线，而不是让组件互相持有指针。

核心信号流：

```
BreakScheduler::breakStarted → OverlayController::showOverlays() + InputBlocker::engage()
InputBlocker::focusStealRequested (每 500ms) → OverlayController::raiseOverlays()
BreakScheduler::breakEnded   → InputBlocker::disengage() + OverlayController::hideOverlays()
TrayIcon::breakNowRequested / resetRequested / enabledToggled / settingsRequested / quitRequested → 对应槽
Overlay.qml 密码正确 → BreakScheduler::unlock()（本次休息作废，重开完整工作周期）
```

### C++ 侧（`src/`）

| 类 | 职责 | 关键约束 |
|---|---|---|
| `AppSettings` | QSettings INI 配置读写 | QML 单例。setter 立即落盘；密码为 SHA-256 + 16 字节盐，默认 `123456`；`autoStart` 同步写注册表 `HKCU\...\Run` |
| `BreakScheduler` | 工作/休息状态机 | QML 单例，`create()` 工厂复用引擎里的 `AppSettings` 单例。计时基于 `QDateTime` **绝对时间戳**而非累加 tick，休眠唤醒后不漂移。总开关 `enabled` 不持久化、每次启动默认关闭，休息中拒绝关闭（否则是免密码逃逸出口） |
| `OverlayController` | 每块屏幕一个遮罩窗口 | 监听 `screenAdded/screenRemoved`：休息中新接入的显示器必须立刻补遮罩，否则就是免密码逃逸出口 |
| `InputBlocker` | Windows 低级键盘钩子 + 周期抢焦点 | 拦截 Alt+Tab / Win / Alt+F4 / Alt+Esc / Ctrl+Esc；Ctrl+Alt+Del 属安全桌面，无法拦截。钩子安装失败时**降级继续**而非中止 |
| `SettingsWindowManager` | 唯一设置窗口 | 三个托盘菜单项打开同一窗口，只切 Tab（序号见 `TrayIcon::SettingsTab`，须与 `SettingsWindow.qml` 的 TabBar 顺序一致） |
| `RichTextFormatter` | 桥接 QML TextArea 的 QTextDocument | QML 普通元素（非单例），对当前选区施加字符格式 |
| `TrayIcon` | 托盘图标与右键菜单 | 图标运行时代码绘制（`TrayIcon::appIcon()`），无图片资源。菜单是 `StayOpenMenu`：带 `forcedBreakStayOpen` 动态属性的项（两个开关、内嵌的工作时长调节行）点击后菜单不关闭。托盘改工作时长必须**先 `emit resetRequested()` 再 `setWorkMinutes()`**，否则调小时长会当场触发休息 |

### QML 侧（`qml/`）

QML 模块 URI 为 `ForcedBreak`，`Overlay.qml` / `SettingsWindow.qml` 由 C++ 通过
`QQmlComponent` 手动加载，URL 形如 `qrc:/qt/qml/ForcedBreak/qml/Overlay.qml`
（**新增需 C++ 加载的 QML 文件时要同步这个路径常量**）。
遮罩窗口用 `Qt.Tool | FramelessWindowHint | WindowStaysOnTopHint`，
使其不出现在任务栏和 Alt+Tab 列表中。

## 平台相关注意事项

- 用 `QApplication`（非 `QGuiApplication`）：`QSystemTrayIcon` 属于 QtWidgets，Qt Quick 无原生托盘支持。
- `setQuitOnLastWindowClosed(false)`：没有主窗口，设置窗口关闭不应退出程序。
- 单实例保护用 `QSharedMemory("ForcedBreak-SingleInstance")`，第二个实例弹气泡后静默退出。
- 所有 Windows API 调用（`windows.h`、注册表、钩子）都包在 `#ifdef Q_OS_WIN` 中，
  非 Windows 平台降级为空操作——新增平台相关代码请保持这个写法。
- `CMakeLists.txt` 中的 `target_include_directories(... PRIVATE src)` 是必需的：
  生成的 `qmltyperegistrations.cpp` 以裸文件名（`<appsettings.h>`）包含头文件。

## 注意事项

- 遵循 QT 最佳实践
