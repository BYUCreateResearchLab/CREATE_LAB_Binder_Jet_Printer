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
    JetServer.h \
    gclib.h \
    gclibo.h \
    gclib_errors.h \
    gclib_record.h



SOURCES += \
    main.cpp \
    mainwindow.cpp \
    outputwindow.cpp \
    progwindow.cpp \
    lineprintdata.cpp \
    printobject.cpp \
    svgview.cpp \
    fakegclib.cpp \

HEADERS += \
    mainwindow.h \
    outputwindow.h \
    progwindow.h \
    lineprintdata.h \
    printobject.h \
    svgview.h \
    vec2.h \
    fakegclib.h

FORMS += \
    mainwindow.ui \
    outputwindow.ui \
    progwindow.ui \

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
