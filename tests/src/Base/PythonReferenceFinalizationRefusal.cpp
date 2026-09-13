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

    Base::NativePythonReference reference(object);
    PyGILState_Release(gilState);

    // A live native owner must prevent interpreter finalization. This is a
    // deliberate invariant-failure subprocess, so leaving Python initialized
    // at process exit is the expected safe outcome.
    Base::Interpreter().finalize();
    const bool refusedFinalization = Py_IsInitialized() != 0;

    // Transfer the still-owned reference without DECREFing it. The process is
    // intentionally exiting with Python alive after the finalization refusal.
    reference.release();
    return refusedFinalization ? 0 : 1;
}
