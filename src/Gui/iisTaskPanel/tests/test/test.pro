TEMPLATE = app
TARGET = test

include(../tests.pri)

HEADERS += test.h
SOURCES += test.cpp main.cpp

FORMS += test.ui

RESOURCES += test.qrc

QMAKE_CLEAN += ""core.* $(TARGET)""

