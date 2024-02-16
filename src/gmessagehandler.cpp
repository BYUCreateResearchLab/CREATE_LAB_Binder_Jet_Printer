#include "gmessagehandler.h"

#include "mister.h"
#include "jetdrive.h"
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
    else if (message.contains("CMD JET_FREQ"))
    {
        int startIndex = message.indexOf("CMD JET_FREQ") + strlen("CMD JET_FREQ") + 1; // +1 to skip the space
        QString valueString = message.mid(startIndex);
        bool conversionOK;
        int freq = valueString.toInt(&conversionOK); // in Hz
        if (conversionOK)
        {
            printer_->jetDrive->set_continuous_mode_frequency(freq);
        }
        else
        {
            qDebug() << "Unable to extract value from received string. CMD JET_FREQ";
        }
    }
    else if (message.contains("CMD JET_NDROPS"))
    {
        int startIndex = message.indexOf("JET_NDROPS") + strlen("JET_NDROPS") + 1; // +1 to skip the space
        QString valueString = message.mid(startIndex);
        bool conversionOK;
        int numDrops = valueString.toInt(&conversionOK);
        if (conversionOK)
        {
            printer_->jetDrive->set_num_drops_per_trigger(numDrops);
        }
        else
        {
            qDebug() << "Unable to extract value from received string. CMD JET_NDROPS";
        }
    }


    // TODO: I need a way to respond back to the motion controller
    // perhaps a signal back to the poller thread?
    // or maybe I just respond back through the main g handle?
}

#include "moc_gmessagehandler.cpp"
