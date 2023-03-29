#include "ginterrupthandler.h"

#include <QDebug>

GInterruptHandler::GInterruptHandler(QObject *parent): QThread(parent)
{

}

GInterruptHandler::~GInterruptHandler()
{
    stop();
    wait();
}

void GInterruptHandler::connect_to_controller(std::string_view IPAddress)
{
    QString stringIn  = QString("%s --subscribe EI").arg(IPAddress.data());
    GOpen(stringIn.toStdString().c_str(), &g);

    // send EI command (subscribe to all axes complete interrupt)
    GCmd(g, "EI 256");

    QMutexLocker locker(&mutex);
    waitCondition.wakeOne();
}

void GInterruptHandler::stop()
{
    mutex.lock();
    mQuit = false;
    mutex.unlock();
}

void GInterruptHandler::run()
{
    GStatus stat;
    mutex.lock();
    // wait until the motion controller is connected
    waitCondition.wait(&mutex);
    // Once the thread is woken
    while (!mQuit)
    {
        mutex.unlock();
        if (GInterrupt(g, &stat) == G_NO_ERROR)
        {
            emit status(stat);
        }
        mutex.lock();
    }
    mutex.unlock();
}

#include "moc_ginterrupthandler.cpp"
