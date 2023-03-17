#ifndef DATARECORDPOLLER_H
#define DATARECORDPOLLER_H

#include "gclib.h"
#include "gclibo.h"
#include "string"
#include "string_view"

#include <QTimer>

class DataRecordPoller : public QObject
{
    Q_OBJECT
public:
    DataRecordPoller(QObject *parent = nullptr);

    void connect_to_controller(std::string_view IPAddress);
    void poll_controller_blocking();
    void poll_controller_async();

    GDataRecord r; //user's data record union.

signals:
    void read_done();

private:
    GCon g;

    QTimer *timer {nullptr};
    int pollPeriod = {100};
};

#endif // DATARECORDPOLLER_H
