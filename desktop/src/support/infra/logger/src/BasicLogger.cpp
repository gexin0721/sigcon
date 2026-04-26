#include "BasicLogger.h"

#include <QByteArray>
#include <QString>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace support::utils::log {

namespace {

// 把 time_t 转成本地时间结构体。
// 之所以单独包一层，是为了兼容不同平台的安全时间转换函数：
// Windows 使用 localtime_s
// 其他常见平台使用 localtime_r
//
// 这样做的目的，是让后面的时间戳格式化逻辑更干净，不把平台差异散落到主流程里。
std::tm buildLocalTime(std::time_t currentTime) {
    std::tm localTime {};
#ifdef _WIN32
    localtime_s(&localTime, &currentTime);
#else
    localtime_r(&currentTime, &localTime);
#endif
    return localTime;
}

}  // namespace

BasicLogger::BasicLogger()
    : filePath_("logs/sigcon.log") {
    // 默认把日志写到项目下的 logs/sigcon.log。
    // 这个路径不算“唯一正确”，但足够直观，适合作为基础版本默认值。
}

BasicLogger::BasicLogger(const std::string& filePath)
    : filePath_(filePath.empty() ? "logs/sigcon.log" : filePath) {
    // 如果外部给了空路径，这里主动兜底成默认路径。
    // 这样调用者即使忘了传值，也不会把 logger 放进一个无效状态。
}

void BasicLogger::setLogFilePath(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(mutex_);
    // 这里也做同样的兜底处理，避免运行时把路径设置成空字符串后无法写入。
    filePath_ = filePath.empty() ? "logs/sigcon.log" : filePath;
}

void BasicLogger::setLogFilePath(const QString& filePath) {
    // Qt 风格路径入口。
    // 先转成标准字符串，再走核心实现，避免路径设置逻辑维护两份。
    setLogFilePath(fromQString(filePath));
}

void BasicLogger::setConsoleEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    // 这里只是修改一个开关位，不涉及写文件动作。
    consoleEnabled_ = enabled;
}

void BasicLogger::debug(const std::string& message) {
    // 快捷函数：固定把日志等级设为 Debug。
    log(LogLevel::Debug, message);
}

void BasicLogger::debug(const QString& message) {
    log(LogLevel::Debug, message);
}

void BasicLogger::info(const std::string& message) {
    log(LogLevel::Info, message);
}

void BasicLogger::info(const QString& message) {
    log(LogLevel::Info, message);
}

void BasicLogger::warn(const std::string& message) {
    log(LogLevel::Warn, message);
}

void BasicLogger::warn(const QString& message) {
    log(LogLevel::Warn, message);
}

void BasicLogger::error(const std::string& message) {
    log(LogLevel::Error, message);
}

void BasicLogger::error(const QString& message) {
    log(LogLevel::Error, message);
}

void BasicLogger::log(LogLevel level, const std::string& message) {
    writeLine(level, message);
}

void BasicLogger::log(LogLevel level, const QString& message) {
    writeLine(level, fromQString(message));
}

std::string BasicLogger::buildTimestamp() {
    // 先拿到当前系统时间点。
    const auto now = std::chrono::system_clock::now();
    const auto currentTime = std::chrono::system_clock::to_time_t(now);
    const std::tm localTime = buildLocalTime(currentTime);

    // 再格式化成统一的人类可读文本。
    // 统一格式的好处是：
    // 1. 人眼扫日志时更整齐。
    // 2. 后面如果做日志分析，也更容易解析。
    std::ostringstream output;
    output << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return output.str();
}

std::string BasicLogger::levelToString(LogLevel level) {
    // 把内部枚举值翻译成外部日志文本。
    // 最终文件里不会看到“枚举数字”，而是直接看到清晰的等级字符串。
    switch (level) {
    case LogLevel::Debug:
        return "DEBUG";
    case LogLevel::Info:
        return "INFO";
    case LogLevel::Warn:
        return "WARN";
    case LogLevel::Error:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

std::string BasicLogger::fromQString(const QString& text) {
    // QString 是 Qt 的字符串类型，而底层写入逻辑统一使用 std::string。
    // 所以这里先转成 UTF-8 字节序列，再构造成标准字符串。
    //
    // 选择 UTF-8 的原因：
    // 1. 跨平台文本处理更常见。
    // 2. 中文内容通常也能更稳定地保存下来。
    const QByteArray utf8Text = text.toUtf8();
    return std::string(utf8Text.constData(), static_cast<std::size_t>(utf8Text.size()));
}

void BasicLogger::writeLine(LogLevel level, const std::string& message) {
    // 进入核心写入流程后，第一件事就是加锁。
    // 这样可以避免多个线程同时往同一个文件写内容时相互打架。
    std::lock_guard<std::mutex> lock(mutex_);

    // 把最终要落盘的一整行日志先拼好。
    // 日志格式现在是：
    // [时间] [等级] 具体消息
    const std::string line = "[" + buildTimestamp() + "] [" + levelToString(level) + "] " + message;
    const std::filesystem::path targetPath(filePath_);
    const std::filesystem::path parentPath = targetPath.parent_path();

    if (!parentPath.empty()) {
        // 如果父目录不存在，就先创建。
        // 比如路径是 logs/sigcon.log，那么这里会尝试先建 logs 目录。
        // 这样调用方不需要提前手工准备目录。
        std::filesystem::create_directories(parentPath);
    }

    // 以追加模式打开文件。
    // 追加模式的含义是：新日志写在旧日志后面，而不是覆盖整个文件。
    std::ofstream outputFile(targetPath, std::ios::app);
    if (!outputFile.is_open()) {
        // 如果连文件都打不开，说明这条日志已经无法正常落盘。
        // 这里直接抛异常，让上层明确知道“日志写入失败”。
        throw std::runtime_error("BasicLogger failed to open log file: " + targetPath.string());
    }

    // 把这一行日志写入文件，并立即刷新缓冲区。
    // 立即 flush 的好处是程序异常退出时，日志更不容易丢失。
    outputFile << line << '\n';
    outputFile.flush();

    if (!consoleEnabled_) {
        // 如果外部关闭了控制台输出，就到这里直接结束。
        return;
    }

    if (level == LogLevel::Error) {
        // 错误日志优先输出到标准错误流，便于和普通信息区分。
        std::cerr << line << std::endl;
        return;
    }

    // 其他等级统一输出到标准输出流。
    std::cout << line << std::endl;
}

}  // namespace support::utils::log
