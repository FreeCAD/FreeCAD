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
            "_freecad_feature_lifetime_doc = App.newDocument(\"FeaturePythonLifetime\")\n"
            "_freecad_feature_lifetime_object = "
            "_freecad_feature_lifetime_doc.addObject(\"App::FeaturePython\", \"Feature\")\n"
            "_freecad_feature_lifetime_object.onChanged = lambda obj, prop: None\n"
        )
        != 0) {
        PyErr_Print();
        result = 1;
    }

    if (result == 0) {
        auto* document = App::GetApplication().getDocument("FeaturePythonLifetime");
        if (!document || !App::GetApplication().closeDocument(document)) {
            result = 2;
        }
    }

    // The FeaturePython wrapper remains reachable from __main__. Its native
    // twin and callback dictionary must therefore be safe to destroy during
    // Python finalization.
    App::GetApplication().prepareForShutdown();
    PyGILState_Release(gilState);
    Base::Interpreter().finalize();
    if (Py_IsInitialized() != 0) {
        result = 3;
    }
    return result;
}
