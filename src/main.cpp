#include <QApplication>
#include <QIcon>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon("/home/johan/projects/parquetpad/assets/icons/cat_dark_green_512x512.png"));
    MainWindow w;

    // Handle command line argument for opening a file
    if (argc > 1) {
        w.openFile(argv[1]);
    }

    w.show();
    return a.exec();
}
