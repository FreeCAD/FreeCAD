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
            "_freecad_attribute_lifetime_doc = App.newDocument(\"FeatureTestAttributeLifetime\")\n"
            "_freecad_attribute_lifetime_object = "
            "_freecad_attribute_lifetime_doc.addObject(\"App::FeatureTestAttribute\", "
            "\"Attribute\")\n"
            "_freecad_attribute_lifetime_object.Object = _freecad_attribute_lifetime_object\n"
        )
        != 0) {
        PyErr_Print();
        result = 1;
    }

    if (result == 0) {
        auto* document = App::GetApplication().getDocument("FeatureTestAttributeLifetime");
        if (!document || !App::GetApplication().closeDocument(document)) {
            result = 2;
        }
    }

    // The Python wrapper and its PropertyPythonObject value remain reachable
    // from __main__. Native property ownership must be released before the
    // wrapper is destroyed by Py_Finalize().
    App::GetApplication().prepareForShutdown();
    PyGILState_Release(gilState);
    Base::Interpreter().finalize();
    if (Py_IsInitialized() != 0) {
        result = 3;
    }
    return result;
}
