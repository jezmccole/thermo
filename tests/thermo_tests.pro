QT += core gui quick quickcontrols2 testlib dbus network
CONFIG += c++17 testcase
TARGET = thermo_tests
TEMPLATE = app

INCLUDEPATH += ../src /usr/include/LayerShellQt
LIBS += -lLayerShellQtInterface

HEADERS += \
    ../src/backend.h \
    ../src/sensormanager.h \
    ../src/theme.h \
    ../src/translator.h

SOURCES += \
    thermo_tests.cpp \
    ../src/backend.cpp \
    ../src/sensormanager.cpp \
    ../src/theme.cpp \
    ../src/translator.cpp

RESOURCES += ../src/resources.qrc
