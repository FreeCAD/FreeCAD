
#include <Python.h>
#include <QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    char* name = "Qt example";
    Py_SetProgramName(name);
    Py_Initialize();
    PySys_SetArgv(argc, argv);

    QApplication app(argc, argv);
    MainWindow mainWin;
    mainWin.resize(600,400);
    mainWin.show();
    return app.exec();
}
