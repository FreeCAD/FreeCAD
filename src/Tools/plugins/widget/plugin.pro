
TEMPLATE = lib
CONFIG += plugin
QT += designer
TARGET = FreeCAD_widgets
DEPENDPATH += .
INCLUDEPATH += .

# Input
HEADERS += customwidgets.h plugin.h
SOURCES += customwidgets.cpp plugin.cpp
