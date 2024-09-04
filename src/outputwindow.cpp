#include "outputwindow.h"
#include "ui_outputwindow.h"
#include <QDateTime>
//#include "globals.h"

bool printComplete = false;

void OutputWindow::print_string(QString s)
{
    // output to window
    ui->mOutputText->appendPlainText(s);

    // check print completion status
    if(s.contains(QString("Print Complete"))){
        printComplete = true;
        ui->mOutputText->appendPlainText(QString("!!!!!!!!!"));
    }
    else{
        printComplete = false;
    }

    // write to log
    auto currentTime = QDateTime::currentDateTime()
            .toString("hh:mm:ss")
            .toStdString();
    m_logFile << s.toStdString();
    if (s.trimmed().isEmpty()) m_logFile << "\n";
    else m_logFile << " | " << currentTime << "\n";
}

OutputWindow::OutputWindow(QWidget *parent, std::ofstream& logFile)
    : QWidget(parent),
      ui(new Ui::OutputWindow),
      m_logFile(logFile)
{
    ui->setupUi(this);
    ui->mOutputText->setReadOnly(true);
    connect(ui->clearText,
            &QPushButton::clicked,
            this,
            &OutputWindow::clear_text);
}

OutputWindow::~OutputWindow()
{
    delete ui;
}

void OutputWindow::clear_text()
{
    ui->mOutputText->clear();
}

#include "moc_outputwindow.cpp"
