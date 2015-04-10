#include "routegraphapplication.h"

#include <QDebug>

RoutegraphApplication::RoutegraphApplication(int &argc, char **&argv) :
    QApplication(argc, argv)
{
}

bool RoutegraphApplication::notify(QObject *receiver, QEvent *event)
{
    try {
        return QApplication::notify(receiver, event);
    } catch (std::exception& e) {
        qDebug() << e.what();
        return false;
    }
}
