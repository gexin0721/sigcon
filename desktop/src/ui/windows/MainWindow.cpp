#include "ui/windows/MainWindow.h"

#include <QLabel>
#include <QString>

#include "support/core/constants.h"

/*
 * 思路说明：
 * 1. 当前窗口只承担“展示骨架已切换完成”的职责，避免 UI 层提前侵入业务实现。
 * 2. 文本内容从 support/core/constants.h 读取，保证 UI 层只依赖支撑域。
 * 3. 这里故意保持实现简单，后续迁移时可以平滑替换为真正的主窗口布局。
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QString::fromUtf8(kAppWindowTitle));
    resize(960, 640);

    auto *label = new QLabel(QString::fromUtf8(kAppBootMessage), this);
    label->setAlignment(Qt::AlignCenter);
    setCentralWidget(label);
}
