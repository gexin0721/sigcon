#pragma once

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QString>
#include <QTextStream>

// ============================================================
//  模式切换：
//    测试模式 —— 在 .pro 中加 DEFINES += LOGGER_TEST_MODE
//              或在包含本头文件前 #define LOGGER_TEST_MODE
//    运行模式 —— 不定义即可，全局单例
// ============================================================

// ---- 日志等级 -----------------------------------------------
enum class LogLevel : int {
    DEBUG   = 0,
    INFO    = 1,
    WARNING = 2,
    ERR     = 3,   // ERROR 与 Windows 宏冲突，使用 ERR
    FATAL   = 4
};

// ---- 核心类 -------------------------------------------------
class Logger {
public:
    // --------------------------------------------------------
    // 构造 / 析构
    //   测试模式：直接 new Logger(dir) 即可，文件名含时间戳
    //   运行模式：通过 Logger::init() / Logger::instance() 使用
    // --------------------------------------------------------
    explicit Logger(const QString &logDir = "logs");
    ~Logger();

    // 禁止拷贝
    Logger(const Logger &)            = delete;
    Logger &operator=(const Logger &) = delete;

    // ---- 等级控制 -------------------------------------------
    void     setLogLevel(LogLevel level);
    LogLevel logLevel() const;

    // ---- 核心写入（建议通过宏调用，自动带文件/行号）-----------
    void log(LogLevel       level,
             const QString &businessId,
             const QString &file,
             int            line,
             const QString &message);

    // ---- 手动刷盘 -------------------------------------------
    void flush();

#ifndef LOGGER_TEST_MODE
    // --------------------------------------------------------
    // 运行模式单例接口
    //   1. main() 里调用 Logger::init() 初始化一次
    //   2. 全局通过 Logger::instance() 获取指针
    //   3. 退出前调用 Logger::destroy()（可选，程序退出会自动清理）
    // --------------------------------------------------------
    static bool    init(const QString &logDir = "logs");
    static Logger *instance();
    static void    destroy();
#endif

private:
    // ---- 内部辅助 -------------------------------------------
    void    openFile(const QString &filePath);
    void    checkRolling();          // 运行模式：检查是否需要滚动
    QString buildRunFilePath() const;
    QString levelToTag(LogLevel level) const;
    void    writeToConsole(LogLevel level, const QString &entry) const;

    // ---- 成员 -----------------------------------------------
    QFile       m_file;
    QTextStream m_stream;
    QMutex      m_mutex;

    QString     m_logDir;
    LogLevel    m_logLevel  = LogLevel::DEBUG;
    QDate       m_curDate;            // 运行模式：记录当前文件日期
    int         m_fileIndex = 0;      // 运行模式：同日内第几个文件
    qint64      m_maxBytes;           // 运行模式：单文件最大字节数

#ifndef LOGGER_TEST_MODE
    static Logger *s_instance;
    static QMutex  s_initMutex;
#endif
};

// ============================================================
//  便捷宏 —— 自动填充 __FILE__ 和 __LINE__
//
//  通用（任意 Logger 指针 ptr）：
//    LOG_DEBUG(ptr, "OrderSvc", "下单成功")
//
//  运行模式单例快捷（无需传 ptr）：
//    SLOG_INFO("PaySvc", "支付完成")
// ============================================================
#define LOG_DEBUG(ptr, id, msg) \
    (ptr)->log(LogLevel::DEBUG,   (id), __FILE__, __LINE__, (msg))
#define LOG_INFO(ptr, id, msg) \
    (ptr)->log(LogLevel::INFO,    (id), __FILE__, __LINE__, (msg))
#define LOG_WARNING(ptr, id, msg) \
    (ptr)->log(LogLevel::WARNING, (id), __FILE__, __LINE__, (msg))
#define LOG_ERR(ptr, id, msg) \
    (ptr)->log(LogLevel::ERR,     (id), __FILE__, __LINE__, (msg))
#define LOG_FATAL(ptr, id, msg) \
    (ptr)->log(LogLevel::FATAL,   (id), __FILE__, __LINE__, (msg))

#ifndef LOGGER_TEST_MODE
#define SLOG_DEBUG(id, msg) \
    Logger::instance()->log(LogLevel::DEBUG,   (id), __FILE__, __LINE__, (msg))
#define SLOG_INFO(id, msg) \
    Logger::instance()->log(LogLevel::INFO,    (id), __FILE__, __LINE__, (msg))
#define SLOG_WARNING(id, msg) \
    Logger::instance()->log(LogLevel::WARNING, (id), __FILE__, __LINE__, (msg))
#define SLOG_ERR(id, msg) \
    Logger::instance()->log(LogLevel::ERR,     (id), __FILE__, __LINE__, (msg))
#define SLOG_FATAL(id, msg) \
    Logger::instance()->log(LogLevel::FATAL,   (id), __FILE__, __LINE__, (msg))
#endif