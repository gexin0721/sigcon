#ifndef DESKTOP_SRC_SUPPORT_CORE_SHARED_STATE_H
#define DESKTOP_SRC_SUPPORT_CORE_SHARED_STATE_H

/*
 * 思路说明：
 * 1. 共享状态区需要极度克制，当前只提供占位声明，不提前引入真实全局变量。
 * 2. 这样做是为了提醒后续开发者：只有跨模块且确实无法下沉的状态，才允许进入这里。
 * 3. 如果未来要新增内容，应优先评估是否能放进 AppContext 或局部对象。
 */
struct SharedState
{
    bool reserved = false;
};

#endif
