cmake_minimum_required(VERSION 3.16)

project(desktop VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

# 思路：
# 1. 优先兼容当前机器可用的 Qt 版本，避免写死 Qt6 导致 Qt5 环境无法构建。
# 2. 保持自动生成工程的结构，减少对现有文件组织的扰动。
if(Qt6_FOUND)
    qt_standard_project_setup()
    qt_add_executable(desktop
        src/ui/windows/MainWindow.cpp
        src/ui/windows/MainWindow.h
        src/scheduler/l3_app/main_entry.cpp
    )
else()
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)

    add_executable(desktop
        src/ui/windows/MainWindow.cpp
        src/ui/windows/MainWindow.h
        src/scheduler/l3_app/main_entry.cpp
    )
endif()

target_include_directories(desktop PRIVATE src)

target_link_libraries(desktop PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

set_target_properties(desktop PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS desktop
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
