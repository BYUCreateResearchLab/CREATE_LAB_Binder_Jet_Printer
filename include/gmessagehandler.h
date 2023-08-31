#pragma once

#include <QThread>
#include <QMutex>
#include <QWaitCondition>

#include "gclib.h"
#include "gclibo.h"

#include "string"
#include "string_view"

class GMessageHandler : public QThread
{
    Q_OBJECT

public:
    explicit GMessageHandler(QObject *parent = nullptr);
    ~GMessageHandler();

    void connect_to_controller(std::string_view IPAddress);
    void stop();

protected:
    void run() override;

signals:
    void error();

protected:
    GCon g_;
    QMutex mutex_;
    QWaitCondition waitCondition_;
    bool quit_ {false};
    unsigned long sleepTime_ms_ {1};
};
