/********************************************************************************
** Form generated from reading UI file 'outputwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.1.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_OUTPUTWINDOW_H
#define UI_OUTPUTWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_OutputWindow
{
public:
    QGridLayout *gridLayout;
    QPlainTextEdit *mOutputText;

    void setupUi(QWidget *OutputWindow)
    {
        if (OutputWindow->objectName().isEmpty())
            OutputWindow->setObjectName(QString::fromUtf8("OutputWindow"));
        OutputWindow->resize(400, 300);
        gridLayout = new QGridLayout(OutputWindow);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        mOutputText = new QPlainTextEdit(OutputWindow);
        mOutputText->setObjectName(QString::fromUtf8("mOutputText"));
        mOutputText->setUndoRedoEnabled(false);
        mOutputText->setReadOnly(true);

        gridLayout->addWidget(mOutputText, 0, 0, 1, 1);


        retranslateUi(OutputWindow);

        QMetaObject::connectSlotsByName(OutputWindow);
    } // setupUi

    void retranslateUi(QWidget *OutputWindow)
    {
        OutputWindow->setWindowTitle(QCoreApplication::translate("OutputWindow", "Form", nullptr));
    } // retranslateUi

};

namespace Ui {
    class OutputWindow: public Ui_OutputWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_OUTPUTWINDOW_H
