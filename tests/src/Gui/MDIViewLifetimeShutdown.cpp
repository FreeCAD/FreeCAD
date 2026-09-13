// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QApplication>

#include <Python.h>

#include <memory>

#include <App/Application.h>
#include <Base/Interpreter.h>
#include <Gui/Application.h>
#include <Gui/MDIView.h>

#include <src/App/InitApplication.h>

int main(int argc, char** argv)
{
    QApplication qtApplication(argc, argv);
    tests::initApplication();
    Gui::Application::initApplication();
    Gui::Application::initOpenInventor();

    int result = 0;
    {
        auto guiApplication = std::make_unique<Gui::Application>(true);

        PyGILState_STATE gilState = PyGILState_Ensure();
        auto view = std::make_unique<Gui::MDIView>(nullptr, nullptr);
        PyObject* wrapper = view->getPyObject();
        PyObject* mainModule = PyImport_AddModule("__main__");
        PyObject* mainDict = PyModule_GetDict(mainModule);
        if (PyDict_SetItemString(mainDict, "_freecad_mdi_lifetime_view", wrapper) != 0) {
            PyErr_Print();
            result = 1;
        }
        Py_DECREF(wrapper);
        view.reset();

        // The native MDI view is gone while its Python wrapper remains
        // reachable from __main__ until Py_Finalize().
        guiApplication->prepareForShutdown();
        App::GetApplication().prepareForShutdown();
        PyGILState_Release(gilState);
    }

    Base::Interpreter().finalize();
    if (Py_IsInitialized() != 0) {
        result = 2;
    }
    return result;
}
