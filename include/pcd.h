#ifndef PCD_H
#define PCD_H

#include <QMutex>
#include "asyncserialdevice.h"

namespace PCD
{

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
        INIT =      'Q', // initialize controller
        UNIT_ID =   'a', // pressure controller unitID
        PURGE_ON =  'P', // purge command
        PURGE_OFF = 'O', // purge off command
        USE_SS =    'S', // Use soft serial
        USE_ANLG =  'X'  // Use analog
    };

    explicit Controller(const QString &portName, QObject *parent = nullptr);
    ~Controller();
    int connect_to_pressure_controller();
    void disconnect_serial();

    void update_set_point(double setPoint_PSIG);
    void purge();
    void stop_purge();

private:
    void initialize_pressure_controller();

    // consider making these virtual functions in the asyncserialdevice ??
    void handle_ready_read();
    void handle_timeout();

    void clear_members();
    void handle_serial_error(QSerialPort::SerialPortError serialPortError);

private:
    mutable QMutex mutex;

    // Members to help with initialization
    InitState initState {NOT_INITIALIZED};
    const QString initString = "PCD";
};

}

#endif // PCD_H
