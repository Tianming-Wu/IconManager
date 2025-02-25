#include "mgrmainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    qRegisterMetaType<IconInfo>("IconInfo");

    MgrMainWindow w;
    w.show();
    return a.exec();
}
