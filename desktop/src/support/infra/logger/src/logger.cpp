#include "logger.h"

#include <QDebug>
#include <QDir>

// ============================================================
//  可调参数
// ============================================================
static constexpr qint64 kDefaultMaxBytes = 50LL * 1024 * 1024; // 50 MB / 文件

// ============================================================
//  运行模式静态成员
// ============================================================
#ifndef LOGGER_TEST_MODE
Logger *Logger::s_instance = nullptr;
QMutex  Logger::s_initMutex;
#endif

// ============================================================
//  构造 / 析构
// ============================================================
Logger::Logger(const QString &logDir)
    : m_logDir(logDir)
    , m_curDate(QDate::currentDate())
    , m_maxBytes(kDefaultMaxBytes)
{
    QDir().mkpath(logDir);

#ifdef LOGGER_TEST_MODE
    // 测试模式：文件名 = 时间戳（精确到毫秒，避免多实例冲突）
    QString ts   = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz");
    QString path = QString("%1/test_%2.log").arg(logDir, ts);
    openFile(path);
#else
    // 运行模式：按日期命名
    openFile(buildRunFilePath());
#endif
}

Logger::~Logger()
{
    QMutexLocker lk(&m_mutex);
    if (m_file.isOpen()) {
        m_stream.flush();
        m_file.close();
    }
}

// ============================================================
//  等级控制
// ============================================================
void Logger::setLogLevel(LogLevel level)
{
    QMutexLocker lk(&m_mutex);
    m_logLevel = level;
}

LogLevel Logger::logLevel() const
{
    // 读操作，简单处理（若需严格安全可加锁）
    return m_logLevel;
}

// ============================================================
//  手动刷盘
// ============================================================
void Logger::flush()
{
    QMutexLocker lk(&m_mutex);
    m_stream.flush();
}

// ============================================================
//  核心写入
// ============================================================
void Logger::log(LogLevel       level,
                 const QString &businessId,
                 const QString &file,
                 int            line,
                 const QString &message)
{
    // 等级过滤（无锁快速判断）
    if (level < m_logLevel) return;

    QMutexLocker lk(&m_mutex);

#ifndef LOGGER_TEST_MODE
    // 运行模式：检查是否需要滚动文件
    checkRolling();
#endif

    // ---- 格式化一行 -----------------------------------------
    //  [2025-01-15 14:23:05.123][INFO   ][OrderSvc][main.cpp:42] 下单成功
    QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    QString shortFile = QFileInfo(file).fileName();   // 只保留文件名，不含路径
    QString tag = levelToTag(level);

    QString entry = QString("[%1][%2][%3][%4:%5] %6")
                        .arg(timeStr)
                        .arg(tag)
                        .arg(businessId)
                        .arg(shortFile)
                        .arg(line)
                        .arg(message);

    // ---- 写文件 ---------------------------------------------
    if (m_file.isOpen()) {
        m_stream << entry << '\n';

#ifdef LOGGER_TEST_MODE
        // 测试模式：每次立即刷盘，确保崩溃时日志不丢
        m_stream.flush();
#else
        // 运行模式：ERROR / FATAL 强制刷盘，其余缓冲（性能优先）
        if (level >= LogLevel::ERR) {
            m_stream.flush();
        }
#endif
    }

    // ---- 写控制台 -------------------------------------------
    writeToConsole(level, entry);
}

// ============================================================
//  私有：打开文件
// ============================================================
void Logger::openFile(const QString &filePath)
{
    if (m_file.isOpen()) {
        m_stream.flush();
        m_file.close();
    }
    m_file.setFileName(filePath);
    if (!m_file.open(QIODevice::Append | QIODevice::Text)) {
        qCritical() << "[Logger] 无法打开日志文件：" << filePath;
        return;
    }
    m_stream.setDevice(&m_file);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    m_stream.setEncoding(QStringConverter::Utf8);
#else
    m_stream.setCodec("UTF-8");
#endif
}

// ============================================================
//  私有：运行模式文件滚动检查
//    优先级 1：日期变了 → 新建当天文件，序号归零
//    优先级 2：当前文件超过 maxBytes → 同日内追加序号
// ============================================================
void Logger::checkRolling()
{
    QDate today = QDate::currentDate();
    if (today != m_curDate) {
        m_curDate   = today;
        m_fileIndex = 0;
        openFile(buildRunFilePath());
        return;
    }
    if (m_file.isOpen() && m_file.size() >= m_maxBytes) {
        ++m_fileIndex;
        openFile(buildRunFilePath());
    }
}

// ============================================================
//  私有：运行模式文件路径
//    无序号：log_2025-01-15.log
//    有序号：log_2025-01-15_1.log、log_2025-01-15_2.log …
// ============================================================
QString Logger::buildRunFilePath() const
{
    QString dateStr = m_curDate.toString("yyyy-MM-dd");
    if (m_fileIndex == 0) {
        return QString("%1/log_%2.log").arg(m_logDir, dateStr);
    }
    return QString("%1/log_%2_%3.log").arg(m_logDir, dateStr).arg(m_fileIndex);
}

// ============================================================
//  私有：等级标签（固定宽度，便于对齐）
// ============================================================
QString Logger::levelToTag(LogLevel level) const
{
    switch (level) {
    case LogLevel::DEBUG:   return "DEBUG  ";
    case LogLevel::INFO:    return "INFO   ";
    case LogLevel::WARNING: return "WARNING";
    case LogLevel::ERR:     return "ERROR  ";
    case LogLevel::FATAL:   return "FATAL  ";
    default:                return "UNKNOWN";
    }
}

// ============================================================
//  私有：控制台输出（利用 Qt 的日志系统，便于 IDE 跳转）
// ============================================================
void Logger::writeToConsole(LogLevel level, const QString &entry) const
{
    switch (level) {
    case LogLevel::DEBUG:
        qDebug().noquote()    << entry; break;
    case LogLevel::INFO:
        qInfo().noquote()     << entry; break;
    case LogLevel::WARNING:
        qWarning().noquote()  << entry; break;
    case LogLevel::ERR:
        qCritical().noquote() << entry; break;
    case LogLevel::FATAL:
        qCritical().noquote() << entry; break;
    default:
        qDebug().noquote()    << entry; break;
    }
}

// ============================================================
//  运行模式：单例接口
// ============================================================
#ifndef LOGGER_TEST_MODE

bool Logger::init(const QString &logDir)
{
    QMutexLocker lk(&s_initMutex);
    if (s_instance) {
        qWarning() << "[Logger] init() 已被调用过，忽略本次调用";
        return false;
    }
    s_instance = new Logger(logDir);
    return true;
}

Logger *Logger::instance()
{
    // 正常运行期间 s_instance 已初始化，无需加锁（读指针是原子的）
    Q_ASSERT_X(s_instance != nullptr,
               "Logger::instance()",
               "请先调用 Logger::init() 完成初始化");
    return s_instance;
}

void Logger::destroy()
{
    QMutexLocker lk(&s_initMutex);
    delete s_instance;
    s_instance = nullptr;
}

#endif // LOGGER_TEST_MODE