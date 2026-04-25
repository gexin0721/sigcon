#include <QCoreApplication>
#include <QDate>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QStringList>
#include <QTextStream>

#include <iostream>

// 说明：
// 1. 强制启用 LOGGER_TEST_MODE，让 logger 进入“测试模式”路径。
// 2. 测试模式下每次写日志都会立即 flush，便于立刻校验文件内容。
#define LOGGER_TEST_MODE

// 说明：
// 1. 直接引入被测模块头与实现，保证测试模式编译路径完全一致。
// 2. 这属于模块内自测常见做法，避免改动上层构建系统。
#include "../include/logger.h"
#include "../src/logger.cpp"

namespace {

// 说明：测试结果统计结构，记录通过数与失败数。
struct TestStats {
    int passed = 0;
    int failed = 0;
};

// 说明：
// 1. 统一断言输出函数。
// 2. 条件为真记通过，否则记失败并打印原因。
void expect(bool condition, const QString &name, const QString &detail, TestStats &stats)
{
    if (condition) {
        ++stats.passed;
        std::cout << "[PASS] " << name.toStdString() << "\n";
        return;
    }

    ++stats.failed;
    std::cout << "[FAIL] " << name.toStdString() << " -> " << detail.toStdString() << "\n";
}

// 说明：
// 1. 读取目录下最新的 test_*.log 文件。
// 2. logger 测试模式文件名带毫秒时间戳，这里按时间排序取最新。
QString findLatestTestLog(const QString &dirPath)
{
    QDir dir(dirPath);
    QFileInfoList entries = dir.entryInfoList(
        QStringList() << "test_*.log",
        QDir::Files,
        QDir::Time);

    if (entries.isEmpty()) {
        return QString();
    }

    return entries.first().absoluteFilePath();
}

// 说明：
// 1. 读取文本文件全部内容。
// 2. 读取失败时返回空字符串，由调用方断言判断。
QString readAllText(const QString &filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }

    QTextStream in(&f);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    in.setEncoding(QStringConverter::Utf8);
#else
    in.setCodec("UTF-8");
#endif
    return in.readAll();
}

// 说明：
// 1. 用例1：验证写入后日志文件会被创建。
// 2. 用例2：验证日志级别过滤生效（DEBUG 被过滤，INFO 保留）。
// 3. 用例3：验证日志格式包含关键字段。
// 4. 用例4：验证多条写入都能保留在文件中。
void runLoggerSelfTests(TestStats &stats)
{
    // 说明：使用模块 test 目录下的临时日志目录，避免污染其他路径。
    const QString baseDir = QDir::currentPath() + "/logs_tmp";

    // 说明：每次测试前清空临时目录，确保结果可重复。
    QDir(baseDir).removeRecursively();
    QDir().mkpath(baseDir);

    // 说明：创建 logger 实例并执行测试写入。
    Logger logger(baseDir);

    // ------------------ 用例1：文件创建 ------------------
    logger.log(LogLevel::INFO,
               "CaseCreate",
               "/tmp/sample.cpp",
               10,
               "create check");

    const QString logPath = findLatestTestLog(baseDir);
    expect(!logPath.isEmpty(),
           "case1_file_created",
           "未找到 test_*.log 文件",
           stats);

    // ------------------ 用例2：等级过滤 ------------------
    logger.setLogLevel(LogLevel::INFO);
    logger.log(LogLevel::DEBUG,
               "CaseLevel",
               "/tmp/sample.cpp",
               20,
               "debug should be filtered");
    logger.log(LogLevel::INFO,
               "CaseLevel",
               "/tmp/sample.cpp",
               21,
               "info should be kept");

    // 说明：强制 flush，确保读取时内容完整。
    logger.flush();

    const QString contentAfterLevel = readAllText(logPath);
    const bool hasFilteredDebug = !contentAfterLevel.contains("debug should be filtered");
    const bool hasKeptInfo = contentAfterLevel.contains("info should be kept");
    expect(hasFilteredDebug,
           "case2_level_filter_debug",
           "DEBUG 日志未被过滤",
           stats);
    expect(hasKeptInfo,
           "case2_level_filter_info",
           "INFO 日志未写入",
           stats);

    // ------------------ 用例3：格式校验 ------------------
    logger.log(LogLevel::WARNING,
               "OrderSvc",
               "/root/project/main.cpp",
               42,
               "format check message");
    logger.flush();

    const QString contentAfterFormat = readAllText(logPath);
    const bool hasLevelTag = contentAfterFormat.contains("[WARNING]");
    const bool hasBizId = contentAfterFormat.contains("[OrderSvc]");
    const bool hasFileAndLine = contentAfterFormat.contains("[main.cpp:42]");
    const bool hasMessage = contentAfterFormat.contains("format check message");
    expect(hasLevelTag,
           "case3_format_level",
           "日志等级标签缺失",
           stats);
    expect(hasBizId,
           "case3_format_business_id",
           "业务 ID 字段缺失",
           stats);
    expect(hasFileAndLine,
           "case3_format_file_line",
           "文件名或行号字段缺失",
           stats);
    expect(hasMessage,
           "case3_format_message",
           "日志正文缺失",
           stats);

    // ------------------ 用例4：多条写入完整性 ------------------
    logger.log(LogLevel::ERR,
               "BatchSvc",
               "/a/b/c.cpp",
               100,
               "batch message 1");
    logger.log(LogLevel::FATAL,
               "BatchSvc",
               "/a/b/c.cpp",
               101,
               "batch message 2");
    logger.flush();

    const QString contentAfterBatch = readAllText(logPath);
    const bool hasBatch1 = contentAfterBatch.contains("batch message 1");
    const bool hasBatch2 = contentAfterBatch.contains("batch message 2");
    expect(hasBatch1,
           "case4_batch_message1",
           "第一条批量日志缺失",
           stats);
    expect(hasBatch2,
           "case4_batch_message2",
           "第二条批量日志缺失",
           stats);
}

} // namespace

int main(int argc, char *argv[])
{
    // 说明：Qt 文件与时间能力依赖应用对象，这里使用最轻量 QCoreApplication。
    QCoreApplication app(argc, argv);

    TestStats stats;
    runLoggerSelfTests(stats);

    std::cout << "[SUMMARY] passed=" << stats.passed
              << " failed=" << stats.failed << "\n";

    // 说明：失败时返回非 0，便于在 CI 或脚本中识别失败。
    return stats.failed == 0 ? 0 : 1;
}
