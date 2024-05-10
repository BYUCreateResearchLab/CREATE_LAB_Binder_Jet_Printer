#include "mjprintheadwidget.h"
#include "ui_mjprintheadwidget.h"

#include "mjdriver.h"

#include <QLineEdit>

MJPrintheadWidget::MJPrintheadWidget(Printer *printer, QWidget *parent) :
    PrinterWidget(printer, parent),
    ui(new Ui::MJPrintheadWidget)
{
    ui->setupUi(this);
    setAccessibleName("Multi-Jet Printhead Widget");

    connect(ui->connectButton, &QPushButton::clicked, this, &MJPrintheadWidget::connect_to_printhead);
    connect(ui->clearButton, &QPushButton::clicked, this, &MJPrintheadWidget::clear_response_text);
    connect(ui->inputLineEdit, &QLineEdit::returnPressed, this, &MJPrintheadWidget::command_entered);
    connect(ui->setFreqSpinBox, &QSpinBox::editingFinished, this, &MJPrintheadWidget::frequencyChanged);
    connect(ui->requestPHStatusPushButton, &QPushButton::clicked, this, [this]{mPrinter->mjController->request_status_of_all_heads();});
    connect(ui->powerToggleButton, &QPushButton::clicked, this, &MJPrintheadWidget::powerTogglePressed);
    connect(ui->getStatusButton, &QPushButton::clicked, this, [this]{mPrinter->mjController->report_status();});
    connect(ui->getPositionButton, &QPushButton::clicked, this, &MJPrintheadWidget::getPositionPressed);

    connect(mPrinter->mjController, &AsyncSerialDevice::response, this, &MJPrintheadWidget::write_to_response_window);
}

MJPrintheadWidget::~MJPrintheadWidget()
{
    delete ui;
}

void MJPrintheadWidget::allow_widget_input(bool allowed)
{

}

void MJPrintheadWidget::connect_to_printhead()
{
    mPrinter->mjController->connect_board();
}

void MJPrintheadWidget::clear_response_text()
{
    ui->responseTextEdit->clear();
}

void MJPrintheadWidget::command_entered()
{
    QString command = ui->inputLineEdit->text();
    send_command(command);
}

void MJPrintheadWidget::powerTogglePressed()
{
    if (ui->powerToggleButton->isChecked())
        mPrinter->mjController->power_off();
    else
        mPrinter->mjController->power_on();
}

void MJPrintheadWidget::getPositionPressed()
{
    mPrinter->mjController->report_current_position();
}

void MJPrintheadWidget::send_command(const QString &command)
{
//    ui->responseTextEdit->appendPlainText(command);
    ui->responseTextEdit->moveCursor(QTextCursor::End);
    ui->responseTextEdit->insertPlainText(command + "\n");
    ui->responseTextEdit->moveCursor(QTextCursor::End);
    mPrinter->mjController->write_line(command.toUtf8());
}

void MJPrintheadWidget::frequencyChanged()
{
    int freq = ui->setFreqSpinBox->value();
    mPrinter->mjController->set_printing_frequency(freq);
}

void MJPrintheadWidget::write_to_response_window(const QString &text)
{
    ui->responseTextEdit->appendPlainText(text);
//    ui->responseTextEdit->moveCursor(QTextCursor::End);
//    ui->responseTextEdit->insertPlainText(text);
//    ui->responseTextEdit->moveCursor(QTextCursor::End);
}
