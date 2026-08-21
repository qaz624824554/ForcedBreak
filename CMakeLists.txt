cmake_minimum_required(VERSION 3.16)

project(ForcedBreak VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Widgets 是托盘图标（QSystemTrayIcon/QMenu）的依赖，Qt Quick 无原生托盘支持
# Svg 用于把 assets/icons 下的矢量图标渲染成托盘/应用图标
find_package(Qt6 REQUIRED COMPONENTS Quick Widgets Svg)

qt_standard_project_setup(REQUIRES 6.10)

# Content of downloaded design from Figma to Qt or similar
if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/importedcontent/CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/importedcontent)
endif()

qt_add_executable(ForcedBreak
    main.cpp
    src/appsettings.h src/appsettings.cpp
    src/breakscheduler.h src/breakscheduler.cpp
    src/inputblocker.h src/inputblocker.cpp
    src/overlaycontroller.h src/overlaycontroller.cpp
    src/richtextformatter.h src/richtextformatter.cpp
    src/settingswindowmanager.h src/settingswindowmanager.cpp
    src/trayicon.h src/trayicon.cpp
)

# Windows 下把 .ico 编进 exe，资源管理器与桌面快捷方式才有图标。
# project() 只声明了 CXX，Ninja/Makefile 生成器下必须显式启用 RC 才会调 windres
if (WIN32)
    enable_language(RC)
    target_sources(ForcedBreak PRIVATE assets/app.rc)
endif()

# 生成的 qmltyperegistrations.cpp 以 <appsettings.h> 这样的裸文件名包含头文件，
# 因此 src/ 必须在头文件搜索路径中
target_include_directories(ForcedBreak PRIVATE src)

# 图标以 SVG 形式打进二进制，运行时按需渲染到各尺寸（见 TrayIcon::appIcon）
qt_add_resources(ForcedBreak "icons"
    PREFIX "/icons"
    BASE assets/icons
    FILES
        assets/icons/app.svg
        assets/icons/tray-cup.svg
)

qt_add_qml_module(ForcedBreak
    URI ForcedBreak
    QML_FILES
        qml/Overlay.qml
        qml/SettingsWindow.qml
        qml/RichTextToolBar.qml
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(ForcedBreak PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.ForcedBreak
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(ForcedBreak
    PRIVATE Qt6::Quick Qt6::Widgets Qt6::Svg
)

install(TARGETS ForcedBreak
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
