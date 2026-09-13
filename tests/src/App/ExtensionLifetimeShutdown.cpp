// SPDX-License-Identifier: LGPL-2.1-or-later

#include <Python.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Extension.h>
#include <Base/Interpreter.h>
#include <Base/Type.h>

#include <src/App/InitApplication.h>

int main(int /*argc*/, char** /*argv*/)
{
    tests::initApplication();

    int result = 0;
    PyGILState_STATE gilState = PyGILState_Ensure();
    if (PyRun_SimpleString(
            "import FreeCAD as App\n"
            "_freecad_extension_lifetime_doc = App.newDocument(\"ExtensionLifetime\")\n"
            "_freecad_extension_lifetime_object = "
            "_freecad_extension_lifetime_doc.addObject(\"App::DocumentObject\", \"Object\")\n"
            "_freecad_extension_lifetime_object.addExtension(\"App::GroupExtensionPython\")\n"
        )
        != 0) {
        PyErr_Print();
        result = 1;
    }

    if (result == 0) {
        PyObject* mainModule = PyImport_AddModule("__main__");
        PyObject* mainDict = PyModule_GetDict(mainModule);
        auto* document = App::GetApplication().getDocument("ExtensionLifetime");
        auto* object = document ? document->getObject("Object") : nullptr;
        auto* extension = object
            ? object->getExtension(Base::Type::fromName("App::GroupExtensionPython"), false, true)
            : nullptr;

        if (!extension) {
            result = 2;
        }
        else {
            PyObject* wrapper = extension->getExtensionPyObject();
            if (PyDict_SetItemString(mainDict, "_freecad_extension_lifetime", wrapper) != 0) {
                PyErr_Print();
                result = 3;
            }
            Py_DECREF(wrapper);
        }

        // The wrapper remains reachable from Python, but the native extension
        // must be destroyed while Python is still running.
        if (document && !App::GetApplication().closeDocument(document)) {
            result = 4;
        }

        if (result == 0) {
            PyObject* retained = PyDict_GetItemString(mainDict, "_freecad_extension_lifetime");
            PyObject* access = PyObject_CallMethod(retained, "getExtendedObject", nullptr);
            if (access) {
                Py_DECREF(access);
                result = 5;
            }
            else if (!PyErr_ExceptionMatches(PyExc_ReferenceError)) {
                PyErr_Print();
                result = 6;
            }
            else {
                PyErr_Clear();
            }
        }
    }

    App::GetApplication().prepareForShutdown();
    PyGILState_Release(gilState);
    Base::Interpreter().finalize();
    if (Py_IsInitialized() != 0) {
        result = 7;
    }
    return result;
}
