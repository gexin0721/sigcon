#ifndef DESKTOP_SRC_SCHEDULER_L3_APP_APP_CONTEXT_APPCONTEXT_H
#define DESKTOP_SRC_SCHEDULER_L3_APP_APP_CONTEXT_APPCONTEXT_H

#include <QString>

/*
 * 思路说明：
 * 1. AppContext 作为 L3 装配层的全局运行时上下文占位，负责描述应用启动期共享信息。
 * 2. 当前只保留最小字段，避免过早把大量全局状态塞进系统。
 * 3. 后续若新增字段，必须先确认该信息确实属于全局装配期上下文，而不是局部模块状态。
 */
struct AppContext
{
    QString applicationName;
};

#endif
