cmake_minimum_required(VERSION 3.24)

# 做什么：定义桌面端最小 Qt 可执行目标。
# 怎么做：查找 Qt Widgets，编译 L3 入口文件，生成可调试程序。
# 为什么：保证使用 CMakePresets 的 desktop-debug 时，F5 能进入桌面端调试。

project(sigcon_desktop VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(sigcon_desktop
    src/scheduler/l3_app/main_entry.cpp
)

target_link_libraries(sigcon_desktop
    PRIVATE
        Qt6::Widgets
)

target_compile_features(sigcon_desktop PRIVATE cxx_std_20)
