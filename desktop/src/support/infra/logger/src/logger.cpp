#include "../include/logger.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <cstdlib>   // abort()

// ═════════════════════════════════════════════════════════════════════════════
//  运行模式：静态成员定义 + init / instance
// ═════════════════════════════════════════════════════════════════════════════

#ifndef LOG_TEST_MODE

std::unique_ptr<Logger> Logger::s_instance;
std::mutex              Logger::s_initMutex;

void Logger::init(const std::string& prefix,
                  const std::string& id,
                  const std::string& fileLocation)
{
    std::lock_guard<std::mutex> lock(s_initMutex);
    if (s_instance) return;   // 已初始化，忽略重复调用

    s_instance.reset(new Logger());
    s_instance->m_prefix       = prefix;
    s_instance->m_id           = id;
    s_instance->m_fileLocation = fileLocation;
    s_instance->m_level        = LogLevel::DEBUG;
    s_instance->openFile();
}

Logger& Logger::instance()
{
    if (!s_instance) {
        throw std::runtime_error(
            "[Logger] 单例尚未初始化，请先调用 Logger::init()");
    }
    return *s_instance;
}

#endif // !LOG_TEST_MODE

// ═════════════════════════════════════════════════════════════════════════════
//  测试模式：构造函数
// ═════════════════════════════════════════════════════════════════════════════

#ifdef LOG_TEST_MODE

Logger::Logger(const std::string& prefix,
               const std::string& id,
               const std::string& fileLocation)
    : m_prefix(prefix)
    , m_id(id)
    , m_fileLocation(fileLocation)
    , m_level(LogLevel::DEBUG)
{
    openFile();
}

#endif // LOG_TEST_MODE

// ═════════════════════════════════════════════════════════════════════════════
//  析构
// ═════════════════════════════════════════════════════════════════════════════

Logger::~Logger()
{
    if (m_file.is_open()) {
        m_file.flush();
        m_file.close();
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  公共接口
// ═════════════════════════════════════════════════════════════════════════════

void Logger::setLevel(LogLevel level)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_level = level;
}

void Logger::debug(const std::string& msg) { log(LogLevel::DEBUG, msg); }
void Logger::info (const std::string& msg) { log(LogLevel::INFO,  msg); }
void Logger::warn (const std::string& msg) { log(LogLevel::WARN,  msg); }
void Logger::error(const std::string& msg) { log(LogLevel::ERROR, msg); }

void Logger::fatal(const std::string& msg)
{
    log(LogLevel::FATAL, msg);
    std::abort();   // 不可恢复，直接终止
}

// ═════════════════════════════════════════════════════════════════════════════
//  核心写入
// ═════════════════════════════════════════════════════════════════════════════

void Logger::log(LogLevel level, const std::string& msg)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // 等级过滤
    if (level < m_level) return;

    // 运行模式：检查是否需要换日滚动
#ifndef LOG_TEST_MODE
    rollIfNewDay();
#endif

    // 拼装日志行
    // 格式：【时间】【ID】【文件位置】【级别】 消息
    std::ostringstream oss;
    oss << "【" << nowDatetime()       << "】"
        << "【" << m_id               << "】"
        << "【" << m_fileLocation     << "】"
        << "【" << levelToStr(level)  << "】 "
        << msg;

    const std::string line = oss.str();

    // 写文件
    if (m_file.is_open()) {
        m_file << line << '\n';
        m_file.flush();
    }

    // 同步打印到终端（stdout）
    std::cout << line << '\n';
}

// ═════════════════════════════════════════════════════════════════════════════
//  文件管理
// ═════════════════════════════════════════════════════════════════════════════

void Logger::openFile()
{
    if (m_file.is_open()) {
        m_file.flush();
        m_file.close();
    }

    std::string filename;

#ifdef LOG_TEST_MODE
    // 测试模式：前缀 + 时间戳，每次构造生成独立文件
    filename = m_prefix + "_" + nowTimestamp() + ".log";
#else
    // 运行模式：前缀 + 日期，以追加方式写入当天文件
    m_currentDate = nowDate();
    filename = m_prefix + "_" + m_currentDate + ".log";
#endif

    // 追加模式打开，保留已有内容（运行模式同一天重启不覆盖）
    m_file.open(filename, std::ios::app);
    if (!m_file.is_open()) {
        std::cerr << "[Logger] 无法打开日志文件: " << filename << '\n';
    }
}

void Logger::rollIfNewDay()
{
    // 调用前必须已持有 m_mutex
    const std::string today = nowDate();
    if (today != m_currentDate) {
        // 日期已变，切换到新文件
        openFile();
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  工具函数
// ═════════════════════════════════════════════════════════════════════════════

std::string Logger::levelToStr(LogLevel level) const
{
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::WARN:  return "WARN ";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default:              return "?????";
    }
}

std::string Logger::nowDatetime() const
{
    using namespace std::chrono;

    const auto now = system_clock::now();
    const auto ms  = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    const std::time_t t = system_clock::to_time_t(now);

    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
        << '.'
        << std::setw(3) << std::setfill('0') << ms.count();
    return oss.str();
}

std::string Logger::nowDate() const
{
    const std::time_t t = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d");
    return oss.str();
}

std::string Logger::nowTimestamp() const
{
    const std::time_t t = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return oss.str();
}