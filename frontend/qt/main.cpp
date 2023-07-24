#include "mothmanwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MothmanWindow w;
    w.show();
    return a.exec();
}
