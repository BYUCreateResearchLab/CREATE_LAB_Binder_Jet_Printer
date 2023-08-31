#include "gmessagehandler.h"

#include "gclib_errors.h"

#include <QDebug>

GMessageHandler::GMessageHandler(QObject *parent):
    QThread(parent)
{

}

GMessageHandler::~GMessageHandler()
{
    stop();
    wait();
}

void GMessageHandler::connect_to_controller(std::string_view IPAddress)
{
    std::string stringIn = IPAddress.data();
    stringIn += " --subscribe MG";

    // don't do anything if it can't connect
    if (GOpen(stringIn.c_str(), &g_) != G_NO_ERROR)
    {
        qDebug() << "Could not connect to controller for messages";
        return;
    }

    // could also put up in the GOpen function but it cant connect to the controller very fast (crashses with low timeout)
    //GTimeout(g_, 250);
    GTimeout(g_, 0); // set timeout to 0 for non-blocking read

    mutex_.lock();
    waitCondition_.wakeOne();
    mutex_.unlock();
}

void GMessageHandler::stop()
{
    mutex_.lock();
    quit_ = true;
    mutex_.unlock();
}

void GMessageHandler::run()
{
    GReturn rc;
    char buf[1024]; //read buffer
    mutex_.lock();
    // wait until the motion controller is connected
    waitCondition_.wait(&mutex_);
    // Once the thread is woken
    while (true)
    {
        //QMutexLocker(&mutex_);
        mutex_.lock();
        if (quit_) break;
        mutex_.unlock();

        if (rc = GMessage(g_, buf, sizeof(buf)) == G_GCLIB_NON_BLOCKING_READ_EMPTY)
        {
            QThread::msleep(sleepTime_ms_);
            continue;
        }

        if (rc != G_NO_ERROR)
        {
            emit error();
            qDebug() << "GMessage read error";
        }

        // handle the message here
        qDebug() << QByteArray(buf, sizeof(buf)) << '\n';
    }
    mutex_.unlock();

    qDebug() << "Quit";
}

#include "moc_gmessagehandler.cpp"
