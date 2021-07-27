#ifndef PROGRAMWINDOW_H
#define PROGRAMWINDOW_H

#include <QDialog>

namespace Ui {
class programWindow;
}

class programWindow : public QDialog
{
    Q_OBJECT

public:
    explicit programWindow(QWidget *parent = nullptr);
    ~programWindow();

private:
    Ui::programWindow *ui;
};

#endif // PROGRAMWINDOW_H
