#ifndef PROGWINDOW_H
#define PROGWINDOW_H

#include <QWidget>

namespace Ui {
class progWindow;
}

class progWindow : public QWidget
{
    Q_OBJECT

public:
    explicit progWindow(QWidget *parent = nullptr);
    ~progWindow();

signals:
    void firstWindow();

private slots:
    void on_back2Home_clicked();

private:
    Ui::progWindow *ui;
};

#endif // PROGWINDOW_H
