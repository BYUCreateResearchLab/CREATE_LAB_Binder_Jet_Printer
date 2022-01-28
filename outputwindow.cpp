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
    connect(ui->clearText, &QPushButton::clicked, this, &OutputWindow::clear_text);
}

OutputWindow::~OutputWindow()
{
    delete ui;
}

void OutputWindow::clear_text()
{
    ui->mOutputText->clear();
}
