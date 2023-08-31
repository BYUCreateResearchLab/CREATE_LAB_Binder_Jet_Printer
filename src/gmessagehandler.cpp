#include "gmessagehandler.h"

#include "gclib_errors.h"

#include <QDebug>

GMessageHandler::GMessageHandler(QObject *parent):
    QThread(parent)
{

}

GMessageHandler::~GMessageHandler()
{  
    qDebug() << "ending handler";
    stop();
    wait();
    qDebug() << "destroyed";
}

void GMessageHandler::connect_to_controller(std::string_view IPAddress)
{
    quit_ = false;

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

    start();
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

    qDebug() << "start message handler";

    char buf[1024]; //read buffer

    while (true)
    {
        mutex_.lock();
        if (quit_)
        {
            mutex_.unlock();
            break;
        }
        mutex_.unlock();

        if ((rc = GMessage(g_, buf, sizeof(buf))) == G_GCLIB_NON_BLOCKING_READ_EMPTY)
        {
            QThread::msleep(sleepTime_ms_);
            // could also use usleep
            continue;
        }

        if (rc != G_NO_ERROR)
        {
            emit error();
            qDebug() << "GMessage read error";
        }

        // handle the message here
        qDebug() << QByteArray(buf, 5) << '\n';
    }

    qDebug() << "Quit";
    GClose(g_);
    g_ = 0;
}

#include "moc_gmessagehandler.cpp"
