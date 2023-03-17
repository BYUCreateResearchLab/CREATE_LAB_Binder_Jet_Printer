#include "jetdrive.h"

#include <QSerialPort>
#include <QDebug>

namespace JetDrive
{

Controller::Controller(QObject *parent) :
    AsyncSerialDevice(parent),
    commandBuilder (std::make_unique<CommandBuilder>())
{
    // connect timer for handling timeout errors
    connect(serialPort, &QSerialPort::readyRead, this, &Controller::handle_ready_read);
    connect(timer, &QTimer::timeout, this, &Controller::handle_timeout);
    connect(serialPort, &QSerialPort::errorOccurred, this, &Controller::handle_serial_error);
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
                emit response("Connected to JetDrive");
                initState = INITIALIZED;
                iX200 = 0;
            }
            write_next();
        } else readData.clear();
        break;

    case INITIALIZED:
        // the command is in the second byte received
        if (readData.size() == 2)
            expectedResponseSize = response_size(static_cast<CMD>(readData.at(1)));
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
    emit error("Serial IO Timeout: No response from JetDrive");
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
    write(commandBuilder->build_command(CMD::RESET, jetParams));        // 1.) Soft Reset
    write(commandBuilder->build_command(CMD::GETVERSION, jetParams));   // 2.) Version
                                                                        // 3.) Get Number of Channels
    write(commandBuilder->build_command(CMD::PULSE, jetParams));        // 4.) Pulse Waveform
    write(commandBuilder->build_command(CMD::CONTMODE, jetParams));     // 5.) Trigger Mode
    write(commandBuilder->build_command(CMD::DROPS, jetParams));        // 6.) Number of Drops per Trigger
    write(commandBuilder->build_command(CMD::FULLFREQ, jetParams));     // 7.) Frequency
    write(commandBuilder->build_command(CMD::STROBEDIV, jetParams));    // 8.) Strobe Divider
    write(commandBuilder->build_command(CMD::STROBEENABLE, jetParams)); // 9.) Strobe Enable
    write(commandBuilder->build_command(CMD::STROBEDELAY, jetParams));  // 10.) Strobe Delay
    write(commandBuilder->build_command(CMD::SOURCE, jetParams));       // 11.) Trigger Source

}

int Controller::connect_to_jet_drive(const QString &portName)
{
    if (is_connected()) {emit response("Already connected to JetDrive"); return 0;} // return if already connected

    clear_members();

    serialPort->setPortName(portName);

    // I think it will automatically set these settings for me
    // but just to be safe :)
    /*
    serialPort->setBaudRate(QSerialPort::Baud9600);
    serialPort->setDataBits(QSerialPort::Data8);
    serialPort->setParity(QSerialPort::NoParity);
    serialPort->setStopBits(QSerialPort::OneStop);
    serialPort->setFlowControl(QSerialPort::NoFlowControl);
    */

    // attempt to open connection to serial port
    if (!serialPort->open(QIODevice::ReadWrite))
    {
        emit error(QString("Can't open %1, error code %2")
                   .arg(portName).arg(serialPort->error()));
        return -1;
    }

    // wait after connection to initialize
    emit response("Connecting to JetDrive");
    QTimer::singleShot(500, this, &Controller::initialize_jet_drive);
    return 0;
}

void Controller::disconnect_serial()
{
    clear_members();
    if (is_connected())
    {
        emit response("Disconnecting JetDrive");
        serialPort->close();
    }
    else emit response("JetDrive is already disconnected");
}

void Controller::clear_members()
{
    iX200 = 0;
    initState = InitState::NOT_INITIALIZED;
    clear_command_queue();
    //disconnect(serialPort, &QSerialPort::readyRead,
    //           this, &Controller::handle_ready_read);
}

void Controller::set_waveform(const Waveform &waveform)
{
    QMutexLocker lock(&mutex);
    jetParams.waveform = waveform;
    write(commandBuilder->build_command(CMD::PULSE, jetParams));
}

void Controller::set_continuous_jetting()
{
    QMutexLocker lock(&mutex);
    if (jetParams.fMode != 1)
    {
        jetParams.fMode = 1;
        write(commandBuilder->build_command(CMD::CONTMODE, jetParams));
    }
}

void Controller::set_single_jetting()
{
    QMutexLocker lock(&mutex);
    if (jetParams.fMode != 0)
    {
        jetParams.fMode = 0;
        write(commandBuilder->build_command(CMD::CONTMODE, jetParams));
    }
}

void Controller::set_continuous_mode_frequency(long frequency_Hz)
{
    QMutexLocker lock(&mutex);
    if (jetParams.fFrequency != frequency_Hz)
    {
        jetParams.fFrequency = frequency_Hz;
        write(commandBuilder->build_command(CMD::FULLFREQ, jetParams));
    }
}

void Controller::set_num_drops_per_trigger(short numDrops)
{
    QMutexLocker lock(&mutex);
    if (jetParams.fDrops != numDrops)
    {
        jetParams.fDrops = numDrops;
        write(commandBuilder->build_command(CMD::DROPS, jetParams));
    }
}

void Controller::set_external_trigger()
{
    QMutexLocker lock(&mutex);
    if (jetParams.fSource != 1)
    {
        jetParams.fSource = 1;
        write(commandBuilder->build_command(CMD::CONTMODE, jetParams));
    }
}

void Controller::set_internal_trigger()
{
    QMutexLocker lock(&mutex);
    if (jetParams.fSource != 0)
    {
        jetParams.fSource = 0;
        write(commandBuilder->build_command(CMD::CONTMODE, jetParams));
    }
}

void Controller::start_continuous_jetting()
{
    set_continuous_jetting();
    set_internal_trigger();
    QMutexLocker lock(&mutex);
    write(commandBuilder->build_command(CMD::SOFTTRIGGER, jetParams));
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
        write(commandBuilder->build_command(CMD::STROBEENABLE, jetParams));
    }
}

void Controller::disable_strobe()
{
    QMutexLocker lock(&mutex);
    if (jetParams.fStrobeEnable != 0)
    {
        jetParams.fStrobeEnable = 0;
        write(commandBuilder->build_command(CMD::STROBEENABLE, jetParams));
    }
}

void Controller::set_strobe_delay(short strobeDelay_microseconds)
{
    if (jetParams.fStrobeDelay != strobeDelay_microseconds)
    {
        jetParams.fStrobeDelay = strobeDelay_microseconds;
        write(commandBuilder->build_command(CMD::STROBEDELAY, jetParams));
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

void Controller::handle_serial_error(QSerialPort::SerialPortError serialPortError)
{
    if (serialPortError == QSerialPort::ReadError)
    {
        emit error(QObject::tr("Read error on port %1, error: %2")
                            .arg(serialPort->portName(), serialPort->errorString()));
        disconnect_serial();
    }

    if (serialPortError == QSerialPort::OpenError)
    {
        emit error(QObject::tr("Error opening port %1, error: %2")
                            .arg(serialPort->portName(), serialPort->errorString()));
        disconnect_serial();
    }

    if (serialPortError == QSerialPort::WriteError)
    {
        emit error(QObject::tr("Error writing to port %1, error: %2")
                            .arg(serialPort->portName(), serialPort->errorString()));
        disconnect_serial();
    }
}

}

# include "moc_jetdrive.cpp"
