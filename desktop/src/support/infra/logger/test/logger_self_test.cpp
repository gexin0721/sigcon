/**
 * logger_self_test.cpp
 *
 * 说明：
 * 1. 本文件是 logger 模块的“自治自测程序”，不依赖任何第三方测试框架。
 * 2. 程序通过返回码表示测试结果：
 *    - 返回 0：全部测试通过。
 *    - 返回 1：存在至少一个测试失败。
 *    - 返回 2：编译模式错误（未启用 LOG_TEST_MODE）。
 * 3. 测试覆盖两个核心能力：
 *    - 日志等级过滤是否生效（低等级消息必须被过滤）。
 *    - 多线程并发写入是否完整（不丢行、不混乱）。
 */

#include "../include/logger.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifdef LOG_TEST_MODE

namespace logger_self_test_detail {

/**
 * 生成测试前缀：
 * 1. 使用毫秒级时间戳拼接前缀，降低和历史文件重名的概率。
 * 2. 该前缀会直接用于 logger 的文件名前缀字段。
 */
std::string makePrefix(const std::string& base)
{
    const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return base + "_" + std::to_string(nowMs);
}

/**
 * 收集目录中以 prefix 开头的 .log 文件：
 * 1. 便于“测试前/测试后”做差集，定位本次测试新生成的日志文件。
 * 2. 避免误读历史日志导致断言结果失真。
 */
std::vector<std::filesystem::path> collectLogFilesByPrefix(const std::string& prefix)
{
    std::vector<std::filesystem::path> files;
    const std::filesystem::path cwd = std::filesystem::current_path();

    for (const auto& entry : std::filesystem::directory_iterator(cwd)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        const std::string name = entry.path().filename().string();
        const bool prefixMatch = (name.rfind(prefix + "_", 0) == 0);
        const bool logSuffix = (entry.path().extension() == ".log");
        if (prefixMatch && logSuffix) {
            files.push_back(entry.path());
        }
    }
    return files;
}

/**
 * 在“after”里找出“before”不存在的文件：
 * 1. 用最直接的字符串路径比较即可满足本测试规模。
 * 2. 返回值通常期望只有 1 个文件（每次测试一个 logger 前缀）。
 */
std::vector<std::filesystem::path> diffNewFiles(
    const std::vector<std::filesystem::path>& before,
    const std::vector<std::filesystem::path>& after)
{
    std::vector<std::filesystem::path> delta;
    for (const auto& candidate : after) {
        bool existed = false;
        for (const auto& oldFile : before) {
            if (oldFile == candidate) {
                existed = true;
                break;
            }
        }
        if (!existed) {
            delta.push_back(candidate);
        }
    }
    return delta;
}

/**
 * 读取文本文件全部行：
 * 1. 统一给各个测试复用，减少重复 I/O 代码。
 * 2. 若读取失败，返回空数组，由上层测试给出失败原因。
 */
std::vector<std::string> readAllLines(const std::filesystem::path& path)
{
    std::vector<std::string> lines;
    std::ifstream in(path);
    if (!in.is_open()) {
        return lines;
    }

    std::string line;
    while (std::getline(in, line)) {
        lines.push_back(line);
    }
    return lines;
}

/**
 * 子串匹配工具：
 * 1. 用于检查日志中是否出现特定等级标签或业务消息。
 * 2. 返回 true 表示命中至少一行。
 */
bool containsAnyLine(const std::vector<std::string>& lines, const std::string& token)
{
    for (const auto& line : lines) {
        if (line.find(token) != std::string::npos) {
            return true;
        }
    }
    return false;
}

/**
 * 测试1：等级过滤。
 * 目标：当 level=LogLevel::WARN 时，仅 WARN/ERROR 被写入。
 */
bool testLevelFilter()
{
    const std::string prefix = makePrefix("selftest_level");
    const auto before = collectLogFilesByPrefix(prefix);

    {
        Logger log(prefix, "SELF-TEST", "logger_self_test.cpp");
        log.setLevel(LogLevel::WARN);
        log.debug("DEBUG_SHOULD_NOT_EXIST");
        log.info("INFO_SHOULD_NOT_EXIST");
        log.warn("WARN_SHOULD_EXIST");
        log.error("ERROR_SHOULD_EXIST");
    }

    const auto after = collectLogFilesByPrefix(prefix);
    const auto delta = diffNewFiles(before, after);
    if (delta.size() != 1U) {
        std::cout << "[FAIL] testLevelFilter: 新日志文件数量异常，期望=1，实际="
                  << delta.size() << '\n';
        return false;
    }

    const auto lines = readAllLines(delta.front());
    if (lines.size() != 2U) {
        std::cout << "[FAIL] testLevelFilter: 日志行数异常，期望=2，实际="
                  << lines.size() << '\n';
        return false;
    }

    const bool hasWarn = containsAnyLine(lines, "【WARN 】");
    const bool hasError = containsAnyLine(lines, "【ERROR】");
    const bool hasDebugMsg = containsAnyLine(lines, "DEBUG_SHOULD_NOT_EXIST");
    const bool hasInfoMsg = containsAnyLine(lines, "INFO_SHOULD_NOT_EXIST");

    if (!hasWarn || !hasError || hasDebugMsg || hasInfoMsg) {
        std::cout << "[FAIL] testLevelFilter: 等级过滤结果不符合预期\n";
        return false;
    }

    std::cout << "[PASS] testLevelFilter\n";
    return true;
}

/**
 * 测试2：并发写入。
 * 目标：多线程写入后，日志总行数和消息完整性都符合预期。
 */
bool testMultiThreadWrite()
{
    const std::string prefix = makePrefix("selftest_thread");
    const auto before = collectLogFilesByPrefix(prefix);

    const int threadCount = 4;
    const int msgPerThread = 25;

    {
        Logger log(prefix, "SELF-TEST", "logger_self_test.cpp");
        log.setLevel(LogLevel::INFO);

        std::vector<std::thread> workers;
        workers.reserve(static_cast<std::size_t>(threadCount));

        for (int t = 0; t < threadCount; ++t) {
            workers.emplace_back([&log, t, msgPerThread]() {
                for (int i = 0; i < msgPerThread; ++i) {
                    std::ostringstream oss;
                    oss << "THREAD_" << t << "_MSG_" << i;
                    log.info(oss.str());
                }
            });
        }

        for (auto& th : workers) {
            th.join();
        }
    }

    const auto after = collectLogFilesByPrefix(prefix);
    const auto delta = diffNewFiles(before, after);
    if (delta.size() != 1U) {
        std::cout << "[FAIL] testMultiThreadWrite: 新日志文件数量异常，期望=1，实际="
                  << delta.size() << '\n';
        return false;
    }

    const auto lines = readAllLines(delta.front());
    const std::size_t expected = static_cast<std::size_t>(threadCount * msgPerThread);
    if (lines.size() != expected) {
        std::cout << "[FAIL] testMultiThreadWrite: 日志行数异常，期望="
                  << expected << "，实际=" << lines.size() << '\n';
        return false;
    }

    for (int t = 0; t < threadCount; ++t) {
        for (int i = 0; i < msgPerThread; ++i) {
            std::ostringstream oss;
            oss << "THREAD_" << t << "_MSG_" << i;
            if (!containsAnyLine(lines, oss.str())) {
                std::cout << "[FAIL] testMultiThreadWrite: 缺少消息 " << oss.str() << '\n';
                return false;
            }
        }
    }

    std::cout << "[PASS] testMultiThreadWrite\n";
    return true;
}

} // namespace logger_self_test_detail

int main()
{
    bool allPassed = true;

    if (!logger_self_test_detail::testLevelFilter()) {
        allPassed = false;
    }

    if (!logger_self_test_detail::testMultiThreadWrite()) {
        allPassed = false;
    }

    if (!allPassed) {
        std::cout << "[SUMMARY] logger_self_test: FAILED\n";
        return 1;
    }

    std::cout << "[SUMMARY] logger_self_test: PASSED\n";
    return 0;
}

#else

int main()
{
    std::cout << "[ERROR] logger_self_test 必须在 LOG_TEST_MODE 下编译运行\n";
    return 2;
}

#endif // LOG_TEST_MODE
