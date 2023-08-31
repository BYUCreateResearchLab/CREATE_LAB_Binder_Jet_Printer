#pragma once

#include <QObject>

class GMessageHandler : public QObject
{
    Q_OBJECT
public:
    explicit GMessageHandler(QObject *parent = nullptr);


};
