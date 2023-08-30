#ifndef MISTER_H
#define MISTER_H

#include <QMutex>
#include "asyncserialdevice.h"

namespace Mister
{

// class for controlling serial communications with
// the Arduino misters
class Controller final : public AsyncSerialDevice
{
    Q_OBJECT

public:

    enum InitState
    {
        NOT_INITIALIZED,
        INITIALIZED
    };
    Q_ENUM(InitState)

    enum CMD
    {
        INIT     = 'Q', // initialize serial device
        MIST_ON  = 'O', // Turn on misters
        MIST_OFF = 'C', // Turn off misters
        LEFT_ON  = 'L', // purge off command
        RIGHT_ON = 'R', // Use soft serial
    };

    explicit Controller(const QString &portName, QObject *parent = nullptr);
    ~Controller();
    int connect_to_misters();
    void disconnect_serial();

private:
    void initialize_misters();

    void turn_on_misters();
    void turn_off_misters();
    void turn_on_left_mister();
    void turn_on_right_mister();

    // consider making these virtual functions in the asyncserialdevice ??
    void handle_ready_read();
    void handle_timeout();

    void clear_members();
    void handle_serial_error(QSerialPort::SerialPortError serialPortError);

private:
    mutable QMutex mutex;

    // Members to help with initialization
    InitState initState {NOT_INITIALIZED};
    const QString initString = "Mister";
};

}

#endif // Mister_H
