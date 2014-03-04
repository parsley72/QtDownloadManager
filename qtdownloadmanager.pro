#-------------------------------------------------
#
# Project created by QtCreator 2011-07-24T12:59:05
#
#-------------------------------------------------

QT       += network

TARGET = qtdownloadmanager
CONFIG   += console

TEMPLATE = app

SOURCES += main.cpp \
    downloadmanager.cpp \
    downloadwidget.cpp \
    downloadmanagerFTP.cpp \
    downloadmanagerHTTP.cpp

HEADERS += \
    downloadmanager.h \
    downloadwidget.h \
    downloadmanagerFTP.h \
    downloadmanagerHTTP.h

FORMS += \
    form.ui

OTHER_FILES += \
    README.md
