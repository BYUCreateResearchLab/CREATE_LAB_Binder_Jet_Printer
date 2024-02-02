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

class DMC4080;

// Runs commands for communicating with the Galil Motion Controller on a different thread
// Look at https://www.youtube.com/watch?v=O1a5Z1ZIbSw for more info
// on multithreading in Qt
// Other threads cannot interact with the GUI. All GUI actions must be done
// on the main thread. Use signals out of the thread to connect to slots of
// object connected to the main thread for GUI actions

class PrintThread : public QThread
{
    Q_OBJECT

// There should be no slots in an object inheriting QThread

public:
    explicit PrintThread(QObject *parent = nullptr);
    ~PrintThread();
    void setup(DMC4080 *printer);
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
    DMC4080 *mPrinter {nullptr};
    std::queue<std::string> queue;
    QMutex mutex;
    QWaitCondition waitCondition;
    bool mQuit {false};
    bool running {true};

    bool mPrintGCmds {false};
};

#endif // PRINTHREAD_H
