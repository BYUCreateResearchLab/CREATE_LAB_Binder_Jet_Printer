/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.1.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
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
    QToolButton *yHome;
    QSpacerItem *verticalSpacer;
    QSpacerItem *horizontalSpacer;
    QPushButton *yPositive;
    QPushButton *yNegative;
    QPushButton *xPositive;
    QSpacerItem *horizontalSpacer_2;
    QPushButton *xNegative;
    QToolButton *xHome;
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
    QSpinBox *xVelocity;
    QLabel *VelocityLabel;
    QLabel *X_axis;
    QLabel *label_2;
    QSpinBox *xDistance;
    QSpinBox *yVelocity;
    QLabel *Y_axis;
    QSpinBox *yDistance;
    QLabel *label_3;
    QCheckBox *activateRoller;
    QCheckBox *activateHopper;
    QPushButton *spreadNewLayer;
    QSpinBox *rollerSpeed;
    QSpinBox *numLayers;
    QLabel *label_4;
    QLabel *VelocityLabel_2;
    QToolButton *zHome;
    QPushButton *OpenProgramWindow;
    QPushButton *connect;
    QPushButton *saveDefault;
    QPushButton *revertDefault;
    QMenuBar *menubar;
    QMenu *menuPrinter_Controls;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(864, 569);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        gridLayoutWidget = new QWidget(centralwidget);
        gridLayoutWidget->setObjectName(QString::fromUtf8("gridLayoutWidget"));
        gridLayoutWidget->setGeometry(QRect(0, 170, 295, 151));
        gridLayout = new QGridLayout(gridLayoutWidget);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setContentsMargins(0, 0, 0, 0);
        yHome = new QToolButton(gridLayoutWidget);
        yHome->setObjectName(QString::fromUtf8("yHome"));

        gridLayout->addWidget(yHome, 1, 1, 1, 1, Qt::AlignHCenter);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(verticalSpacer, 2, 1, 1, 1);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer, 0, 0, 1, 1);

        yPositive = new QPushButton(gridLayoutWidget);
        yPositive->setObjectName(QString::fromUtf8("yPositive"));

        gridLayout->addWidget(yPositive, 0, 1, 1, 1);

        yNegative = new QPushButton(gridLayoutWidget);
        yNegative->setObjectName(QString::fromUtf8("yNegative"));

        gridLayout->addWidget(yNegative, 4, 1, 1, 1);

        xPositive = new QPushButton(gridLayoutWidget);
        xPositive->setObjectName(QString::fromUtf8("xPositive"));

        gridLayout->addWidget(xPositive, 2, 2, 1, 1);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer_2, 0, 2, 1, 1);

        xNegative = new QPushButton(gridLayoutWidget);
        xNegative->setObjectName(QString::fromUtf8("xNegative"));

        gridLayout->addWidget(xNegative, 2, 0, 1, 1);

        xHome = new QToolButton(gridLayoutWidget);
        xHome->setObjectName(QString::fromUtf8("xHome"));

        gridLayout->addWidget(xHome, 3, 2, 1, 1, Qt::AlignHCenter);

        label4Fun = new QLabel(centralwidget);
        label4Fun->setObjectName(QString::fromUtf8("label4Fun"));
        label4Fun->setGeometry(QRect(310, 50, 181, 16));
        verticalLayoutWidget = new QWidget(centralwidget);
        verticalLayoutWidget->setObjectName(QString::fromUtf8("verticalLayoutWidget"));
        verticalLayoutWidget->setGeometry(QRect(380, 170, 29, 141));
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
        zStepSize->setValue(15);
        label = new QLabel(centralwidget);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(410, 190, 131, 16));
        BedLabel = new QLabel(centralwidget);
        BedLabel->setObjectName(QString::fromUtf8("BedLabel"));
        BedLabel->setGeometry(QRect(340, 330, 61, 16));
        bedSpinBox = new QSpinBox(centralwidget);
        bedSpinBox->setObjectName(QString::fromUtf8("bedSpinBox"));
        bedSpinBox->setGeometry(QRect(400, 320, 51, 22));
        bedSpinBox->setMinimum(0);
        bedSpinBox->setMaximum(300);
        bedSpinBox->setValue(100);
        xVelocity = new QSpinBox(centralwidget);
        xVelocity->setObjectName(QString::fromUtf8("xVelocity"));
        xVelocity->setGeometry(QRect(80, 350, 42, 22));
        xVelocity->setValue(10);
        VelocityLabel = new QLabel(centralwidget);
        VelocityLabel->setObjectName(QString::fromUtf8("VelocityLabel"));
        VelocityLabel->setGeometry(QRect(80, 330, 111, 16));
        X_axis = new QLabel(centralwidget);
        X_axis->setObjectName(QString::fromUtf8("X_axis"));
        X_axis->setGeometry(QRect(40, 350, 55, 16));
        label_2 = new QLabel(centralwidget);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setGeometry(QRect(190, 330, 91, 16));
        xDistance = new QSpinBox(centralwidget);
        xDistance->setObjectName(QString::fromUtf8("xDistance"));
        xDistance->setGeometry(QRect(190, 350, 42, 22));
        xDistance->setValue(10);
        yVelocity = new QSpinBox(centralwidget);
        yVelocity->setObjectName(QString::fromUtf8("yVelocity"));
        yVelocity->setGeometry(QRect(80, 380, 42, 22));
        yVelocity->setValue(10);
        Y_axis = new QLabel(centralwidget);
        Y_axis->setObjectName(QString::fromUtf8("Y_axis"));
        Y_axis->setGeometry(QRect(40, 380, 55, 16));
        yDistance = new QSpinBox(centralwidget);
        yDistance->setObjectName(QString::fromUtf8("yDistance"));
        yDistance->setGeometry(QRect(190, 380, 42, 22));
        yDistance->setValue(10);
        label_3 = new QLabel(centralwidget);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        label_3->setGeometry(QRect(410, 210, 55, 16));
        activateRoller = new QCheckBox(centralwidget);
        activateRoller->setObjectName(QString::fromUtf8("activateRoller"));
        activateRoller->setGeometry(QRect(530, 170, 121, 22));
        activateHopper = new QCheckBox(centralwidget);
        activateHopper->setObjectName(QString::fromUtf8("activateHopper"));
        activateHopper->setGeometry(QRect(530, 210, 121, 22));
        spreadNewLayer = new QPushButton(centralwidget);
        spreadNewLayer->setObjectName(QString::fromUtf8("spreadNewLayer"));
        spreadNewLayer->setGeometry(QRect(530, 320, 141, 25));
        rollerSpeed = new QSpinBox(centralwidget);
        rollerSpeed->setObjectName(QString::fromUtf8("rollerSpeed"));
        rollerSpeed->setGeometry(QRect(530, 280, 42, 22));
        rollerSpeed->setValue(10);
        numLayers = new QSpinBox(centralwidget);
        numLayers->setObjectName(QString::fromUtf8("numLayers"));
        numLayers->setGeometry(QRect(650, 280, 42, 22));
        numLayers->setValue(10);
        label_4 = new QLabel(centralwidget);
        label_4->setObjectName(QString::fromUtf8("label_4"));
        label_4->setGeometry(QRect(650, 260, 101, 16));
        VelocityLabel_2 = new QLabel(centralwidget);
        VelocityLabel_2->setObjectName(QString::fromUtf8("VelocityLabel_2"));
        VelocityLabel_2->setGeometry(QRect(530, 260, 111, 16));
        zHome = new QToolButton(centralwidget);
        zHome->setObjectName(QString::fromUtf8("zHome"));
        zHome->setGeometry(QRect(360, 350, 61, 23));
        OpenProgramWindow = new QPushButton(centralwidget);
        OpenProgramWindow->setObjectName(QString::fromUtf8("OpenProgramWindow"));
        OpenProgramWindow->setGeometry(QRect(510, 60, 171, 25));
        connect = new QPushButton(centralwidget);
        connect->setObjectName(QString::fromUtf8("connect"));
        connect->setGeometry(QRect(74, 70, 121, 23));
        saveDefault = new QPushButton(centralwidget);
        saveDefault->setObjectName(QString::fromUtf8("saveDefault"));
        saveDefault->setGeometry(QRect(460, 360, 141, 25));
        revertDefault = new QPushButton(centralwidget);
        revertDefault->setObjectName(QString::fromUtf8("revertDefault"));
        revertDefault->setGeometry(QRect(610, 360, 141, 25));
        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 864, 21));
        menuPrinter_Controls = new QMenu(menubar);
        menuPrinter_Controls->setObjectName(QString::fromUtf8("menuPrinter_Controls"));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        MainWindow->setStatusBar(statusbar);

        menubar->addAction(menuPrinter_Controls->menuAction());

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        yHome->setText(QCoreApplication::translate("MainWindow", "Y Home", nullptr));
        yPositive->setText(QCoreApplication::translate("MainWindow", "^", nullptr));
        yNegative->setText(QCoreApplication::translate("MainWindow", "v", nullptr));
        xPositive->setText(QCoreApplication::translate("MainWindow", ">", nullptr));
        xNegative->setText(QCoreApplication::translate("MainWindow", "<", nullptr));
        xHome->setText(QCoreApplication::translate("MainWindow", "X Home", nullptr));
        label4Fun->setText(QCoreApplication::translate("MainWindow", "...", nullptr));
        zMax->setText(QCoreApplication::translate("MainWindow", "\342\244\212", nullptr));
        zUp->setText(QCoreApplication::translate("MainWindow", "^", nullptr));
        zDown->setText(QCoreApplication::translate("MainWindow", "v", nullptr));
        zMin->setText(QCoreApplication::translate("MainWindow", "\342\244\213", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "Z Step Size", nullptr));
        BedLabel->setText(QCoreApplication::translate("MainWindow", "Bed Level:", nullptr));
        VelocityLabel->setText(QCoreApplication::translate("MainWindow", "Velocity (mm/s)", nullptr));
        X_axis->setText(QCoreApplication::translate("MainWindow", "X Axis", nullptr));
        label_2->setText(QCoreApplication::translate("MainWindow", "Distance (mm)", nullptr));
        Y_axis->setText(QCoreApplication::translate("MainWindow", "Y Axis", nullptr));
        label_3->setText(QCoreApplication::translate("MainWindow", "(Microns)", nullptr));
        activateRoller->setText(QCoreApplication::translate("MainWindow", "Activate Roller", nullptr));
        activateHopper->setText(QCoreApplication::translate("MainWindow", "Activate Hopper", nullptr));
        spreadNewLayer->setText(QCoreApplication::translate("MainWindow", "Spread New Layer(s)", nullptr));
        label_4->setText(QCoreApplication::translate("MainWindow", "Number of Layers", nullptr));
        VelocityLabel_2->setText(QCoreApplication::translate("MainWindow", "Roller Speed (RPM)", nullptr));
        zHome->setText(QCoreApplication::translate("MainWindow", "Z Home", nullptr));
        OpenProgramWindow->setText(QCoreApplication::translate("MainWindow", "OPEN PROGRAM WINDOW", nullptr));
        connect->setText(QCoreApplication::translate("MainWindow", "Connect to Controller", nullptr));
        saveDefault->setText(QCoreApplication::translate("MainWindow", "SAVE values as default", nullptr));
        revertDefault->setText(QCoreApplication::translate("MainWindow", "REVERT values to default", nullptr));
        menuPrinter_Controls->setTitle(QCoreApplication::translate("MainWindow", "Printer Controls", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
