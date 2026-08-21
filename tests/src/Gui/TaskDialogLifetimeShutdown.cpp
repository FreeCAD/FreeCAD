// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QApplication>

#include <Python.h>

#include <memory>

#include <Base/Interpreter.h>
#include <Gui/Application.h>
#include <Gui/TaskView/TaskDialogPython.h>

#include <src/App/InitApplication.h>

int main(int argc, char** argv)
{
    QApplication qtApplication(argc, argv);
    tests::initApplication();
    Gui::Application::initApplication();

    std::unique_ptr<Gui::TaskView::TaskDialogPython> taskDialog;
    int result = 0;
    {
        auto guiApplication = std::make_unique<Gui::Application>(false);

        PyGILState_STATE gilState = PyGILState_Ensure();
        PyObject* mainModule = PyImport_AddModule("__main__");
        PyObject* mainDict = PyModule_GetDict(mainModule);
        PyObject* dialog
            = PyRun_String("type('TaskDialogLifetime', (), {})()", Py_eval_input, mainDict, mainDict);
        if (!dialog) {
            PyErr_Print();
            result = 1;
        }
        else {
            Py::Object pyDialog(dialog, true);
            taskDialog = std::make_unique<Gui::TaskView::TaskDialogPython>(pyDialog);

            // Framework teardown clears the Python form reference while the
            // interpreter is running. The native object itself deliberately
            // survives until after Py_Finalize().
            taskDialog->closed();
            pyDialog = Py::None();
        }
        PyGILState_Release(gilState);
    }

    Base::Interpreter().finalize();
    if (Py_IsInitialized() != 0) {
        result = 2;
    }

    // This must not access Python: the interpreter has already finalized.
    taskDialog.reset();
    return result;
}
