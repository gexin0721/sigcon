cmake_minimum_required(VERSION 3.16)

# 说明：
# 1. 该 CMake 仅用于 logger 模块内部自测。
# 2. 不接入全局工程，不改动上层 CMake。
# 3. 测试程序使用独立 main，符合“模块自治测试”原则。
project(logger_self_test LANGUAGES CXX)

# 说明：使用 C++17 与现有 desktop 工程保持一致。
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 说明：logger_self_test 使用了 std::thread，需要显式链接平台线程库（如 Linux 的 pthread）。
#       使用 CMake 官方 Threads 包可以跨平台解析正确的线程链接参数。
find_package(Threads REQUIRED)

# 说明：
# 1. 按“方案2”新增 logger 静态库目标。
# 2. 库目标显式编译 logger 实现文件，避免测试程序只看见声明导致链接缺符号。
# 3. 通过 PUBLIC include 暴露 logger 头文件目录，测试程序与其他目标可复用。
add_library(logger STATIC
    ../src/logger.cpp
)
target_include_directories(logger PUBLIC
    ../include
)
# 说明：
# 1. 自测目标强制启用 LOG_TEST_MODE，走“可多实例构造”的测试模式接口。
# 2. 宏设置在库目标上，确保 logger.cpp 与包含 logger.h 的调用方行为一致。
target_compile_definitions(logger PUBLIC
    LOG_TEST_MODE
)

# 说明：
# 1. 构建 logger 模块自测可执行程序。
# 2. 可执行程序只保留测试入口与测试逻辑，不再直接承载 logger 实现体。
add_executable(logger_self_test
    logger_self_test.cpp
)

# 说明：链接 logger 静态库（方案2核心步骤）与 Qt Core。
target_link_libraries(logger_self_test PRIVATE
    logger
    Threads::Threads
)
