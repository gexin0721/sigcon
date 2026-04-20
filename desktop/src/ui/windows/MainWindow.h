#ifndef DESKTOP_SRC_UI_WINDOWS_MAINWINDOW_H
#define DESKTOP_SRC_UI_WINDOWS_MAINWINDOW_H

#include <QMainWindow>

/*
 * 思路说明：
 * 1. 该文件属于纯 UI 层顶层窗口占位实现，只负责界面骨架，不承载业务逻辑。
 * 2. 当前阶段先提供一个最小可运行窗口，确保新的目录骨架建立后工程仍可编译启动。
 * 3. 后续如需接入业务层，必须通过 scheduler 层中转，不能在此处直接依赖 business。
 */
class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;
};

#endif
