#include "wittypi2window.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    WittyPi2Window w;
    w.show();

    return a.exec();
}
