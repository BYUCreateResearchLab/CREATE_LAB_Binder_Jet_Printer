#ifndef POWDERSETUPWIDGET_H
#define POWDERSETUPWIDGET_H

#include <QWidget>
#include <sstream>

namespace Ui {
class PowderSetupWidget;
}

class PowderSetupWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PowderSetupWidget(QWidget *parent = nullptr);
    ~PowderSetupWidget();

public slots:
    void allow_user_input(bool allowed);

signals:
    void execute_command(std::stringstream &s);
    void generate_printing_message_box(const std::string &message);

private slots:
    void level_recoat_clicked();
    void normal_recoat_clicked();

private:
    Ui::PowderSetupWidget *ui;
};

#endif // POWDERSETUPWIDGET_H
