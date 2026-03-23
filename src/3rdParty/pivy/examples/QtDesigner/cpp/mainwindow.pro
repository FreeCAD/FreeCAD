SOURCES       = main.cpp mainwindow.cpp
HEADERS       = mainwindow.h
FORMS         = ../test.ui

LIBS = -framework Inventor -framework SoQt `coin-config --ldflags --libs`
