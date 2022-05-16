////////////////////////////////////////////////////////
///
///     "main.cpp"
///
///     Entry point to program
///
///     Author: Jarod Graygo
///
////////////////////////////////////////////////////////


#include "servercontroller.h"
#include "cachemanager.h"

#include <QApplication>

// Application entry point
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ServerController w;
    w.show();
    return a.exec();
}
