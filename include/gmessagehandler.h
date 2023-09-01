#pragma once

#include "printer.h"

class GMessageHandler : public QObject
{
    Q_OBJECT
public:
    explicit GMessageHandler(QObject *parent = nullptr);

public slots:
    void handle_message(QString message);

protected:
    Printer *printer {nullptr};
};
