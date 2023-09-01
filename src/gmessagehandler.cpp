#include "gmessagehandler.h"

#include "mister.h"
#include <QDebug>

GMessageHandler::GMessageHandler(Printer* printer, QObject *parent) :
    QObject(parent),
    printer_(printer)
{

}

void GMessageHandler::handle_message(QString message)
{
    // TODO: get rid of magic strings
    if (message == "CMD MIST_ON")
    {
        printer_->mister->turn_on_misters();
    }
    else if (message == "CMD MIST_OFF")
    {
        printer_->mister->turn_off_misters();
    }

    // TODO: I need a way to respond back to the motion controller
    // perhaps a signal back to the poller thread?
    // or maybe I just respond back through the main g handle?
}

#include "moc_gmessagehandler.cpp"
