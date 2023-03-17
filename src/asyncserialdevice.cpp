#include "asyncserialdevice.h"
#include <QDebug>

AsyncSerialDevice::AsyncSerialDevice(QObject *parent) : QObject(parent),
    serialPort (new QSerialPort(this)),
    timer (new QTimer(this))
{
    timer->setSingleShot(true);
}

bool AsyncSerialDevice::is_connected() const
{
    return serialPort->isOpen();
}

void AsyncSerialDevice::write(const QByteArray &data)
{
    if (!serialPort->isOpen())
    {
        emit error("Can't send command. JetDrive is not connected");
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
        // start timeout timer (expect a response from the device before the timer ends)
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
