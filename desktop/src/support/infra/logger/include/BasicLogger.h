#pragma once

/*
做什么：定义一个基础日志工具类，统一写入控制台与日志文件。
怎么做：核心接口使用标准 C++ 字符串，同时提供 QString 重载作为适配入口。
为什么：先满足桌面端当前的基础日志与数据落盘需求，并为后续扩展保留空间。
*/

#include <mutex>
#include <string>

class QString;

namespace support::utils::log {

// 日志等级。
// 这一层的作用很简单，就是告诉日志系统“这条消息属于什么性质”。
// 后面写入文件时，会把这个等级一起写进去，方便人眼排查问题。
enum class LogLevel {
    Debug,  // 调试信息：主要给开发阶段排查流程用，通常最啰嗦。
    Info,   // 普通信息：记录程序正常运行时的重要状态。
    Warn,   // 警告信息：程序还能继续运行，但这里已经出现异常迹象。
    Error   // 错误信息：已经发生明确错误，通常需要重点关注。
};

// BasicLogger 是一个“基础可用版”日志工具。
// 它当前只负责几件最核心的事情：
// 1. 把日志按追加模式写入文件。
// 2. 按需要把日志同时打印到控制台。
// 3. 自动在每条日志前面补上时间和等级。
// 4. 同时兼容 std::string 和 QString 两种入参。
//
// 这里故意不做太重的设计，例如异步队列、日志切分、按大小滚动文件等。
// 原因是你当前先要一个基础能用、容易看懂、容易扩展的版本。
class BasicLogger final {
public:
    // 默认构造。
    // 如果调用者没有主动指定日志文件路径，就使用默认路径 logs/sigcon.log。
    BasicLogger();

    // 带路径的构造函数。
    // 适合你在创建对象时就明确日志应该写到哪里。
    // 如果传入空字符串，内部仍然会回退到默认路径，避免出现“路径为空无法写入”的问题。
    explicit BasicLogger(const std::string& filePath);
    ~BasicLogger() = default;

    // 设置日志文件路径，参数为标准 C++ 字符串。
    // 这个函数只负责更新目标路径，不会立刻创建文件。
    // 真正的文件创建动作发生在第一次写日志时。
    void setLogFilePath(const std::string& filePath);

    // 设置日志文件路径，参数为 Qt 的 QString。
    // 这是一个适配入口，方便在 Qt 工程里直接传 QString，不需要调用方自己转换。
    void setLogFilePath(const QString& filePath);

    // 控制是否把日志同步打印到控制台。
    // true：写文件的同时也打印到控制台。
    // false：只写文件，不打印控制台。
    void setConsoleEnabled(bool enabled);

    // 下面这四组函数是最常用的快捷接口。
    // 它们本质上都会转到 log(...) 去执行，只是帮调用方少写一个日志等级参数。
    void debug(const std::string& message);
    void debug(const QString& message);

    void info(const std::string& message);
    void info(const QString& message);

    void warn(const std::string& message);
    void warn(const QString& message);

    void error(const std::string& message);
    void error(const QString& message);

    // 通用日志入口。
    // 当你不想只用固定的 debug/info/warn/error 快捷函数时，
    // 可以直接传入等级和消息，走统一日志流程。
    void log(LogLevel level, const std::string& message);
    void log(LogLevel level, const QString& message);

private:
    // 生成当前时间字符串。
    // 例如：2026-04-19 13:45:00
    // 之所以单独拆出来，是因为每条日志都要带时间戳，这属于公共逻辑。
    static std::string buildTimestamp();

    // 把日志等级枚举转成人能看懂的字符串，例如 INFO、ERROR。
    // 这样写入文件后，排查日志时不用再去反查枚举值。
    static std::string levelToString(LogLevel level);

    // 把 QString 转成 UTF-8 编码的 std::string。
    // 这样可以让底层写入逻辑统一只处理一套字符串类型，减少核心实现复杂度。
    static std::string fromQString(const QString& text);

    // 真正执行日志写入的核心函数。
    // 外层的 info/warn/error/log 最终都会收敛到这里。
    // 这里会统一完成加锁、拼接日志行、创建目录、写入文件、控制台输出。
    void writeLine(LogLevel level, const std::string& message);

    // 互斥锁。
    // 原因：多个线程如果同时写同一个日志文件，可能导致内容交叉、错乱甚至写坏格式。
    // 这里先用最直接、最容易理解的方式保证基础线程安全。
    std::mutex mutex_;

    // 当前日志文件路径。
    // 例如 logs/sigcon.log。
    // 每次写日志时都会使用这个路径作为目标文件。
    std::string filePath_;

    // 是否同时输出到控制台。
    // 默认值为 true，表示“除了写文件，也顺便打印出来”，这样开发阶段更容易观察程序行为。
    bool consoleEnabled_ = true;
};

}  // namespace support::utils::log
