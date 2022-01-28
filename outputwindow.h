#ifndef OUTPUTWINDOW_H
#define OUTPUTWINDOW_H

#include <QWidget>

namespace Ui {
class OutputWindow;
}

class OutputWindow : public QWidget
{
    Q_OBJECT

public:
    explicit OutputWindow(QWidget *parent = 0);
    ~OutputWindow();
public slots:
    void print_string(QString s);

private slots:
    void clear_text();

private:
    Ui::OutputWindow *ui;
};

#endif // OUTPUTWINDOW_H
