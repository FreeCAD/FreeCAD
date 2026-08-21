// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QApplication>

#include <Python.h>

#include <memory>

#include <App/Application.h>
#include <Base/Interpreter.h>
#include <Gui/Application.h>
#include <Gui/MainWindow.h>
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
        auto guiApplication = std::make_unique<Gui::Application>(true);
        auto mainWindow = std::make_unique<Gui::MainWindow>();
        auto viewer = std::make_unique<Gui::View3DInventorViewer>(nullptr);

        PyGILState_STATE gilState = PyGILState_Ensure();
        PyObject* mainModule = PyImport_AddModule("__main__");
        PyObject* mainDict = PyModule_GetDict(mainModule);
        PyObject* viewerWrapper = viewer->getPyObject();
        if (PyDict_SetItemString(mainDict, "_freecad_editable_label_lifetime_viewer", viewerWrapper)
            != 0) {
            PyErr_Print();
            result = 1;
        }
        Py_DECREF(viewerWrapper);

        if (result == 0
            && PyRun_SimpleString(
                   "import FreeCAD as App\n"
                   "import FreeCADGui as Gui\n"
                   "_freecad_editable_label_lifetime_label = Gui.EditableDatumLabel(\n"
                   "    _freecad_editable_label_lifetime_viewer,\n"
                   "    App.Placement()\n"
                   ")\n"
                   "_freecad_editable_label_lifetime_label.setValueChangedCallback(\n"
                   "    lambda value: None\n"
                   ")\n"
               ) != 0) {
            PyErr_Print();
            result = 2;
        }

        viewer.reset();
        guiApplication->prepareForShutdown();
        App::GetApplication().prepareForShutdown();
        PyGILState_Release(gilState);
    }

    Base::Interpreter().finalize();
    if (Py_IsInitialized() != 0) {
        result = 3;
    }
    return result;
}
