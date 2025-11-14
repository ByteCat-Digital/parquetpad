#include <QApplication>
#include <QIcon>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/icons/app_icon.png"));
    MainWindow w;

    // Handle command line argument for opening a file
    if (argc > 1) {
        w.openFile(argv[1]);
    }

    w.show();
    return a.exec();
}
