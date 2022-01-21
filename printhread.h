#ifndef PRINTHREAD_H
#define PRINTHREAD_H

#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include <queue>
#include <sstream>
#include <string>

class Printer;

class PrintThread : public QThread
{
    Q_OBJECT

public:
    explicit PrintThread(QObject *parent = nullptr);
    ~PrintThread();
    void setup(Printer *printer);

    void execute_command(std::stringstream &ss);
    void stop();

private:
    void run() override;
    void clear_queue();

signals:
    void response(QString s);
    void error(const std::string &text);
    void ended();

private:
    Printer *mPrinter{nullptr};
    std::queue<std::string> mQueue;
    QMutex mMutex;
    QWaitCondition mCond;
    bool mQuit{false};
    bool mRunning{true};
};

#endif // PRINTHREAD_H
