#include "jetdrive.h"

#include <QSerialPort>
#include <QDebug>

namespace JetDrive
{

Controller::Controller(const QString &portName, QObject *parent) :
    AsyncSerialDevice(portName, parent),
    cmdBuilder (std::make_unique<CommandBuilder>())
{
    name = "JetDrive";
    // connect timer for handling timeout errors
    connect(serialPort, &QSerialPort::readyRead,
            this, &Controller::handle_ready_read);

    connect(timer, &QTimer::timeout,
            this, &Controller::handle_timeout);

    connect(serialPort, &QSerialPort::errorOccurred,
            this, &Controller::handle_serial_error);
}

Controller::~Controller()
{
    if (is_connected()) serialPort->close();
}

void Controller::handle_ready_read()
{
    readData.append(serialPort->readAll());

    switch (initState)
    {

    case INIT_Q:
        if (readData.contains(">") || readData.contains("MFJET32"))
        {
            //emit response("MFJET32");
            readData.clear();
            initState = INIT_X2000;
            write_next();
        }
        break;

    case INIT_X2000:
        if (readData.contains(xCmd.at(iX200)))
        {
            //emit response(QString::fromUtf8(&xCmd.constData()[iX200], 1));
            readData.clear();
            iX200++;

            // finish once all of the bytes have been written and read
            if (iX200 >= xCmd.size())
            {
                emit response(QString("Connected to %1").arg(name));
                initState = INITIALIZED;
                iX200 = 0;
            }
            write_next();
        } else readData.clear();
        break;

    case INITIALIZED:
        // the command is in the second byte received
        if (readData.size() == 2)
            expectedResponseSize = response_size((CMD)(readData.at(1)));
        else if (readData.size() >= expectedResponseSize)
        {
            // TODO: can do something with the response here (error checking)
            // qDebug() << readData;
            readData.clear();
            write_next();
        }

        break;

    default: break;
    }
}

void Controller::handle_timeout()
{
    timer->stop();
    emit error(QString("Serial IO Timeout: No response from %1").arg(name));
    emit error(readData);
    disconnect_serial();

}

void Controller::initialize_jet_drive()
{
    // Start-up for MicroJet III.
    initState = InitState::INIT_Q;

    write("Q");
    // write the X2000 command byte by byte
    for (const auto byte : xCmd)
        write(QByteArray(1, byte));

    // initialize jetDrive parameters (order specified in command reference)
    write(cmdBuilder->build(CMD::RESET, jetParams));        // 1.) Soft Reset
    write(cmdBuilder->build(CMD::GETVERSION, jetParams));   // 2.) Version
                                                            // 3.) Get Number of Channels
    write(cmdBuilder->build(CMD::PULSE, jetParams));        // 4.) Pulse Waveform
    write(cmdBuilder->build(CMD::CONTMODE, jetParams));     // 5.) Trigger Mode
    write(cmdBuilder->build(CMD::DROPS, jetParams));        // 6.) Number of Drops per Trigger
    write(cmdBuilder->build(CMD::FULLFREQ, jetParams));     // 7.) Frequency
    write(cmdBuilder->build(CMD::STROBEDIV, jetParams));    // 8.) Strobe Divider
    write(cmdBuilder->build(CMD::STROBEENABLE, jetParams)); // 9.) Strobe Enable
    write(cmdBuilder->build(CMD::STROBEDELAY, jetParams));  // 10.) Strobe Delay
    write(cmdBuilder->build(CMD::SOURCE, jetParams));       // 11.) Trigger Source
}

int Controller::connect_to_jet_drive()
{
    if (is_connected()) // return if already connected
    {
        emit response(QString("Already connected to %1").arg(name));
        return 0;
    }

    clear_members();

    // attempt to open connection to serial port
    if (!serialPort->open(QIODevice::ReadWrite))
    {
        emit error(QString("Can't open %1 on %2, error code %3")
                   .arg(name, serialPort->portName(), QVariant::fromValue(serialPort->error()).toString()));
        return -1;
    }

    // wait after connection to initialize
    emit response(QString("Connecting to %1").arg(name));
    QTimer::singleShot(500, this, &Controller::initialize_jet_drive);
    return 0;
}

void Controller::disconnect_serial()
{
    clear_members();
    if (is_connected())
    {
        emit response(QString("Disconnecting %1").arg(name));
        serialPort->close();
    }
    else emit response(QString("%1 is already disconnected").arg(name));
}

void Controller::clear_members()
{
    iX200 = 0;
    initState = InitState::NOT_INITIALIZED;
    clear_command_queue();
}

void Controller::set_waveform(const Waveform &waveform)
{
    QMutexLocker lock(&mutex);
    jetParams.waveform = waveform;
    write(cmdBuilder->build(CMD::PULSE, jetParams));
}

void Controller::set_continuous_jetting()
{
    QMutexLocker lock(&mutex);
    if (jetParams.fMode != 1)
    {
        jetParams.fMode = 1;
        write(cmdBuilder->build(CMD::CONTMODE, jetParams));
    }
}

void Controller::set_single_jetting()
{
    QMutexLocker lock(&mutex);
    if (jetParams.fMode != 0)
    {
        jetParams.fMode = 0;
        write(cmdBuilder->build(CMD::CONTMODE, jetParams));
    }
}

void Controller::set_continuous_mode_frequency(long frequency_Hz)
{
    QMutexLocker lock(&mutex);
    if (jetParams.fFrequency != frequency_Hz)
    {
        jetParams.fFrequency = frequency_Hz;
        write(cmdBuilder->build(CMD::FULLFREQ, jetParams));
    }
}

void Controller::set_num_drops_per_trigger(short numDrops)
{
    QMutexLocker lock(&mutex);
    if (jetParams.fDrops != numDrops)
    {
        jetParams.fDrops = numDrops;
        write(cmdBuilder->build(CMD::DROPS, jetParams));
    }
}

void Controller::set_external_trigger()
{
    QMutexLocker lock(&mutex);
    if (jetParams.fSource != 1)
    {
        jetParams.fSource = 1;
        write(cmdBuilder->build(CMD::SOURCE, jetParams));
    }
}

void Controller::set_internal_trigger()
{
    QMutexLocker lock(&mutex);
    if (jetParams.fSource != 0)
    {
        jetParams.fSource = 0;
        write(cmdBuilder->build(CMD::SOURCE, jetParams));
    }
}

void Controller::start_continuous_jetting()
{
    set_continuous_jetting();
    set_internal_trigger();
    QMutexLocker lock(&mutex);
    write(cmdBuilder->build(CMD::SOFTTRIGGER, jetParams));
}

void Controller::stop_continuous_jetting()
{
    set_single_jetting();
}

void Controller::enable_strobe()
{
    QMutexLocker lock(&mutex);
    if (jetParams.fStrobeEnable != 1)
    {
        jetParams.fStrobeEnable = 1;
        write(cmdBuilder->build(CMD::STROBEENABLE, jetParams));
    }
}

void Controller::disable_strobe()
{
    QMutexLocker lock(&mutex);
    if (jetParams.fStrobeEnable != 0)
    {
        jetParams.fStrobeEnable = 0;
        write(cmdBuilder->build(CMD::STROBEENABLE, jetParams));
    }
}

void Controller::set_strobe_delay(short strobeDelay_microseconds)
{
    if (jetParams.fStrobeDelay != strobeDelay_microseconds)
    {
        jetParams.fStrobeDelay = strobeDelay_microseconds;
        write(cmdBuilder->build(CMD::STROBEDELAY, jetParams));
    }
}

const Settings Controller::get_jetting_parameters() const
{
    QMutexLocker lock(&mutex);
    return jetParams;
}

const Waveform Controller::get_jetting_waveform() const
{
    QMutexLocker lock(&mutex);
    return jetParams.waveform;
}

void Controller::handle_serial_error(
        QSerialPort::SerialPortError serialPortError)
{
    if (serialPortError == QSerialPort::ReadError)
    {
        emit error(QObject::tr("Read error on port %1, error: %2")
                            .arg(serialPort->portName(),
                                 serialPort->errorString()));
        disconnect_serial();
    }

    if (serialPortError == QSerialPort::OpenError)
    {
        emit error(QObject::tr("Error opening port %1, error: %2")
                            .arg(serialPort->portName(),
                                 serialPort->errorString()));
        disconnect_serial();
    }

    if (serialPortError == QSerialPort::WriteError)
    {
        emit error(QObject::tr("Error writing to port %1, error: %2")
                            .arg(serialPort->portName(),
                                 serialPort->errorString()));
        disconnect_serial();
    }
}

}

# include "moc_jetdrive.cpp"
