QT       += core gui svg svgwidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Comment these out and define AVOIDGCLIB in mainwindow.h to compile on machines that don't play well with gclib
SOURCES += \
    gclibo.c \
    arrays.c \

HEADERS += \
    gclib.h \
    gclibo.h \
    gclib_errors.h
    gclib_record.h



SOURCES += \
    main.cpp \
    mainwindow.cpp \
    progwindow.cpp \
    lineprintdata.cpp \
    printobject.cpp \
    svgview.cpp \
    fakegclib.cpp \

HEADERS += \
    #../../../../../../Program Files (x86)/Galil/gclib/include/gclib.h \
    #../../../../../../Program Files (x86)/Galil/gclib/include/gclib_errors.h \
    #../../../../../../Program Files (x86)/Galil/gclib/include/gclib_record.h \
    #../../../../../../Program Files (x86)/Galil/gclib/include/gclibo.h \
    mainwindow.h \
    progwindow.h \
    lineprintdata.h \
    printobject.h \
    svgview.h \
    vec2.h \
    fakegclib.h

FORMS += \
    mainwindow.ui \
    progwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target




LIBS +=   -L /usr/linclude -lz # fixes references to 'uncompress' in zlib.h

win32: LIBS += -L$$PWD/./ -lgclib

INCLUDEPATH += $$PWD/.
DEPENDPATH += $$PWD/.

win32: LIBS += -L$$PWD/./ -lgclibo

INCLUDEPATH += $$PWD/.
DEPENDPATH += $$PWD/.

DISTFILES += \
    PrinterSettings.txt
