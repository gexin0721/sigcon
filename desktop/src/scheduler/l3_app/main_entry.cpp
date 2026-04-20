#include <QApplication>

#include "scheduler/l3_app/app_context/AppContext.h"
#include "support/core/constants.h"
#include "ui/windows/MainWindow.h"

/*
 * 思路说明：
 * 1. main_entry.cpp 是新的单一启动入口，负责把 UI 与全局装配层连接起来。
 * 2. 当前只做最小启动流程，不在这里提前接入线程池、业务对象或复杂异常体系。
 * 3. 这样做的原因是先确保目录边界和入口边界落地，再逐步迁移后续模块。
 */
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    AppContext appContext;
    appContext.applicationName = QString::fromUtf8(kAppName);

    QApplication::setApplicationName(appContext.applicationName);

    MainWindow window;
    window.show();

    return app.exec();
}
