#include <QApplication>
#include <QLabel>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QLabel helloLabel("hello word");
    helloLabel.resize(420, 180);
    helloLabel.setAlignment(Qt::AlignCenter);
    helloLabel.setWindowTitle("SIGCON L3");
    helloLabel.show();

    return app.exec();
}
