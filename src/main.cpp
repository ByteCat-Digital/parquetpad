#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    MainWindow w;

    // Handle command line argument for opening a file
    if (argc > 1) {
        w.openFile(argv[1]);
    }

    w.show();
    return a.exec();
}
