/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.1.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QWidget *gridLayoutWidget;
    QGridLayout *gridLayout;
    QSpacerItem *verticalSpacer;
    QPushButton *xPositive;
    QSpacerItem *horizontalSpacer_2;
    QPushButton *yNegative;
    QSpacerItem *horizontalSpacer;
    QPushButton *xNegative;
    QPushButton *yPositive;
    QToolButton *xHome;
    QToolButton *yHome;
    QLabel *label4Fun;
    QWidget *verticalLayoutWidget;
    QVBoxLayout *verticalLayout;
    QToolButton *zMax;
    QToolButton *zUp;
    QToolButton *zDown;
    QToolButton *zMin;
    QSpinBox *zStepSize;
    QLabel *label;
    QLabel *BedLabel;
    QSpinBox *bedSpinBox;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(800, 600);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        gridLayoutWidget = new QWidget(centralwidget);
        gridLayoutWidget->setObjectName(QString::fromUtf8("gridLayoutWidget"));
        gridLayoutWidget->setGeometry(QRect(0, 170, 295, 141));
        gridLayout = new QGridLayout(gridLayoutWidget);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setContentsMargins(0, 0, 0, 0);
        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(verticalSpacer, 1, 1, 1, 1);

        xPositive = new QPushButton(gridLayoutWidget);
        xPositive->setObjectName(QString::fromUtf8("xPositive"));

        gridLayout->addWidget(xPositive, 1, 2, 1, 1);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer_2, 0, 2, 1, 1);

        yNegative = new QPushButton(gridLayoutWidget);
        yNegative->setObjectName(QString::fromUtf8("yNegative"));

        gridLayout->addWidget(yNegative, 2, 1, 1, 1);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer, 0, 0, 1, 1);

        xNegative = new QPushButton(gridLayoutWidget);
        xNegative->setObjectName(QString::fromUtf8("xNegative"));

        gridLayout->addWidget(xNegative, 1, 0, 1, 1);

        yPositive = new QPushButton(gridLayoutWidget);
        yPositive->setObjectName(QString::fromUtf8("yPositive"));

        gridLayout->addWidget(yPositive, 0, 1, 1, 1);

        xHome = new QToolButton(centralwidget);
        xHome->setObjectName(QString::fromUtf8("xHome"));
        xHome->setGeometry(QRect(300, 230, 51, 22));
        yHome = new QToolButton(centralwidget);
        yHome->setObjectName(QString::fromUtf8("yHome"));
        yHome->setGeometry(QRect(120, 140, 61, 22));
        label4Fun = new QLabel(centralwidget);
        label4Fun->setObjectName(QString::fromUtf8("label4Fun"));
        label4Fun->setGeometry(QRect(310, 50, 181, 16));
        verticalLayoutWidget = new QWidget(centralwidget);
        verticalLayoutWidget->setObjectName(QString::fromUtf8("verticalLayoutWidget"));
        verticalLayoutWidget->setGeometry(QRect(380, 170, 21, 141));
        verticalLayout = new QVBoxLayout(verticalLayoutWidget);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        zMax = new QToolButton(verticalLayoutWidget);
        zMax->setObjectName(QString::fromUtf8("zMax"));

        verticalLayout->addWidget(zMax);

        zUp = new QToolButton(verticalLayoutWidget);
        zUp->setObjectName(QString::fromUtf8("zUp"));

        verticalLayout->addWidget(zUp);

        zDown = new QToolButton(verticalLayoutWidget);
        zDown->setObjectName(QString::fromUtf8("zDown"));

        verticalLayout->addWidget(zDown);

        zMin = new QToolButton(verticalLayoutWidget);
        zMin->setObjectName(QString::fromUtf8("zMin"));

        verticalLayout->addWidget(zMin);

        zStepSize = new QSpinBox(centralwidget);
        zStepSize->setObjectName(QString::fromUtf8("zStepSize"));
        zStepSize->setGeometry(QRect(410, 230, 42, 22));
        label = new QLabel(centralwidget);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(410, 210, 111, 16));
        BedLabel = new QLabel(centralwidget);
        BedLabel->setObjectName(QString::fromUtf8("BedLabel"));
        BedLabel->setGeometry(QRect(340, 330, 61, 16));
        bedSpinBox = new QSpinBox(centralwidget);
        bedSpinBox->setObjectName(QString::fromUtf8("bedSpinBox"));
        bedSpinBox->setGeometry(QRect(400, 320, 42, 22));
        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 800, 26));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        xPositive->setText(QCoreApplication::translate("MainWindow", ">", nullptr));
        yNegative->setText(QCoreApplication::translate("MainWindow", "v", nullptr));
        xNegative->setText(QCoreApplication::translate("MainWindow", "<", nullptr));
        yPositive->setText(QCoreApplication::translate("MainWindow", "^", nullptr));
        xHome->setText(QCoreApplication::translate("MainWindow", "X Home", nullptr));
        yHome->setText(QCoreApplication::translate("MainWindow", "Y Home", nullptr));
        label4Fun->setText(QCoreApplication::translate("MainWindow", "...", nullptr));
        zMax->setText(QCoreApplication::translate("MainWindow", "\342\244\212", nullptr));
        zUp->setText(QCoreApplication::translate("MainWindow", "^", nullptr));
        zDown->setText(QCoreApplication::translate("MainWindow", "v", nullptr));
        zMin->setText(QCoreApplication::translate("MainWindow", "\342\244\213", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "Step Size (Microns)", nullptr));
        BedLabel->setText(QCoreApplication::translate("MainWindow", "Bed Level:", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
