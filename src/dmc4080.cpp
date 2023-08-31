#include "dmc4080.h"

#include "gclib.h"
#include "gclibo.h"
#include "gclib_errors.h"
#include "gclib_record.h"

#include "printhread.h"
#include "ginterrupthandler.h"

#include "printer.h"

#include <QDebug>

DMC4080::DMC4080(std::string_view address_, QObject *parent) :
    QObject(parent),
    address ( address_.data() ),
    printerThread ( new PrintThread(this) ),
    //interruptHandler ( new GInterruptHandler(this) ),
    messageHandler ( new GMessageHandler(this) )
{
    printerThread->setup(this);
}

DMC4080::~DMC4080()
{
    disconnect_controller();
}

void DMC4080::connect_to_motion_controller(bool homeZAxis)
{
    std::stringstream s;

    s << CMD::open_connection_to_controller();
    s << CMD::set_default_controller_settings();
    s << CMD::homing_sequence(homeZAxis);

    printerThread->execute_command(s);

    // subscribe to messages
    messageHandler->connect_to_controller(address);
}

void DMC4080::disconnect_controller()
{
    qDebug() << "disconnecting";
    //interruptHandler->stop();
    messageHandler->stop();
    // this needs to go first
    printerThread->stop();

    // TODO: don't write the raw commands directly, make API
    if (g) // double check there is actually a connection
    {
        GCmd(g, "ST"); // stop motion
        GCmd(g, "MO"); // disable Motors
        GCmd(g, "CB 18"); // stop roller 1
        GCmd(g, "CB 21"); // stop roller 2
        GCmd(g, "MG{P2} {^85}, {^48}, {^13}{N}"); // stop hopper
        GClose(g);     // close connection to the motion controller
    }
    g = 0;             // Reset connection handle


    //messageHandler->terminate();

    // wait for threads to quit
    //messageHandler->wait();
    //interruptHandler->wait();
}

#include "moc_dmc4080.cpp"
