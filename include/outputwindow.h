#ifndef OUTPUTWINDOW_H
#define OUTPUTWINDOW_H

#include <QWidget>
#include <fstream>

namespace Ui {
class OutputWindow;
}

class OutputWindow : public QWidget
{
    Q_OBJECT

public:
    explicit OutputWindow(QWidget *parent, std::ofstream& logFile);
    ~OutputWindow();

signals:
    void windowText(QString text);

public slots:
    void print_string(QString s);
    QString getOutputText();


private slots:
    void clear_text();

private:
    Ui::OutputWindow *ui;
    std::ofstream& m_logFile;
};

#endif // OUTPUTWINDOW_H
