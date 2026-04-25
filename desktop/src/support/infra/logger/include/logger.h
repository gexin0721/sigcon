#pragma once

/**
 * Logger.h  —  双模式日志类
 *
 * 编译期切换模式：
 *   测试模式  —— 在编译选项或此文件顶部加  #define LOG_TEST_MODE
 *   运行模式  —— 不定义该宏（默认）
 *
 * CMake 示例：  target_compile_definitions(MyApp PRIVATE LOG_TEST_MODE)
 * qmake 示例：  DEFINES += LOG_TEST_MODE
 *
 * ─────────────────────────────────────────────────────────────────────────────
 * 测试模式用法：
 *     Logger log("myapp", "SensorModule", "sensor/reader.cpp");
 *     log.setLevel(LogLevel::DEBUG);
 *     log.info("started");
 *
 * 运行模式用法：
 *     // 程序启动时调用一次
 *     Logger::init("myapp", "MainProcess", "main.cpp");
 *     // 任意位置获取单例
 *     Logger::instance().warn("low memory");
 * ─────────────────────────────────────────────────────────────────────────────
 */

// 说明：
// 1. 不要在头文件内直接定义 LOG_TEST_MODE。
// 2. 由编译系统（如 CMake target_compile_definitions）显式传入该宏，
//    这样同一份源码可以在“测试模式”和“运行模式”之间稳定切换。
// 3. 如需手工快速试验，可临时取消下一行注释，但提交前应恢复。
// #define LOG_TEST_MODE

#include <string>
#include <fstream>
#include <mutex>
#include <memory>
#include <stdexcept>

// ── 日志等级 ──────────────────────────────────────────────────────────────────

enum class LogLevel : int {
    DEBUG = 0,  // 详细调试信息
    INFO  = 1,  // 常规运行信息
    WARN  = 2,  // 警告，不影响运行
    ERROR = 3,  // 错误，影响局部功能
    FATAL = 4   // 致命错误，输出后调用 abort()
};

// ── Logger 类 ─────────────────────────────────────────────────────────────────

class Logger {
public:

    // ── 公共接口（两种模式通用）────────────────────────────────────────────────

    /** 动态设置最低输出等级，低于此等级的日志直接丢弃 */
    void setLevel(LogLevel level);

    void debug(const std::string& msg);
    void info (const std::string& msg);
    void warn (const std::string& msg);
    void error(const std::string& msg);

    /**
     * fatal —— 输出日志后立即调用 abort() 终止程序。
     * 适用于不可恢复的致命错误。
     */
    void fatal(const std::string& msg);

    ~Logger();

    // ── 模式特有接口 ────────────────────────────────────────────────────────────

#ifdef LOG_TEST_MODE
    // ---- 测试模式：可自由构造多个实例 ----------------------------------------
    /**
     * @param prefix       日志文件名前缀（不含路径分隔符）
     * @param id           标识本实例的业务 ID，写入每行日志
     * @param fileLocation 标识调用来源的描述字符串（如模块名或相对路径）
     *
     * 文件名格式：<prefix>_<YYYYMMDDHHmmss>.log
     */
    explicit Logger(const std::string& prefix,
                    const std::string& id,
                    const std::string& fileLocation);

#else
    // ---- 运行模式：全局单例 ---------------------------------------------------
    /**
     * 必须在程序启动时调用一次，重复调用无效。
     * 文件名格式：<prefix>_<YYYYMMDD>.log，每天零点滚动到新文件。
     *
     * @param prefix       日志文件名前缀
     * @param id           标识本进程的业务 ID
     * @param fileLocation 标识调用来源的描述字符串
     */
    static void    init(const std::string& prefix,
                        const std::string& id,
                        const std::string& fileLocation);

    /**
     * 获取全局单例引用。
     * 若 init() 尚未调用，抛出 std::runtime_error。
     */
    static Logger& instance();

#endif // LOG_TEST_MODE

private:

    // 运行模式下禁止外部构造
#ifndef LOG_TEST_MODE
    Logger() = default;
    Logger(const Logger&)            = delete;
    Logger& operator=(const Logger&) = delete;

    static std::unique_ptr<Logger> s_instance;
    static std::mutex              s_initMutex;   // 保护 s_instance 初始化
#endif

    // ── 核心实现 ────────────────────────────────────────────────────────────────

    /** 统一日志写入入口，持有 m_mutex 时调用 */
    void log(LogLevel level, const std::string& msg);

    /** 打开（或重新打开）日志文件 */
    void openFile();

    /**
     * 检查日期是否已变更，若变更则滚动到新文件。
     * 仅运行模式需要；调用时必须已持有 m_mutex。
     */
    void rollIfNewDay();

    // ── 工具函数 ────────────────────────────────────────────────────────────────

    std::string levelToStr(LogLevel level) const;

    /** 返回带毫秒的完整时间字符串，用于日志行 "YYYY-MM-DD HH:MM:SS.mmm" */
    std::string nowDatetime() const;

    /** 返回 "YYYYMMDD"，用于运行模式文件名和换日检测 */
    std::string nowDate() const;

    /** 返回 "YYYYMMDD_HHmmss"，用于测试模式文件名 */
    std::string nowTimestamp() const;

    // ── 成员变量 ────────────────────────────────────────────────────────────────

    std::string   m_prefix;        // 文件名前缀
    std::string   m_id;            // 业务 ID（写入每行）
    std::string   m_fileLocation;  // 来源描述（写入每行）
    std::string   m_currentDate;   // 运行模式：记录当前日期用于换日检测
    std::ofstream m_file;          // 日志文件流
    LogLevel      m_level { LogLevel::DEBUG };  // 当前最低输出等级
    mutable std::mutex m_mutex;    // 保护所有成员的并发访问
};
