// SPDX-License-Identifier: LGPL-2.1-or-later

#include <Python.h>

#include <Base/Interpreter.h>

#include <src/App/InitApplication.h>

int main(int /*argc*/, char** /*argv*/)
{
    tests::initApplication();

    int result = 0;
    PyGILState_STATE gilState = PyGILState_Ensure();
    if (PyRun_SimpleString(
            "import FreeCAD as App\n"
            "_freecad_typepy_lifetime = "
            "App.Base.TypeId.fromName('App::StringHasher').createInstance()\n"
        )
        != 0) {
        PyErr_Print();
        result = 1;
    }

    App::GetApplication().prepareForShutdown();
    PyGILState_Release(gilState);

    // The Python wrapper deliberately remains reachable until Py_Finalize().
    // Its generic tp_dealloc deletes the native twin, so this exercises native
    // destruction from a Python-owned finalization path without retaining a
    // native-owned wrapper reference that would intentionally refuse shutdown.
    Base::Interpreter().finalize();
    if (Py_IsInitialized() != 0) {
        result = 2;
    }
    return result;
}
