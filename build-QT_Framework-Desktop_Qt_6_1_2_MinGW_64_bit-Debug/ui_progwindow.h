/********************************************************************************
** Form generated from reading UI file 'progwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.1.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PROGWINDOW_H
#define UI_PROGWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_progWindow
{
public:
    QPushButton *back2Home;

    void setupUi(QWidget *progWindow)
    {
        if (progWindow->objectName().isEmpty())
            progWindow->setObjectName(QString::fromUtf8("progWindow"));
        progWindow->resize(556, 390);
        back2Home = new QPushButton(progWindow);
        back2Home->setObjectName(QString::fromUtf8("back2Home"));
        back2Home->setGeometry(QRect(220, 170, 101, 25));

        retranslateUi(progWindow);

        QMetaObject::connectSlotsByName(progWindow);
    } // setupUi

    void retranslateUi(QWidget *progWindow)
    {
        progWindow->setWindowTitle(QCoreApplication::translate("progWindow", "Form", nullptr));
        back2Home->setText(QCoreApplication::translate("progWindow", "Back to Main", nullptr));
    } // retranslateUi

};

namespace Ui {
    class progWindow: public Ui_progWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PROGWINDOW_H
