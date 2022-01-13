#include "printhread.h"
#include "printer.h"

PrintThread::PrintThread(QObject *parent) : QThread(parent)
{

}

PrintThread::~PrintThread()
{
    mMutex.lock();
    mQuit = true;
    mCond.wakeOne();
    mMutex.unlock();
    wait();
}

void PrintThread::setup(Printer *printer)
{
    mPrinter = printer;
}

void PrintThread::stop()
{
    mMutex.lock();
    mRunning = false;
    mMutex.unlock();
}

void PrintThread::execute_command(std::stringstream &ss)
{
    const QMutexLocker locker(&mMutex);
    if(mQueue.size() != 0) // if the queue is not empty
    {
        emit error("command queue was not empty when new commands were attempted");
    }
    else // start or wake the thread
    {
        // Auto execute code here
        // emit response("Testing...\n");

        std::string buffer;
        while(ss >> buffer)
        {
            //buffer.erase(std::remove(buffer.begin(), buffer.end(), '\n'), buffer.end());
            mQueue.push(buffer);
        }

        if (!isRunning())
        {
            start(); // start a new thread if one has not been created before
        }
        else
        {
            mCond.wakeOne();   // else wake the thread
        }
    }
}

void PrintThread::clear_queue()
{
    mMutex.lock();
    while(mQueue.size() > 0)
    {
        mQueue.pop();
    }
    mMutex.unlock();
}

void PrintThread::run()
{
    while (!mQuit) {

        while(mQueue.size() > 0)
        {
            if(!mRunning) // If the queue is externally stopped
            {
                clear_queue();
                // Code to run on stop
                emit response("Print Stopped\n");
            }
            else
            {
                // === Code to run on each queue item ===
                emit response(QString::fromStdString(mQueue.front()));
                msleep(1500);

                mQueue.pop(); // remove command from the queue
                if(mQueue.size() == 0)
                {
                    // code to run when the queue completes normally
                    emit response("Finished Queue\n");
                }
            }
        }

        mMutex.lock();
        mCond.wait(&mMutex); // wait until thread is woken again by transaction call
        // Once the thread is woken again
        mRunning = true;
        mMutex.unlock();
    }
}
