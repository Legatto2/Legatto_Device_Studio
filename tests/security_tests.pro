QT += core widgets testlib
CONFIG += c++11 testcase console
CONFIG -= app_bundle
TARGET = security_tests
DEFINES += QT_NO_DEBUG_OUTPUT
INCLUDEPATH += ..
SOURCES += security_tests.cpp ../securityhelpers.cpp ../portselection.cpp
HEADERS += ../securityhelpers.h ../portselection.h
