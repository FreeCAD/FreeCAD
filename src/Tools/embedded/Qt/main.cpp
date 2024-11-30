
#include <Python.h>
#include <QApplication>

#include "mainwindow.h"

int main(int argc, char* argv[])
{
    const char* name = "Qt example";
    Py_SetProgramName(Py_DecodeLocale(name, NULL));
    Py_Initialize();

    size_t size = argc;
    wchar_t** _argv = new wchar_t*[size];
    for (int i = 0; i < argc; i++) {
        _argv[i] = Py_DecodeLocale(argv[i], NULL);
    }
    PySys_SetArgv(argc, _argv);

    QApplication app(argc, argv);
    MainWindow mainWin;
    mainWin.resize(600, 400);
    mainWin.show();
    return app.exec();
}
