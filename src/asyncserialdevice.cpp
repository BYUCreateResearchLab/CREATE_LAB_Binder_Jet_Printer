#include "asyncserialdevice.h"
#include <QDebug>

AsyncSerialDevice::AsyncSerialDevice(const QString& portName, QObject *parent) :
    QObject(parent),
    serialPort (new QSerialPort(this)),
    timer (new QTimer(this))
{
    serialPort->setPortName(portName);
    timer->setSingleShot(true);
}

bool AsyncSerialDevice::is_connected() const
{
    return serialPort->isOpen();
}

void AsyncSerialDevice::set_port_name(const QString &portName)
{
    serialPort->setPortName(portName);
}

void AsyncSerialDevice::write(const QByteArray &data)
{
    if (!serialPort->isOpen())
    {
        emit error(QString("Can't send command. %1 is not connected").arg(name));
        return;
    }
    writeQueue.enqueue(data);
    if (isWriteReady) write_next();
}

void AsyncSerialDevice::write_next()
{
    if (!writeQueue.isEmpty())
    {
        isWriteReady = false;
        // dequeue, store, and write the next message
        prevWrite = writeQueue.dequeue();
        serialPort->write(prevWrite);
        // start timeout timer
        // (expect a response from the device before the timer ends)
        timer->start(readTimeout_ms);
    }

    else // writing complete
    {
        isWriteReady = true;
        timer->stop();
    }
}

void AsyncSerialDevice::clear_command_queue()
{
    writeQueue.clear();
    isWriteReady = true;
}


#include "moc_asyncserialdevice.cpp"
