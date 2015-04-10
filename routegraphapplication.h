#ifndef ROUTEGRAPHAPPLICATION_H
#define ROUTEGRAPHAPPLICATION_H

#include <QApplication>

class RoutegraphApplication : public QApplication
{
    Q_OBJECT
public:
    explicit RoutegraphApplication(int &argc, char** &argv);
    bool notify(QObject *receiver, QEvent *event);

signals:

public slots:

};

#endif // ROUTEGRAPHAPPLICATION_H
