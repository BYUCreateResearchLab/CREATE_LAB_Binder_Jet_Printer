#pragma once

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

protected:
    void run() override;

signals:
    void status(uchar status);

protected:
    GCon g_;
    QMutex mutex_;
    QWaitCondition waitCondition_;
    bool quit_ {false};
};
