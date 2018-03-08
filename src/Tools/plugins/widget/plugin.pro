
TEMPLATE = lib
CONFIG += plugin
greaterThan(QT_MAJOR_VERSION, 4) {
  QT += designer
} else {
  CONFIG += designer
}
TARGET = FreeCAD_widgets
DEPENDPATH += .
INCLUDEPATH += .

# Input
HEADERS += customwidgets.h plugin.h
SOURCES += customwidgets.cpp plugin.cpp
