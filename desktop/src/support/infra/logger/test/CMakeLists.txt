cmake_minimum_required(VERSION 3.16)

# 说明：
# 1. 该 CMake 仅用于 logger 模块内部自测。
# 2. 不接入全局工程，不改动上层 CMake。
# 3. 测试程序使用独立 main，符合“模块自治测试”原则。
project(logger_self_test LANGUAGES CXX)

# 说明：使用 C++17 与现有 desktop 工程保持一致。
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 说明：logger 当前依赖 QtCore 的时间、文件、字符串、互斥锁等能力。
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

# 说明：
# 1. 仅构建一个自测可执行程序。
# 2. logger_self_test.cpp 内部会以 LOGGER_TEST_MODE 方式包含 logger.cpp。
# 3. 这样可以确保测试模式与被测实现编译在同一编译单元内。
add_executable(logger_self_test
    logger_self_test.cpp
)

# 说明：暴露 logger 头文件目录，便于测试源文件 include "logger.h"。
target_include_directories(logger_self_test PRIVATE
    ../include
)

# 说明：链接 Qt Core。
target_link_libraries(logger_self_test PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
)
