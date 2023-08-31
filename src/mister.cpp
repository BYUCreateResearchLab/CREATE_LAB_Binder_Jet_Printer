#include "mister.h"

#include <QSerialPort>
#include <QDebug>

namespace Mister
{

Controller::Controller(const QString &portName, QObject *parent) :
    AsyncSerialDevice(portName, parent)
{
    name = "Mister";
    // connect timer for handling timeout errors
    connect(serialPort, &QSerialPort::readyRead, this, &Controller::handle_ready_read);
    connect(timer, &QTimer::timeout, this, &Controller::handle_timeout);
    connect(serialPort, &QSerialPort::errorOccurred, this, &Controller::handle_serial_error);

    serialPort->setBaudRate(QSerialPort::Baud19200);
}

Controller::~Controller()
{
    if (is_connected()) serialPort->close();
}

void Controller::handle_ready_read()
{
    auto inData = serialPort->readAll();
    readData.append(inData);
    QString responseString = QString(readData).simplified();

    if (!inData.contains("\r")) {return;}
    switch (initState)
    {
    case NOT_INITIALIZED:
        if (responseString == initString)
        {
            readData.clear();
            initState = INITIALIZED;
            emit response(QString("Connected to %1").arg(name));
            write_next();
        }
        else
        {
            emit error(QString("Unexpected response from device."
                               " Expected %1 from device but got %2")
                       .arg(initString)
                       .arg(responseString));
            disconnect_serial();
        }
        break;
    case INITIALIZED:
        // handle response from device here
        // right now the response does nothing,
        // just exists so we know the Arduino got the
        // command and is ready for a new one
        write_next();
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

int Controller::connect_to_misters()
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
    QTimer::singleShot(1500, this, &Controller::initialize_misters);
    return 0;
}

void Controller::disconnect_serial()
{
    clear_members();
    if (is_connected())
    {
        emit response(QString("Disconnecting %1").arg(name));
        serialPort->close();
        timer->stop();
    }
    else emit response(QString("%1 is already disconnected").arg(name));
}

void Controller::send_command(CMD command)
{
    write(QString("%1\r").arg((char)command).toUtf8());
}

void Controller::initialize_misters()
{
    send_command(INIT);
}

void Controller::turn_on_misters()
{
    send_command(MIST_ON);
}

void Controller::turn_off_misters()
{
    send_command(MIST_OFF);
}

void Controller::turn_on_left_mister()
{
    send_command(LEFT_ON);
}

void Controller::turn_on_right_mister()
{
    send_command(RIGHT_ON);
}

void Controller::clear_members()
{
    initState = InitState::NOT_INITIALIZED;
    clear_command_queue();
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

#include "moc_mister.cpp"
