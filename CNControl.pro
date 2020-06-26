#-------------------------------------------------
#
# Project created by QtCreator 2020-04-21T15:13:25
#
#-------------------------------------------------

QT       += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = CNControl
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
    QCsvFile.cpp \
    QFocusLineEdit.cpp \
    aaaa_idees.cpp \
    configurationdialog.cpp \
    gcode.cpp \
    grblconfigurationdialog.cpp \
    main.cpp \
    highlighter.cpp \
    machine.cpp \
    mainwindow.cpp \
    port_serial.cpp \
    port.cpp \
    codeeditor.cpp \
    grbl.cpp \
    visualizer.cpp

HEADERS += \
    QCsvFile \
    QFocusLineEdit \
    configurationdialog.h \
    gcode.h \
    grbl_config.h \
    grblconfigurationdialog.h \
    mainwindow.h \
    singleton.h \
    highlighter.h \
    bits.h \
    machine.h \
    port_serial.h \
    port.h \
    codeeditor.h \
    grbl.h \
    visualizer.h

FORMS += \
    configurationdialog.ui \
    grblconfigurationdialog.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc

TRANSLATIONS = app_en.ts app_fr.ts
