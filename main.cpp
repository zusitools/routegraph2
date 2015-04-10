#include "mainwindow.h"
#include "routegraphapplication.h"

int main(int argc, char *argv[])
{
    RoutegraphApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
