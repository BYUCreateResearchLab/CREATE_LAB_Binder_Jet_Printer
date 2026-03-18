#include "dmc4080.h"

#include "gclib.h"
#include "gclibo.h"
#include "gclib_errors.h"
#include "gclib_record.h"

#include "printhread.h"

#include "printer.h"

#include <QDebug>

DMC4080::DMC4080(std::string_view address_, QObject *parent) :
    QObject(parent),
    address ( address_.data() ),
    printerThread ( new PrintThread(this) ),
    //interruptHandler ( new GInterruptHandler(this) ),
    messagePoller ( new GMessagePoller(this) )
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
    printerThread->execute_command(s);
    s = std::stringstream();

    std::string program =
        "REM Y-Axis Commutation Initialization\r"
        "MOY\r"               // Motor Off Y
        "BAY\r"               // Brushless Axis Y
        "BMY=2000\r"          // Brushless Modulo Y
        "BIY=-1\r"            // Brushless Init Y
        "BCY\r"               // Brushless Calibration Y
        "hall=_QHY\r"         // Store Hall State
        "SHY\r"               // Servo Here Y
        "JGY=-1600\r"         // Jog Y Negative (-1600)
        "BGY\r"               // Begin Jog Y
        "#hall\r"             // Label for loop
        "WT2\r"               // Wait 2ms
        "JP#hall,_QHY=hall\r" // Jump back if hall state is same
        "STY\r"               // Stop Y
        "AMY\r"               // After Motion (Wait for stop)
        "MG \"Y-Axis Initialization Complete\"\r" // Prints to Output Window
        "EN\r";               // End Program

    std::time_t currentTime = std::time(nullptr);
    while (!g && std::time(nullptr) < currentTime + 10) {}
    if(g) {
        // 1. Download the program to the controller
        GProgramDownload(g, program.c_str(), "");

        // 2. Execute the program
        s << "GCmd," << "XQ" << "\n";
        s << CMD::sleep(500);
        s << CMD::after_motion(Axis::Y);

        s << CMD::set_default_controller_settings();
        s << CMD::homing_sequence(homeZAxis);

        printerThread->execute_command(s);
    } else {
        qDebug() << "couldn't connect to controller";
    }

    // subscribe to messages
    messagePoller->connect_to_controller(address);
}

void DMC4080::disconnect_controller()
{
    qDebug() << "disconnecting";
    //interruptHandler->stop();
    messagePoller->stop();
    // this needs to go first
    printerThread->stop();

    // TODO: don't write the raw commands directly, make API
    if (g) // double check there is actually a connection
    {
        GCmd(g, "ST"); // stop motion
        GCmd(g, "MO"); // disable Motors
        GCmd(g, "CB 18"); // stop roller 1
        GCmd(g, "CB 21"); // stop roller 2
        GCmd(g, "OFE=0"); // turn off heat lamp
        GCmd(g, "CB 9"); //turn off heat lamp
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
