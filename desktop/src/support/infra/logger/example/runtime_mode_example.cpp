/**
 * runtime_mode_example.cpp
 *
 * 说明：
 * 1. 本示例演示 logger 在“运行模式（不定义 LOG_TEST_MODE）”下的用法。
 * 2. 运行模式特点是：全进程单例，启动时初始化一次，后续各模块统一调用。
 * 3. 适用场景：业务程序正式运行、跨模块统一日志汇聚、按日期滚动写盘。
 */

#include "../include/logger.h"

/**
 * 模拟业务模块 A 的日志调用。
 * 重点：模块内部不持有 logger 实例，只通过 Logger::instance() 获取全局单例。
 */
void moduleA()
{
    Logger::instance().debug("moduleA: begin task");
    Logger::instance().info("moduleA: task finished");
}

/**
 * 模拟业务模块 B 的日志调用。
 * 重点：可按不同严重级别记录运行状态和异常。
 */
void moduleB()
{
    Logger::instance().warn("moduleB: optional config not found, fallback to default");
    Logger::instance().error("moduleB: write transaction failed");
}

int main()
{
    // 程序入口初始化 logger 单例（仅需一次）。
    Logger::init("example_runtime", "PROCESS-MAIN", "example/runtime_mode_example.cpp");

    // 动态设置日志最低输出等级。
    // 当前设置为 INFO，因此 DEBUG 级日志将被过滤。
    Logger::instance().setLevel(LogLevel::INFO);

    // 各业务模块直接记录日志。
    moduleA();
    moduleB();

    Logger::instance().info("runtime example exit");

    // 注意：fatal 会在记录日志后终止进程，示例默认注释。
    // Logger::instance().fatal("unrecoverable error");

    return 0;
}
