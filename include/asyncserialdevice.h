#ifndef ASYNCSERIALDEVICE_H
#define ASYNCSERIALDEVICE_H

#include <QObject>
#include <QQueue>
#include <QTimer>
#include <QSerialPort>

class AsyncSerialDevice : public QObject
{
    Q_OBJECT
public:
    explicit AsyncSerialDevice(const QString& portName, QObject *parent = nullptr);
    bool is_connected() const; // returns whether the device is connected or not
    void set_port_name(const QString &portName); // sets the port number for the device

signals:
    void response(const QString &s); // emit info to be printed to console window
    void error(const QString &s); // error messages
    void timeout(const QString &s); // timeout errors

protected:
    void write(const QByteArray &data); // add command to queue for writing
    void write_next(); // send next command in queue
    void clear_command_queue(); // clear the queue

protected:
    QByteArray prevWrite; // stores the last command sent
    QSerialPort *serialPort {nullptr}; // handle for the serial port
    QTimer *timer {nullptr}; // timer for managing timeout
    QByteArray readData; // data of response from device
    QString name {"Serial Device"}; // name of the device

private:
    QQueue<QByteArray> writeQueue; // queue of commands to write to the serial device
    bool isWriteReady {true}; // keeps track of if the device is ready to receive another command
    const int readTimeout_ms {3000}; // timeout time in milliseconds

};

#endif // ASYNCSERIALDEVICE_H
