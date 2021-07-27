#include "programwindow.h"
#include "ui_programWindow.h"

programWindow::programWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::programWindow)
{
    ui->setupUi(this);
}

programWindow::~programWindow()
{
    delete ui;
}
