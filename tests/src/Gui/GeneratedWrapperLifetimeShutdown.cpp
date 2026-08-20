// SPDX-License-Identifier: LGPL-2.1-or-later

#include <QApplication>

#include <Python.h>

#include <memory>

#include <Base/Interpreter.h>
#include <Gui/Application.h>

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
        if (PyRun_SimpleString(
                "import FreeCADGui as Gui\n"
                "_freecad_axis_origin_lifetime = Gui.AxisOrigin()\n"
                "_freecad_link_view_lifetime = Gui.LinkView()\n"
            )
            != 0) {
            PyErr_Print();
            result = 1;
        }
        PyGILState_Release(gilState);
    }

    // These generated Delete=True wrappers intentionally remain Python-owned
    // until Py_Finalize(); their native twins must be safe to delete there.
    Base::Interpreter().finalize();
    if (Py_IsInitialized() != 0) {
        result = 2;
    }
    return result;
}
