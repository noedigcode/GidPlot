#-------------------------------------------------
#
# Project created by QtCreator 2019-03-29T09:44:17
#
#-------------------------------------------------

QT       += core gui svg

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
include(src/QGeoView/QGeoView.pri)

SOURCES += \
    src/CrosshairsDialog.cpp \
    src/CsvImportDialog.cpp \
    src/LinkDialog.cpp \
    src/MapPlot.cpp \
    src/MarkerEditDialog.cpp \
    src/PlotMarkerItem.cpp \
    src/PopoutTabWidget.cpp \
    src/ProgressDialog.cpp \
    src/QCustomPlot/GidQCustomPlot.cpp \
    src/Range.cpp \
    src/aboutdialog.cpp \
    src/graph.cpp \
    src/link.cpp \
    src/main.cpp \
    src/mainwindow.cpp \
    src/QCustomPlot/qcustomplot.cpp \
    src/mapMarker.cpp \
    src/mapline.cpp \
    src/plot.cpp \
    src/plotwindow.cpp \
    src/subplot.cpp \
    src/tablewidget.cpp \
    src/matrix.cpp \
    src/csvimporter.cpp

HEADERS += \
    src/CrosshairsDialog.h \
    src/CsvImportDialog.h \
    src/LinkDialog.h \
    src/MapPlot.h \
    src/MarkerEditDialog.h \
    src/PlotMarkerItem.h \
    src/PopoutTabWidget.h \
    src/ProgressDialog.h \
    src/QCustomPlot/GidQCustomPlot.h \
    src/Range.h \
    src/aboutdialog.h \
    src/csv.h \
    src/defer.h \
    src/graph.h \
    src/link.h \
    src/mainwindow.h \
    src/QCustomPlot/qcustomplot.h \
    src/mapMarker.h \
    src/mapline.h \
    src/plot.h \
    src/plotwindow.h \
    src/subplot.h \
    src/tablewidget.h \
    src/matrix.h \
    src/csvimporter.h \
    src/utils.h \
    src/version.h

FORMS += \
    src/CrosshairsDialog.ui \
    src/CsvImportDialog.ui \
    src/LinkDialog.ui \
    src/MarkerEditDialog.ui \
    src/ProgressDialog.ui \
    src/aboutdialog.ui \
    src/mainwindow.ui \
    src/plotwindow.ui \
    src/tablewidget.ui

RESOURCES += \
    images/images.qrc \
    src/text.qrc
