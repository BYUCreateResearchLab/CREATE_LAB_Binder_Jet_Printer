#include "dmc4080.h"

#include "gclib.h"
#include "gclibo.h"
#include "gclib_errors.h"
#include "gclib_record.h"

#include "printhread.h"
#include "ginterrupthandler.h"

DMC4080::DMC4080(std::string_view address_, QObject *parent) :
    QObject(parent),
    address ( address_.data() ),
    printerThread ( new PrintThread(this) ),
    interruptHandler ( new GInterruptHandler(this) )
{
    printerThread->setup(this);
}

DMC4080::~DMC4080()
{
    if (g) // if there is an active connection to a controller
    {
        GCmd(g, "ST");
        GCmd(g, "MO");    // Turn off the motors
        GCmd(g, "CB 18"); // stop roller 1
        GCmd(g, "CB 21"); // stop roller 2
        GCmd(g, "MG{P2} {^85}, {^48}, {^13}{N}"); // stop hopper
        GClose(g); // Close the connection to the controller
    }

    // I need to actually handle this
    if (interruptHandler) interruptHandler->stop();
}

#include "moc_dmc4080.cpp"
