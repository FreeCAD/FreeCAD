// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QApplication>

#include <Python.h>

#include <memory>

#include <App/Application.h>
#include <Base/Interpreter.h>
#include <Gui/Application.h>
#include <Gui/MainWindow.h>
#include <Gui/Navigation/NavigationStyle.h>
#include <Gui/View3DInventorViewer.h>

#include <src/App/InitApplication.h>

int main(int argc, char** argv)
{
    QApplication qtApplication(argc, argv);
    tests::initApplication();
    Gui::Application::initApplication();
    Gui::Application::initOpenInventor();
    App::GetApplication()
        .GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")
        ->SetBool("ShowNaviCube", false);

    int result = 0;
    {
        auto guiApplication = std::make_unique<Gui::Application>(false);
        auto mainWindow = std::make_unique<Gui::MainWindow>();
        auto viewer = std::make_unique<Gui::View3DInventorViewer>(nullptr);

        PyGILState_STATE gilState = PyGILState_Ensure();
        PyObject* wrapper = viewer->getPyObject();
        PyObject* navigationWrapper = viewer->navigationStyle()->getPyObject();

        // Keep the wrapper reachable from Python. Its native-owned reference
        // is released when the viewer is destroyed below; the remaining
        // Python reference is deliberately left for Py_Finalize() to release.
        PyObject* mainModule = PyImport_AddModule("__main__");
        PyObject* mainDict = PyModule_GetDict(mainModule);
        if (PyDict_SetItemString(mainDict, "_freecad_viewer_lifetime_test", wrapper) != 0) {
            PyErr_Print();
            result = 1;
        }
        if (PyDict_SetItemString(mainDict, "_freecad_navigation_style_lifetime_test", navigationWrapper)
            != 0) {
            PyErr_Print();
            result = 1;
        }
        Py_DECREF(wrapper);
        Py_DECREF(navigationWrapper);
        viewer.reset();

        if (result == 0) {
            PyObject* retained
                = PyDict_GetItemString(mainDict, "_freecad_navigation_style_lifetime_test");
            PyObject* access = PyObject_CallMethod(retained, "isRotationEnabled", nullptr);
            if (access) {
                Py_DECREF(access);
                result = 2;
            }
            else if (!PyErr_ExceptionMatches(PyExc_ReferenceError)) {
                PyErr_Print();
                result = 3;
            }
            else {
                PyErr_Clear();
            }
        }
        PyGILState_Release(gilState);
    }

    Base::Interpreter().finalize();
    if (Py_IsInitialized() != 0) {
        result = 4;
    }
    return result;
}
