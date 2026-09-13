// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QApplication>
#include <QTemporaryFile>

#include <Python.h>

#include <Base/Interpreter.h>
#include <Gui/Application.h>
#include <Gui/ApplicationPy.h>

#include <src/App/InitApplication.h>

int main(int argc, char** argv)
{
    QApplication qtApplication(argc, argv);
    tests::initApplication();
    Gui::Application::initApplication();
    Gui::Application::initOpenInventor();

    QTemporaryFile uiFile;
    if (!uiFile.open()) {
        return 1;
    }
    uiFile.write(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<ui version=\"4.0\">"
        "<class>Dialog</class>"
        "<widget class=\"QDialog\" name=\"Dialog\"/>"
        "<resources/><connections/>"
        "</ui>"
    );
    uiFile.flush();
    uiFile.close();

    int result = 0;
    {
        Gui::Application guiApplication(false);

        PyGILState_STATE gilState = PyGILState_Ensure();
        PyObject* args = Py_BuildValue("(s)", uiFile.fileName().toUtf8().constData());
        PyObject* resource = args ? Gui::ApplicationPy::sCreateDialog(nullptr, args) : nullptr;
        Py_XDECREF(args);
        if (!resource) {
            PyErr_Print();
            result = 2;
        }
        else {
            PyObject* mainModule = PyImport_AddModule("__main__");
            PyObject* mainDict = PyModule_GetDict(mainModule);
            if (PyDict_SetItemString(mainDict, "_freecad_pyresource_lifetime", resource) != 0) {
                PyErr_Print();
                result = 3;
            }
            Py_DECREF(resource);
        }
        guiApplication.prepareForShutdown();
        App::GetApplication().prepareForShutdown();
        PyGILState_Release(gilState);
    }

    Base::Interpreter().finalize();
    if (Py_IsInitialized() != 0) {
        result = 4;
    }
    return result;
}
