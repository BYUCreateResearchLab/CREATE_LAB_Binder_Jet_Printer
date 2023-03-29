#include "datarecordpoller.h"
#include <QDebug>

inline void x_e(GReturn rc)
{
    if (rc != G_NO_ERROR)
        throw rc;
}


DataRecordPoller::DataRecordPoller(QObject *parent): QObject(parent)
{
    timer = new QTimer(this);
    //time = r.dmc4000.sample_number; //pull out the desired value
}

void DataRecordPoller::connect_to_controller(std::string_view IPAddress)
{
    GOpen(IPAddress.data(), &g);
    int original_dr;
    x_e(GCmdI(g, "MG_DR", &original_dr)); //grab the current DR value
    x_e(GRecordRate(g, pollPeriod)); //set up DR to 10Hz
    timer->start(pollPeriod - 10); // gives me problems when it is set as the same as the record rate... :( how to fix?...
    connect(timer, &QTimer::timeout, this, &DataRecordPoller::poll_controller_async);
}

void DataRecordPoller::poll_controller_blocking()
{
    GRecord(g, &r, G_QR); // command and response type
}

void DataRecordPoller::poll_controller_async()
{
    //Read data records asynchronously for a given interval.
    //note -s DR must have been specified in GOpen()
    x_e(GRecord(g, &r, G_DR));
    emit read_done();
    // the gui thread gets bogged down if I handle this signal there...
}

#include "moc_datarecordpoller.cpp"
