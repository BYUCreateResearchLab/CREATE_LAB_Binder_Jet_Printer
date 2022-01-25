#include "outputwindow.h"
#include "ui_outputwindow.h"

void OutputWindow::print_string(QString s)
{
    ui->mOutputText->appendPlainText(s);
}

OutputWindow::OutputWindow(QWidget *parent) : QWidget(parent), ui(new Ui::OutputWindow)
{
    ui->setupUi(this);
    ui->mOutputText->setReadOnly(true);
}

OutputWindow::~OutputWindow()
{
    delete ui;
}
