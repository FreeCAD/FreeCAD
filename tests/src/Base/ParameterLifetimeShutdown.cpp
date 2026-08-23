// SPDX-License-Identifier: LGPL-2.1-or-later

#include <Python.h>

#include <App/Application.h>
#include <Base/Interpreter.h>

#include <src/App/InitApplication.h>

int main(int /*argc*/, char** /*argv*/)
{
    tests::initApplication();

    int result = 0;
    PyGILState_STATE gilState = PyGILState_Ensure();
    if (PyRun_SimpleString(
            "import FreeCAD as App\n"
            "class _ShutdownObserver:\n"
            "    def onChange(self, *args):\n"
            "        pass\n"
            "_freecad_parameter_lifetime_observer = _ShutdownObserver()\n"
            "_freecad_parameter_lifetime_group = App.ParamGet(\n"
            "    'User parameter:BaseApp/Tests/PythonLifetime'\n"
            ")\n"
            "_freecad_parameter_lifetime_group.Attach(\n"
            "    _freecad_parameter_lifetime_observer\n"
            ")\n"
        )
        != 0) {
        PyErr_Print();
        result = 1;
    }

    App::GetApplication().prepareForShutdown();
    PyGILState_Release(gilState);

    Base::Interpreter().finalize();
    if (Py_IsInitialized() != 0) {
        result = 2;
    }
    return result;
}
