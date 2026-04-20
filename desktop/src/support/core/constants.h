#ifndef DESKTOP_SRC_SUPPORT_CORE_CONSTANTS_H
#define DESKTOP_SRC_SUPPORT_CORE_CONSTANTS_H

/*
 * 思路说明：
 * 1. 这里放当前工程最小公共常量，供 UI 层和调度层安全共享。
 * 2. 常量保持只读文本用途，避免把运行时状态误塞进全局常量区。
 * 3. 后续如果常量数量增长，需要按协议、路径、界面文案继续拆分。
 */
inline constexpr const char *kAppName = "sigcon-desktop";
inline constexpr const char *kAppWindowTitle = "SigCon Desktop Skeleton";
inline constexpr const char *kAppBootMessage =
    "Desktop skeleton is ready.\nPlease continue implementing modules under the new layered structure.";

#endif
