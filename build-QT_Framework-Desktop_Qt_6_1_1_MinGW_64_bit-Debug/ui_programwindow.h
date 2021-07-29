/********************************************************************************
** Form generated from reading UI file 'programwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.1.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PROGRAMWINDOW_H
#define UI_PROGRAMWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QPushButton>

QT_BEGIN_NAMESPACE

class Ui_programWindow
{
public:
    QPushButton *backToMain_Button;

    void setupUi(QDialog *programWindow)
    {
        if (programWindow->objectName().isEmpty())
            programWindow->setObjectName(QString::fromUtf8("programWindow"));
        programWindow->resize(556, 395);
        backToMain_Button = new QPushButton(programWindow);
        backToMain_Button->setObjectName(QString::fromUtf8("backToMain_Button"));
        backToMain_Button->setGeometry(QRect(230, 190, 101, 25));

        retranslateUi(programWindow);

        QMetaObject::connectSlotsByName(programWindow);
    } // setupUi

    void retranslateUi(QDialog *programWindow)
    {
        programWindow->setWindowTitle(QCoreApplication::translate("programWindow", "Dialog", nullptr));
        backToMain_Button->setText(QCoreApplication::translate("programWindow", "Back To Main", nullptr));
    } // retranslateUi

};

namespace Ui {
    class programWindow: public Ui_programWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PROGRAMWINDOW_H
