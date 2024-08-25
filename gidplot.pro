#-------------------------------------------------
#
# Project created by QtCreator 2019-03-29T09:44:17
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

CONFIG += c++14

TARGET = gidplot
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
#DEFINES += QT_DEPRECATED_WARNINGS

QMAKE_CXXFLAGS += -Wno-deprecated-declarations

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

include(appicon/appicon.pri)

SOURCES += \
    src/CsvImportDialog.cpp \
    src/PopoutTabWidget.cpp \
    src/ProgressDialog.cpp \
    src/Range.cpp \
    src/aboutdialog.cpp \
    src/crosshair.cpp \
    src/main.cpp \
    src/mainwindow.cpp \
    src/QCustomPlot/qcustomplot.cpp \
    src/plotwidget.cpp \
    src/tablewidget.cpp \
    src/matrix.cpp \
    src/csvimporter.cpp

HEADERS += \
    src/CsvImportDialog.h \
    src/PopoutTabWidget.h \
    src/ProgressDialog.h \
    src/Range.h \
    src/aboutdialog.h \
    src/crosshair.h \
    src/csv.h \
    src/defer.h \
    src/mainwindow.h \
    src/QCustomPlot/qcustomplot.h \
    src/plotwidget.h \
    src/tablewidget.h \
    src/matrix.h \
    src/csvimporter.h \
    src/utils.h \
    src/version.h

FORMS += \
    src/CsvImportDialog.ui \
    src/ProgressDialog.ui \
    src/aboutdialog.ui \
    src/mainwindow.ui \
    src/plotwidget.ui \
    src/tablewidget.ui

RESOURCES += \
    images/images.qrc \
    src/text.qrc
