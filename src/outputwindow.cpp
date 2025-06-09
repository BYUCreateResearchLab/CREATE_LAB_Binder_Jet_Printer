#include "outputwindow.h"
#include "ui_outputwindow.h"
#include <QDateTime>

bool printComplete = false;
bool atLocation = false;

void OutputWindow::print_string(QString outS)
{
    // Debug value for mjprinthead encoder ticks
    const QString positionPrefix = "Encoder current count: ";
    // output to window
    if(!outS.startsWith(positionPrefix)){
        ui->mOutputText->appendPlainText(outS);
    }

    // check print completion status
    if(outS.contains(QString("Print Complete"))){
        printComplete = true;
    }

    // check if at location status
    if(outS.contains(QString("Arrived at Location"))){
        atLocation = true;
    }

    // write to log
    auto currentTime = QDateTime::currentDateTime()
            .toString("hh:mm:ss")
            .toStdString();
    m_logFile << outS.toStdString();
    if (outS.trimmed().isEmpty()) m_logFile << "\n";
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
