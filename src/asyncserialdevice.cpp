#include "asyncserialdevice.h"
#include <QDebug>
#include <QThread>

AsyncSerialDevice::AsyncSerialDevice(const QString& portName, QObject *parent) :
    QObject(parent),
    serialPort (new QSerialPort(this)),
    timer (new QTimer(this)),
    delayTimer(new QTimer(this))
{
    serialPort->setPortName(portName);
    timer->setSingleShot(true);
    delayTimer->setSingleShot(true);

    // --- PLACE THE CONNECT HERE ---
    connect(delayTimer, &QTimer::timeout, this, [this]() {
        if (serialPort && serialPort->isOpen()) {
            serialPort->write(prevWrite);
            timer->start(readTimeout_ms);
        }
        write_next(); // Proceed to next queued item
    });
    // ------------------------------
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
        prevWrite = writeQueue.dequeue();
        delayTimer->start(50); // This will trigger the handler configured in the constructor
    }
    else
    {
        isWriteReady = true;
        timer->stop();
    }
}

void AsyncSerialDevice::directWrite(const QByteArray &data){
    if(serialPort && serialPort->isOpen()){
        serialPort->write(data);
        serialPort->flush();
    }
}

void AsyncSerialDevice::clear_command_queue()
{
    writeQueue.clear();
    isWriteReady = true;
}


#include "moc_asyncserialdevice.cpp"
