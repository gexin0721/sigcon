#ifndef DESKTOP_SRC_SUPPORT_CORE_EXCEPTIONS_APPBASEEXCEPTION_H
#define DESKTOP_SRC_SUPPORT_CORE_EXCEPTIONS_APPBASEEXCEPTION_H

#include <stdexcept>
#include <string>

/*
 * 思路说明：
 * 1. 该类是全局异常基类占位，用于后续统一错误码、重试策略和错误来源。
 * 2. 当前保持最小实现，避免在骨架阶段过度设计异常系统。
 * 3. 后续扩展时优先保持接口稳定，再补充分级和恢复策略字段。
 */
class AppBaseException : public std::runtime_error
{
public:
    explicit AppBaseException(const std::string &message)
        : std::runtime_error(message)
    {
    }
};

#endif
