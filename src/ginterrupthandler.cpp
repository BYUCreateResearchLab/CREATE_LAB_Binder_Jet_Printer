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
    std::string stringIn = IPAddress.data();
    stringIn += " --subscribe EI";
    GOpen(stringIn.c_str(), &g);

    // send EI command (subscribe to all axes complete interrupt)
    // work to be able to build up this number from documentation on EI command
    GCmd(g, "EI 8447");
    // individual axis interrupts and all axes complete interrupts seem to be exclusive.
    // the controller will prefer to send a single axis complete interrupt instead of all axes interrupt

    // could also put up in the GOpen function but it cant connect to the controller very fast (crashses with low timeout)
    GTimeout(g, 250);

    mutex.lock();
    waitCondition.wakeOne();
    mutex.unlock();
}

void GInterruptHandler::stop()
{
    mutex.lock();
    mQuit = true;
    mutex.unlock();
}

void GInterruptHandler::run()
{
    GStatus stat;
    mutex.lock();
    // wait until the motion controller is connected
    qDebug() << "Wait";
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

    qDebug() << "Quit";
}

#include "moc_ginterrupthandler.cpp"
