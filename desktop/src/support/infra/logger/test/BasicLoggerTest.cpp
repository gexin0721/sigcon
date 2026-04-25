#include "BasicLogger.h"

#include <QString>

#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

/*
做什么：验证基础日志类能够同时处理 std::string 与 QString 输入。
怎么做：写入测试日志文件后再读取内容，检查关键文本是否存在。
为什么：确认当前实现的基础落盘能力有效，避免后续接入时才暴露最基础的问题。
*/

int main() {
    namespace fs = std::filesystem;
    using support::utils::log::BasicLogger;

    // 测试日志文件固定写到 test 目录下，符合当前仓库对测试文件位置的约束。
    const fs::path testLogPath = fs::path("desktop/src/support/utils/Log/test/basic_logger_test.log");

    if (fs::exists(testLogPath)) {
        // 如果上一次测试遗留了旧文件，先删掉，避免旧内容干扰本次结果判断。
        fs::remove(testLogPath);
    }

    // 创建日志对象，并关闭控制台输出。
    // 这样测试更安静，重点只验证文件里是否真的写进去了内容。
    BasicLogger logger(testLogPath.string());
    logger.setConsoleEnabled(false);

    // 分别测试标准字符串入口和 QString 入口。
    // 这样可以确认“二合一”设计确实两边都能正常工作。
    logger.info(std::string("std message"));
    logger.warn(QString::fromUtf8("qt message"));

    // 尝试重新打开刚刚写入的日志文件。
    std::ifstream inputFile(testLogPath);
    assert(inputFile.is_open());

    // 把文件完整读出来，后面直接在整段文本里查关键字。
    std::ostringstream buffer;
    buffer << inputFile.rdbuf();
    const std::string content = buffer.str();

    // 断言 1：标准字符串写入的消息必须存在。
    assert(content.find("std message") != std::string::npos);

    // 断言 2：QString 写入的消息也必须存在。
    // 如果这个断言失败，通常说明 QString 到 std::string 的转换链路有问题。
    assert(content.find("qt message") != std::string::npos);

    return 0;
}
