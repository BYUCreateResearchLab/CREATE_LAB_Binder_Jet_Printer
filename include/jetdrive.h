#ifndef JETDRIVE_H
#define JETDRIVE_H

#include <QMutex>
#include "asyncserialdevice.h"
#include "mfjdrv.h"

namespace JetDrive
{

class Controller final : public AsyncSerialDevice
{
    Q_OBJECT

public:

    enum InitState
    {
        NOT_INITIALIZED,
        INIT_Q,
        INIT_X2000,
        INITIALIZED
    };
    Q_ENUM(InitState)

    explicit Controller(QObject *parent = nullptr);
    ~Controller();
    int connect_to_jet_drive(const QString &portName);
    void disconnect_serial();

    const Settings get_jetting_parameters() const;
    const Waveform get_jetting_waveform() const;

    void set_waveform(const Waveform &waveform);

    //Before sending the next command, the response packet must have been received.
    // TODO: Implement a queue for commands that get popped off and sent when
    // the appropriate response has been received
    // use response_size(CMD) to get the appropriate of bytes to wait for
    // from the response
    void set_continuous_jetting();
    void set_single_jetting();

    void set_continuous_mode_frequency(long frequency_Hz);
    void set_num_drops_per_trigger(short numDrops);

    void set_external_trigger();
    void set_internal_trigger();

    void start_continuous_jetting();
    void stop_continuous_jetting();

    void enable_strobe();
    void disable_strobe();
    void set_strobe_delay(short strobeDelay_microseconds);

private:
    // consider making these virtual functions in the asyncserialdevice ??
    void handle_ready_read();
    void handle_timeout();
    void initialize_jet_drive();

    void clear_members();
    void handle_serial_error(QSerialPort::SerialPortError serialPortError);

private:
    mutable QMutex mutex;
    std::unique_ptr<CommandBuilder> commandBuilder;
    Settings jetParams;

    // Members to help with initialization
    InitState initState {NOT_INITIALIZED};
    const QByteArray xCmd {"X2000"};
    short iX200 = {0};
    int expectedResponseSize {4};
};

}

#endif // JETDRIVE_H
