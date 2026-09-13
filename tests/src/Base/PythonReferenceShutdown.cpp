// SPDX-License-Identifier: LGPL-2.1-or-later

#include <Python.h>

#include "Base/Interpreter.h"
#include "Base/NativePythonReference.h"

int main(int argc, char** argv)
{
    Base::Interpreter().init(argc, argv);

    PyGILState_STATE gilState = PyGILState_Ensure();
    PyObject* object = PyList_New(0);
    if (!object) {
        PyGILState_Release(gilState);
        Base::Interpreter().finalize();
        return 1;
    }

    {
        Base::NativePythonReference reference(object);
        reference.reset();
    }

    PyGILState_Release(gilState);
    Base::Interpreter().finalize();
    return Py_IsInitialized() == 0 ? 0 : 1;
}
