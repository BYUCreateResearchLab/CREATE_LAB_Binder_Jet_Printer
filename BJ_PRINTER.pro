QT       += core gui svg widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
greaterThan(QT_MAJOR_VERSION, 5): QT += svgwidgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

HEADERS += \
        JetServer.h \
        gclib.h \
        gclibo.h \
        gclib_errors.h \
        gclib_record.h \
    jettingwidget.h \
    lineprintwidget.h \
        mainwindow.h \
        outputwindow.h \
        powdersetupwidget.h \
        printer.h \
    printerwidget.h \
        lineprintdata.h \
        printobject.h \
        svgview.h \
        vec2.h \
        printhread.h \

SOURCES += \
        gclibo.c \
        arrays.c \
    jettingwidget.cpp \
    lineprintwidget.cpp \
        main.cpp \
        mainwindow.cpp \
        outputwindow.cpp \
        powdersetupwidget.cpp \
        printer.cpp \
    printerwidget.cpp \
        lineprintdata.cpp \
        printobject.cpp \
        svgview.cpp \
        printhread.cpp \

FORMS += \
    jettingwidget.ui \
    lineprintwidget.ui \
        mainwindow.ui \
        outputwindow.ui \
        powdersetupwidget.ui \

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

INCLUDEPATH += $$PWD/.
DEPENDPATH += $$PWD/.

win32: LIBS += -L /usr/linclude -lz # fixes references to 'uncompress' in zlib.h
win32: LIBS += -L$$PWD/ -lgclib
win32: LIBS += -L$$PWD/ -lgclibo

DISTFILES += \
    PrinterSettings.txt
