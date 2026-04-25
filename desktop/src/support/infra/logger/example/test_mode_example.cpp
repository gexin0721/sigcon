/**
 * test_mode_example.cpp
 *
 * 说明：
 * 1. 本示例演示 logger 在“测试模式（LOG_TEST_MODE）”下的典型用法。
 * 2. 测试模式特点是：可以自由构造多个 Logger 实例，每个实例拥有独立日志文件。
 * 3. 适用场景：模块自治测试、离线功能演示、并行子模块日志隔离。
 */

#include "../include/logger.h"

#include <string>
#include <thread>
#include <vector>

int main()
{
    // 创建两个独立日志实例，分别模拟 sensor/network 两个子模块。
    Logger sensorLog("example_sensor", "SENSOR-MODULE", "example/test_mode_example.cpp");
    Logger networkLog("example_network", "NETWORK-MODULE", "example/test_mode_example.cpp");

    // 设定各自等级门限：
    // - sensor 保留全部日志。
    // - network 从 INFO 起输出，DEBUG 将被过滤。
    sensorLog.setLevel(LogLevel::DEBUG);
    networkLog.setLevel(LogLevel::INFO);

    // 写入 sensor 模块日志。
    sensorLog.debug("sensor: debug trace");
    sensorLog.info("sensor: warmup completed");
    sensorLog.warn("sensor: calibration drift detected");
    sensorLog.error("sensor: sample timeout");

    // 写入 network 模块日志，第一条 DEBUG 预期会被过滤。
    networkLog.debug("network: this debug should be filtered");
    networkLog.info("network: handshake ok");
    networkLog.warn("network: packet retry count high");
    networkLog.error("network: remote disconnected");

    // 并发写入演示：验证一个 logger 实例可被多线程安全调用。
    Logger mtLog("example_mt", "MULTI-THREAD", "example/test_mode_example.cpp");
    mtLog.setLevel(LogLevel::INFO);

    std::vector<std::thread> workers;
    workers.reserve(2);
    for (int t = 0; t < 2; ++t) {
        workers.emplace_back([&mtLog, t]() {
            for (int i = 0; i < 5; ++i) {
                mtLog.info("worker_" + std::to_string(t) + " iteration_" + std::to_string(i));
            }
        });
    }

    for (auto& worker : workers) {
        worker.join();
    }

    // 注意：fatal 会在记录日志后直接 abort，示例默认注释。
    // mtLog.fatal("example fatal");

    return 0;
}
