QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets serialport network

CONFIG += c++11

# Temporary containment: legacy debug streams include authentication material.
# Apply to debug AND release builds until structured redacted logging is in place.
DEFINES += QT_NO_DEBUG_OUTPUT
SOURCES += securityhelpers.cpp portselection.cpp
HEADERS += securityhelpers.h portselection.h

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
#DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    backendclass.cpp \
    class_logfile.cpp \
    globalvars.cpp \
    main.cpp \
    mainwindow.cpp \
    metamodethread.cpp

HEADERS += \
    backendclass.h \
    class_logfile.h \
    globalvars.h \
    mainwindow.h \
    metamodethread.h

FORMS += \
    mainwindow.ui

RESOURCES += \
        resources.qrc\



RC_ICONS = pics/appIcon.ico #myappico.ico





# Default rules for deployment.
#qnx: target.path = /tmp/$${TARGET}/bin
#else: unix:!android: target.path = /opt/$${TARGET}/bin
#!isEmpty(target.path): INSTALLS += target

