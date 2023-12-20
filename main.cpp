#include "membranelogger.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MembraneLogger w;
    w.show();
    return a.exec();
}
