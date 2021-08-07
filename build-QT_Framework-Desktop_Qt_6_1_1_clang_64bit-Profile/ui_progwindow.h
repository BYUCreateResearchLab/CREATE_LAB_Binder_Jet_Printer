/********************************************************************************
** Form generated from reading UI file 'progwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.1.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PROGWINDOW_H
#define UI_PROGWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "svgview.h"

QT_BEGIN_NAMESPACE

class Ui_progWindow
{
public:
    QVBoxLayout *verticalLayout_3;
    QHBoxLayout *horizontalLayout_7;
    QLabel *label_numSets;
    QSpinBox *numSets;
    QLabel *label_StartXY;
    QDoubleSpinBox *startX;
    QDoubleSpinBox *startY;
    QLabel *label_setSpacing;
    QDoubleSpinBox *setSpacing;
    QTableWidget *tableWidget;
    QHBoxLayout *horizontalLayout_5;
    QTextEdit *consoleOutput;
    QVBoxLayout *verticalLayout_2;
    SvgView *SVGViewer;
    QHBoxLayout *horizontalLayout_6;
    QCheckBox *viewArrows;
    QCheckBox *viewMovePaths;
    QSlider *printPercentSlider;
    QHBoxLayout *horizontalLayout_8;
    QPushButton *back2Home;
    QPushButton *clearConsole;
    QPushButton *saveConfig;
    QPushButton *startPrint;

    void setupUi(QWidget *progWindow)
    {
        if (progWindow->objectName().isEmpty())
            progWindow->setObjectName(QString::fromUtf8("progWindow"));
        progWindow->resize(789, 731);
        verticalLayout_3 = new QVBoxLayout(progWindow);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        horizontalLayout_7 = new QHBoxLayout();
        horizontalLayout_7->setObjectName(QString::fromUtf8("horizontalLayout_7"));
        label_numSets = new QLabel(progWindow);
        label_numSets->setObjectName(QString::fromUtf8("label_numSets"));
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(label_numSets->sizePolicy().hasHeightForWidth());
        label_numSets->setSizePolicy(sizePolicy);
        label_numSets->setMinimumSize(QSize(112, 0));
        label_numSets->setMaximumSize(QSize(112, 16777215));

        horizontalLayout_7->addWidget(label_numSets);

        numSets = new QSpinBox(progWindow);
        numSets->setObjectName(QString::fromUtf8("numSets"));
        numSets->setMaximumSize(QSize(100, 16777215));
        numSets->setMinimum(1);
        numSets->setMaximum(10);
        numSets->setValue(1);

        horizontalLayout_7->addWidget(numSets);

        label_StartXY = new QLabel(progWindow);
        label_StartXY->setObjectName(QString::fromUtf8("label_StartXY"));

        horizontalLayout_7->addWidget(label_StartXY, 0, Qt::AlignRight);

        startX = new QDoubleSpinBox(progWindow);
        startX->setObjectName(QString::fromUtf8("startX"));
        startX->setMaximumSize(QSize(100, 16777215));
        startX->setValue(10.000000000000000);

        horizontalLayout_7->addWidget(startX);

        startY = new QDoubleSpinBox(progWindow);
        startY->setObjectName(QString::fromUtf8("startY"));
        startY->setMaximumSize(QSize(100, 16777215));
        startY->setValue(10.000000000000000);

        horizontalLayout_7->addWidget(startY);

        label_setSpacing = new QLabel(progWindow);
        label_setSpacing->setObjectName(QString::fromUtf8("label_setSpacing"));

        horizontalLayout_7->addWidget(label_setSpacing, 0, Qt::AlignRight);

        setSpacing = new QDoubleSpinBox(progWindow);
        setSpacing->setObjectName(QString::fromUtf8("setSpacing"));
        setSpacing->setMaximumSize(QSize(100, 16777215));
        setSpacing->setDecimals(2);
        setSpacing->setMinimum(0.010000000000000);
        setSpacing->setValue(5.000000000000000);

        horizontalLayout_7->addWidget(setSpacing);


        verticalLayout_3->addLayout(horizontalLayout_7);

        tableWidget = new QTableWidget(progWindow);
        if (tableWidget->columnCount() < 6)
            tableWidget->setColumnCount(6);
        QTableWidgetItem *__qtablewidgetitem = new QTableWidgetItem();
        tableWidget->setHorizontalHeaderItem(0, __qtablewidgetitem);
        QTableWidgetItem *__qtablewidgetitem1 = new QTableWidgetItem();
        tableWidget->setHorizontalHeaderItem(1, __qtablewidgetitem1);
        QTableWidgetItem *__qtablewidgetitem2 = new QTableWidgetItem();
        tableWidget->setHorizontalHeaderItem(2, __qtablewidgetitem2);
        QTableWidgetItem *__qtablewidgetitem3 = new QTableWidgetItem();
        tableWidget->setHorizontalHeaderItem(3, __qtablewidgetitem3);
        QTableWidgetItem *__qtablewidgetitem4 = new QTableWidgetItem();
        tableWidget->setHorizontalHeaderItem(4, __qtablewidgetitem4);
        QTableWidgetItem *__qtablewidgetitem5 = new QTableWidgetItem();
        tableWidget->setHorizontalHeaderItem(5, __qtablewidgetitem5);
        if (tableWidget->rowCount() < 3)
            tableWidget->setRowCount(3);
        QTableWidgetItem *__qtablewidgetitem6 = new QTableWidgetItem();
        tableWidget->setVerticalHeaderItem(0, __qtablewidgetitem6);
        QTableWidgetItem *__qtablewidgetitem7 = new QTableWidgetItem();
        tableWidget->setVerticalHeaderItem(1, __qtablewidgetitem7);
        QTableWidgetItem *__qtablewidgetitem8 = new QTableWidgetItem();
        tableWidget->setVerticalHeaderItem(2, __qtablewidgetitem8);
        QTableWidgetItem *__qtablewidgetitem9 = new QTableWidgetItem();
        tableWidget->setItem(0, 0, __qtablewidgetitem9);
        QTableWidgetItem *__qtablewidgetitem10 = new QTableWidgetItem();
        tableWidget->setItem(0, 1, __qtablewidgetitem10);
        QTableWidgetItem *__qtablewidgetitem11 = new QTableWidgetItem();
        tableWidget->setItem(0, 2, __qtablewidgetitem11);
        QTableWidgetItem *__qtablewidgetitem12 = new QTableWidgetItem();
        tableWidget->setItem(0, 3, __qtablewidgetitem12);
        QTableWidgetItem *__qtablewidgetitem13 = new QTableWidgetItem();
        tableWidget->setItem(0, 4, __qtablewidgetitem13);
        QTableWidgetItem *__qtablewidgetitem14 = new QTableWidgetItem();
        tableWidget->setItem(0, 5, __qtablewidgetitem14);
        QTableWidgetItem *__qtablewidgetitem15 = new QTableWidgetItem();
        tableWidget->setItem(1, 0, __qtablewidgetitem15);
        tableWidget->setObjectName(QString::fromUtf8("tableWidget"));
        tableWidget->horizontalHeader()->setVisible(false);
        tableWidget->horizontalHeader()->setCascadingSectionResizes(false);
        tableWidget->horizontalHeader()->setDefaultSectionSize(150);
        tableWidget->verticalHeader()->setCascadingSectionResizes(false);

        verticalLayout_3->addWidget(tableWidget);

        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setObjectName(QString::fromUtf8("horizontalLayout_5"));
        consoleOutput = new QTextEdit(progWindow);
        consoleOutput->setObjectName(QString::fromUtf8("consoleOutput"));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Minimum);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(consoleOutput->sizePolicy().hasHeightForWidth());
        consoleOutput->setSizePolicy(sizePolicy1);
        consoleOutput->setSizeAdjustPolicy(QAbstractScrollArea::AdjustIgnored);
        consoleOutput->setReadOnly(true);

        horizontalLayout_5->addWidget(consoleOutput);

        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        SVGViewer = new SvgView(progWindow);
        SVGViewer->setObjectName(QString::fromUtf8("SVGViewer"));
        QSizePolicy sizePolicy2(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(SVGViewer->sizePolicy().hasHeightForWidth());
        SVGViewer->setSizePolicy(sizePolicy2);
        SVGViewer->setMinimumSize(QSize(500, 500));
        SVGViewer->setMaximumSize(QSize(500, 500));
        SVGViewer->setMouseTracking(false);
        SVGViewer->setInteractive(false);
        SVGViewer->setTransformationAnchor(QGraphicsView::AnchorViewCenter);
        SVGViewer->setResizeAnchor(QGraphicsView::AnchorViewCenter);

        verticalLayout_2->addWidget(SVGViewer);

        horizontalLayout_6 = new QHBoxLayout();
        horizontalLayout_6->setObjectName(QString::fromUtf8("horizontalLayout_6"));
        viewArrows = new QCheckBox(progWindow);
        viewArrows->setObjectName(QString::fromUtf8("viewArrows"));

        horizontalLayout_6->addWidget(viewArrows);

        viewMovePaths = new QCheckBox(progWindow);
        viewMovePaths->setObjectName(QString::fromUtf8("viewMovePaths"));

        horizontalLayout_6->addWidget(viewMovePaths);


        verticalLayout_2->addLayout(horizontalLayout_6);


        horizontalLayout_5->addLayout(verticalLayout_2);

        printPercentSlider = new QSlider(progWindow);
        printPercentSlider->setObjectName(QString::fromUtf8("printPercentSlider"));
        printPercentSlider->setMaximum(1000);
        printPercentSlider->setValue(1000);
        printPercentSlider->setSliderPosition(1000);
        printPercentSlider->setOrientation(Qt::Vertical);

        horizontalLayout_5->addWidget(printPercentSlider);


        verticalLayout_3->addLayout(horizontalLayout_5);

        horizontalLayout_8 = new QHBoxLayout();
        horizontalLayout_8->setObjectName(QString::fromUtf8("horizontalLayout_8"));
        back2Home = new QPushButton(progWindow);
        back2Home->setObjectName(QString::fromUtf8("back2Home"));

        horizontalLayout_8->addWidget(back2Home);

        clearConsole = new QPushButton(progWindow);
        clearConsole->setObjectName(QString::fromUtf8("clearConsole"));

        horizontalLayout_8->addWidget(clearConsole);

        saveConfig = new QPushButton(progWindow);
        saveConfig->setObjectName(QString::fromUtf8("saveConfig"));

        horizontalLayout_8->addWidget(saveConfig);

        startPrint = new QPushButton(progWindow);
        startPrint->setObjectName(QString::fromUtf8("startPrint"));

        horizontalLayout_8->addWidget(startPrint);


        verticalLayout_3->addLayout(horizontalLayout_8);


        retranslateUi(progWindow);

        QMetaObject::connectSlotsByName(progWindow);
    } // setupUi

    void retranslateUi(QWidget *progWindow)
    {
        progWindow->setWindowTitle(QCoreApplication::translate("progWindow", "Form", nullptr));
        label_numSets->setText(QCoreApplication::translate("progWindow", "Number of Sets", nullptr));
        label_StartXY->setText(QCoreApplication::translate("progWindow", "Start X, Y (mm)", nullptr));
        label_setSpacing->setText(QCoreApplication::translate("progWindow", "Set Spacing (mm)", nullptr));
        QTableWidgetItem *___qtablewidgetitem = tableWidget->horizontalHeaderItem(0);
        ___qtablewidgetitem->setText(QCoreApplication::translate("progWindow", "Number of Lines", nullptr));
        QTableWidgetItem *___qtablewidgetitem1 = tableWidget->horizontalHeaderItem(1);
        ___qtablewidgetitem1->setText(QCoreApplication::translate("progWindow", "Line Spacing (mm)", nullptr));
        QTableWidgetItem *___qtablewidgetitem2 = tableWidget->horizontalHeaderItem(2);
        ___qtablewidgetitem2->setText(QCoreApplication::translate("progWindow", "Line Length (mm)", nullptr));
        QTableWidgetItem *___qtablewidgetitem3 = tableWidget->horizontalHeaderItem(3);
        ___qtablewidgetitem3->setText(QCoreApplication::translate("progWindow", "Droplet Spacing (um)", nullptr));
        QTableWidgetItem *___qtablewidgetitem4 = tableWidget->horizontalHeaderItem(4);
        ___qtablewidgetitem4->setText(QCoreApplication::translate("progWindow", "Jetting Frequency (Hz)", nullptr));
        QTableWidgetItem *___qtablewidgetitem5 = tableWidget->horizontalHeaderItem(5);
        ___qtablewidgetitem5->setText(QCoreApplication::translate("progWindow", "Printing Velocity (mm/s)", nullptr));
        QTableWidgetItem *___qtablewidgetitem6 = tableWidget->verticalHeaderItem(0);
        ___qtablewidgetitem6->setText(QCoreApplication::translate("progWindow", "Set 1", nullptr));
        QTableWidgetItem *___qtablewidgetitem7 = tableWidget->verticalHeaderItem(1);
        ___qtablewidgetitem7->setText(QCoreApplication::translate("progWindow", "Set 2", nullptr));
        QTableWidgetItem *___qtablewidgetitem8 = tableWidget->verticalHeaderItem(2);
        ___qtablewidgetitem8->setText(QCoreApplication::translate("progWindow", "Set 3", nullptr));

        const bool __sortingEnabled = tableWidget->isSortingEnabled();
        tableWidget->setSortingEnabled(false);
        QTableWidgetItem *___qtablewidgetitem9 = tableWidget->item(0, 0);
        ___qtablewidgetitem9->setText(QCoreApplication::translate("progWindow", "7", nullptr));
        QTableWidgetItem *___qtablewidgetitem10 = tableWidget->item(0, 1);
        ___qtablewidgetitem10->setText(QCoreApplication::translate("progWindow", "10", nullptr));
        QTableWidgetItem *___qtablewidgetitem11 = tableWidget->item(0, 2);
        ___qtablewidgetitem11->setText(QCoreApplication::translate("progWindow", "20", nullptr));
        QTableWidgetItem *___qtablewidgetitem12 = tableWidget->item(0, 3);
        ___qtablewidgetitem12->setText(QCoreApplication::translate("progWindow", "5", nullptr));
        QTableWidgetItem *___qtablewidgetitem13 = tableWidget->item(0, 4);
        ___qtablewidgetitem13->setText(QCoreApplication::translate("progWindow", "1000", nullptr));
        QTableWidgetItem *___qtablewidgetitem14 = tableWidget->item(0, 5);
        ___qtablewidgetitem14->setText(QCoreApplication::translate("progWindow", "5", nullptr));
        tableWidget->setSortingEnabled(__sortingEnabled);

        viewArrows->setText(QCoreApplication::translate("progWindow", "View Arrows", nullptr));
        viewMovePaths->setText(QCoreApplication::translate("progWindow", "View Move Paths", nullptr));
        back2Home->setText(QCoreApplication::translate("progWindow", "Back to Main", nullptr));
        clearConsole->setText(QCoreApplication::translate("progWindow", "Clear Console", nullptr));
        saveConfig->setText(QCoreApplication::translate("progWindow", "Save Configuration", nullptr));
        startPrint->setText(QCoreApplication::translate("progWindow", "Start Print", nullptr));
    } // retranslateUi

};

namespace Ui {
    class progWindow: public Ui_progWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PROGWINDOW_H
