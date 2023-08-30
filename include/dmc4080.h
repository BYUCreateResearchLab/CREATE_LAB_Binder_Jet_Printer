#ifndef DMC4080_H
#define DMC4080_H

#include <QObject>
#include <string_view>

class PrintThread;
class GInterruptHandler;
typedef void* GCon;

class DMC4080 : public QObject
{
    Q_OBJECT
public:
    explicit DMC4080(std::string_view address_, QObject *parent = nullptr);
    ~DMC4080();

public:
    // the computer ethernet port needs to be set to 192.168.42.10
    const char *address; // IP address of motion controller
    PrintThread *printerThread {nullptr};
    GInterruptHandler *interruptHandler {nullptr};

    GCon g {0}; // Handle for connection to Galil Motion Controller

private:

};

#endif // DMC4080_H