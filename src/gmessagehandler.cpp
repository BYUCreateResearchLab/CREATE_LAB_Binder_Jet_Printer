#include "gmessagehandler.h"

#include "mister.h"

GMessageHandler::GMessageHandler(QObject *parent) :
    QObject(parent)
{

}

void GMessageHandler::handle_message(QString message)
{
    if (message == "MIST_ON")
    {
        //printer->mister->turn_on_misters();
    }
    else if (message == "MIST_OFF")
    {
        //printer->mister->turn_off_misters();
    }

    // TODO: I need a way to respond back to the motion controller
    // perhaps a signal back to the poller thread?
    // or maybe I just respond back through the main g handle?
}

#include "moc_gmessagehandler.cpp"
