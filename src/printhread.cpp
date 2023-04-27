#include "printhread.h"

#include "dmc4080.h"
#include "gclib.h"
#include "gclibo.h"
#include "gclib_errors.h"
#include "gclib_record.h"

#include <QDebug>

GReturn GCALL GProgramComplete(GCon g)
{
    char pred[] = "_XQ=-1";
    GReturn rc;

    // poll forever. Change this if a premature exit is desired.
    rc = GWaitForBool(g, pred, -1);
    if (rc != G_NO_ERROR)
        return rc;

    return G_NO_ERROR;
}

PrintThread::PrintThread(QObject *parent) : QThread(parent)
{

}

PrintThread::~PrintThread()
{
    mutex.lock();
    mQuit = true;
    waitCondition.wakeOne();
    mutex.unlock();
    wait();
}

void PrintThread::setup(DMC4080 *printer)
{
    mPrinter = printer;
}

void PrintThread::stop()
{
    mutex.lock();
    running = false;
    mutex.unlock();
}

void PrintThread::print_gcmds(bool print)
{
    mutex.lock();
    mPrintGCmds = print;
    mutex.unlock();
}

void PrintThread::execute_command(std::stringstream &ss)
{
    const QMutexLocker locker(&mutex);
    if (queue.size() != 0) // if the queue is not empty
    {
        emit error("command queue was not empty when new commands were attempted");
        return;
    }

    // start or wake the thread

    std::string buffer;
    //while(ss >> buffer) // spaces split into different objects
    while (std::getline(ss, buffer)) // Reads whole line (includes spaces)
    { queue.push(buffer); }
    if (!isRunning())
    { start(); } // start a new thread if one has not been created before
    else
    { waitCondition.wakeOne(); } // else wake the thread
}

void PrintThread::clear_queue()
{
    mutex.lock();
    while (queue.size() > 0)
    { queue.pop(); }
    mutex.unlock();
}

void PrintThread::run()
{
    while (!mQuit)
    {
        while (queue.size() > 0)
        {
            if (!running) // If the queue is externally stopped
            {
                clear_queue();
                // Code to run on stop
                emit response("Stream to Motion Controller Stopped");
                if (mPrinter->g)
                {
                    GCmd(mPrinter->g, "ST"); // stop motors
                    emit response("GCmd: ST");
                }
            }
            else
            {
                // === Code to run on each queue item ===

                // split string into command type and command string
                std::string commandString = queue.front();
                std::string delimeterChar = ",";
                size_t pos{0};
                std::string commandType;
                pos = commandString.find(delimeterChar);

                if (pos != std::string::npos)
                {
                    commandType = commandString.substr(0, pos);
                    commandString.erase(0, pos + delimeterChar.length());
                }
                else
                {
                    commandType = commandString;
                    commandString = "";
                }


                if (commandType == "GCmd")
                {
                    if (mPrintGCmds) emit response(QString::fromStdString(commandString));
                    if (mPrinter->g)
                    {
                        if (e(GCmd(mPrinter->g, commandString.c_str())) == G_BAD_RESPONSE_QUESTION_MARK)
                        {
                            emit response(QString("Above is the error for: ") + QString::fromStdString(commandString));
                        }
                    }
                    else
                    {
                        //emit response("ERROR: not connected to controller!");
                    }
                }
                else if (commandType == "PrintLineSet")
                {
                    if (mPrintGCmds) emit response(QString::fromStdString(commandString));
                    if (mPrinter->g)
                    {
                        // wait until the first value in the Data Array is 0
                        int val{};
                        constexpr int sleepTime_ms = 100;
                        // safety to let the program break out of the loop eventually
                        constexpr int breakLoopTime_sec = 500; // breaks out in just under 8 minutes
                        constexpr int maxLoop{(breakLoopTime_sec * 1000) / sleepTime_ms};
                        int counter{0};
                        do {
                            GCmdI(mPrinter->g, "Data[0]=?", &val);
                            GSleep(sleepTime_ms);
                            counter++;
                        }
                        while (val != 0 && counter < maxLoop && running);

                        if (running) //download full array
                            e(GArrayDownload(mPrinter->g, "Data", G_BOUNDS, G_BOUNDS, commandString.c_str()));
                    }
                    else
                    {
                        //emit response("ERROR: not connected to controller!");
                    }
                }
                else if (commandType == "GMotionComplete")
                {
                    //emit response(QString::fromStdString(commandType) + QString::fromStdString(": ") + QString::fromStdString(commandString));
                    if (mPrinter->g)
                    {
                        e(GMotionComplete(mPrinter->g, commandString.c_str()));
                    }
                    else
                    {
                        //emit response("ERROR: not connected to controller!");
                    }
                }
                else if (commandType == "GSleep")
                {
                    //emit response(QString::fromStdString(commandType) + QString::fromStdString(": ") + QString::fromStdString(commandString));
                    if (mPrinter->g)
                    {
                        GSleep(std::stoi(commandString));
                    }
                    else
                    {
                        //emit response("ERROR: not connected to controller!");
                    }
                }
                else if (commandType == "JetDrive")
                {

                }
                else if (commandType == "GCmdInt")
                {
                    // I NEED TO PLAN OUT BETTER WHAT I WANT TO DO HERE...
                    // maybe doing it through the thread is not the best option...
                }
                else if (commandType == "GProgramComplete")
                {
                    if (mPrinter->g)
                    {
                        e(GProgramComplete(mPrinter->g));
                    }
                    else
                    {
                        //emit response("ERROR: not connected to controller!");
                    }
                }
                else if (commandType == "GOpen")
                {
                    emit response(QString::fromStdString("Attempting to connect to ") + QString::fromStdString(mPrinter->address));
                    if (e(GOpen(mPrinter->address, &mPrinter->g)) != G_NO_ERROR)
                    {
                        emit response("Could not connect to motion controller!");
                        stop();
                    }
                    else
                    {
                        emit response("Connected to motion controller");
                        emit connected_to_controller();
                    }
                }
                else if (commandType == "Message")
                {
                    emit response(QString::fromStdString(commandString));
                }
                else
                {
                    emit response(QString::fromStdString("Command Not Found!: \"") + QString::fromStdString(commandType) + QString::fromStdString("\"\nStopping Print..."));
                    stop();
                }



                //msleep(150);
                queue.pop(); // remove command from the queue
                if(queue.size() == 0)
                {
                    // code to run when the queue completes normally
                    if (mPrintGCmds)
                    {
                        emit response("Finished Queue\n");
                    }
                }



            }
        }

        emit ended();
        mutex.lock();
        // wait until thread is woken again by transaction call
        waitCondition.wait(&mutex);
        // Once the thread is woken again
        running = true;
        mutex.unlock();
    }
}

GReturn PrintThread::e(GReturn rc)
{
    char buf[G_SMALL_BUFFER];
    GError(rc, buf, G_SMALL_BUFFER); // Get Error Information
    if (mPrinter->g)
    {
        GSize size = sizeof(buf);
        GUtility(mPrinter->g, G_UTIL_ERROR_CONTEXT, buf, &size);
        if(buf[0])
        {
            emit response(buf);
        }

        if ((rc == G_BAD_RESPONSE_QUESTION_MARK) && (GCmdT(mPrinter->g, "TC1", buf, G_SMALL_BUFFER, 0) == G_NO_ERROR))
        {
            //std::cout << buf << '\n'; // Error code from controller
            emit response(buf);
        }
    }
    return rc;
}

#include "moc_printhread.cpp"
