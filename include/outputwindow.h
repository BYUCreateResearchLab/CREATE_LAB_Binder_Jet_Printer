#ifndef OUTPUTWINDOW_H
#define OUTPUTWINDOW_H

#include <QWidget>
#include <fstream>

extern bool printComplete;

namespace Ui {
class OutputWindow;
}

class OutputWindow : public QWidget
{
    Q_OBJECT

public:
    explicit OutputWindow(QWidget *parent, std::ofstream& logFile);
    ~OutputWindow();
public slots:
    void print_string(QString s);

private slots:
    void clear_text();

private:
    Ui::OutputWindow *ui;
    std::ofstream& m_logFile;
};

#endif // OUTPUTWINDOW_H
