#ifndef PRINTHREAD_H
#define PRINTHREAD_H

#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include <queue>
#include <sstream>
#include <string>

#include "gclib.h"
#include "gclibo.h"
#include "gclib_errors.h"
#include "gclib_record.h"

class Printer;

// Runs commands for communicating with the Galil Motion Controller on a different thread
// Look at https://www.youtube.com/watch?v=O1a5Z1ZIbSw for more info
// on multithreading in Qt
// Other threads cannot interact with the GUI. All GUI actions must be done
// on the main thread. Use signals out of the thread to connect to slots of
// object connected to the main thread for GUI actions

class PrintThread : public QThread
{
    Q_OBJECT

public:
    explicit PrintThread(QObject *parent = nullptr);
    ~PrintThread();
    void setup(Printer *printer);

// TODO: Need to avoid slots in a subclass of QThread. These are actually
// being run on the main thread as they have affinity there.
// Work on removing slots, or move to a worker object that is moved to QThread
public slots:
    // I'm pretty sure I can just make these public (not slots) as there
    // are no connections.
    void execute_command(std::stringstream &ss);
    void stop();
    void print_gcmds(bool print);

private:
    void run() override;
    void clear_queue();
    GReturn e(GReturn rc);

signals:
    void response(QString s);
    void error(const std::string &text);
    void ended();
    void connected_to_controller();

private:
    Printer *mPrinter {nullptr};
    std::queue<std::string> mQueue;
    QMutex mMutex;
    QWaitCondition mCond;
    bool mQuit {false};
    bool mRunning {true};

    bool mPrintGCmds {false};
};

#endif // PRINTHREAD_H
