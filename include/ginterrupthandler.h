#ifndef GINTERRUPTHANDLER_H
#define GINTERRUPTHANDLER_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>

#include "gclib.h"
#include "gclibo.h"

#include "string"
#include "string_view"

class GInterruptHandler : public QThread
{
    Q_OBJECT

public:
    explicit GInterruptHandler(QObject *parent = nullptr);
    ~GInterruptHandler();

    void connect_to_controller(std::string_view IPAddress);
    void stop();


private:
    void run() override;

signals:
    void status(uchar status);

private:
    GCon g;
    QMutex mutex;
    QWaitCondition waitCondition;
    bool mQuit {false};
};

#endif // GINTERRUPTHANDLER_H
