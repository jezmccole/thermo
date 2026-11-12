QT += core gui qml quick quickcontrols2 network dbus

CONFIG += c++17 release
TARGET = thermo
TEMPLATE = app

INCLUDEPATH += /usr/include/LayerShellQt
LIBS += -lLayerShellQtInterface

HEADERS += \
    src/backend.h \
    src/sensormanager.h \
    src/theme.h \
    src/translator.h

SOURCES += \
    src/main.cpp \
    src/backend.cpp \
    src/sensormanager.cpp \
    src/theme.cpp \
    src/translator.cpp

RESOURCES += src/resources.qrc
