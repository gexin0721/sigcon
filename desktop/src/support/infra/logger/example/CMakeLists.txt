cmake_minimum_required(VERSION 3.16)

# 说明：
# 1. 本 CMake 仅用于 logger 示例程序的独立编译演示。
# 2. 示例分两类目标：测试模式示例 + 运行模式示例。
project(logger_examples LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

# 运行模式库：不定义 LOG_TEST_MODE，暴露 init/instance 单例接口。
add_library(logger_runtime STATIC
    ../src/logger.cpp
)
target_include_directories(logger_runtime PUBLIC
    ../include
)

# 测试模式库：定义 LOG_TEST_MODE，暴露可构造多实例接口。
add_library(logger_test_mode STATIC
    ../src/logger.cpp
)
target_include_directories(logger_test_mode PUBLIC
    ../include
)
target_compile_definitions(logger_test_mode PUBLIC
    LOG_TEST_MODE
)

# 运行模式示例目标。
add_executable(runtime_mode_example
    runtime_mode_example.cpp
)
target_link_libraries(runtime_mode_example PRIVATE
    logger_runtime
    Threads::Threads
)

# 测试模式示例目标。
add_executable(test_mode_example
    test_mode_example.cpp
)
target_link_libraries(test_mode_example PRIVATE
    logger_test_mode
    Threads::Threads
)
