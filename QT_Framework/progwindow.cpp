#include "progwindow.h"
#include "ui_progwindow.h"

progWindow::progWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::progWindow)
{
    ui->setupUi(this);
}

progWindow::~progWindow()
{
    delete ui;
}

void progWindow::on_back2Home_clicked()
{
    this->close();
    emit firstWindow();
}

