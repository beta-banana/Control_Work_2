QT += core gui widgets
CONFIG += c++17
TEMPLATE = app
TARGET = Control_Work_02_Polymorphism
SOURCES += main.cpp mainwindow.cpp instruments.cpp uav.cpp sensor.cpp navigation.cpp controller.cpp autopilot.cpp debuglog.cpp selftest.cpp telemetrymodel.cpp telemetrychart.cpp telemetrypage.cpp
HEADERS += mainwindow.h instruments.h uav.h sensor.h navigation.h controller.h autopilot.h debuglog.h selftest.h telemetrymodel.h telemetrychart.h telemetrypage.h
RESOURCES += resources.qrc
