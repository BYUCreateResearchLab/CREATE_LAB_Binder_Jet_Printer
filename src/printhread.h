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

class PrintThread : public QThread
{
    Q_OBJECT

public:
    explicit PrintThread(QObject *parent = nullptr);
    ~PrintThread();
    void setup(Printer *printer);

public slots:
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
    Printer *mPrinter{nullptr};
    std::queue<std::string> mQueue;
    QMutex mMutex;
    QWaitCondition mCond;
    bool mQuit{false};
    bool mRunning{true};

    bool mPrintGCmds{false};
};

#endif // PRINTHREAD_H
