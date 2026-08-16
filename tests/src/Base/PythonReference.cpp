// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <Python.h>

#include <thread>

#include "Base/Interpreter.h"
#include "Base/NativePythonReference.h"
#include "Base/SmartPtrPy.h"

namespace
{

void ensurePython()
{
    if (!Py_IsInitialized()) {
        Py_Initialize();
    }
}

}  // namespace

TEST(PythonReference, ReleasesWithAttachedThreadState)
{
    ensurePython();
    PyGILState_STATE gilState = PyGILState_Ensure();

    PyObject* object = PyList_New(0);
    ASSERT_NE(object, nullptr);
    Py_INCREF(object);

    {
        Base::NativePythonReference reference(object);
    }

    EXPECT_EQ(Py_REFCNT(object), 1);
    Py_DECREF(object);
    PyGILState_Release(gilState);
}

TEST(PythonReference, ResetAdoptsOwnedSamePointer)
{
    ensurePython();
    PyGILState_STATE gilState = PyGILState_Ensure();

    PyObject* object = PyList_New(0);
    ASSERT_NE(object, nullptr);
    Py_INCREF(object);

    Base::NativePythonReference reference(object);
    Py_INCREF(object);
    reference.reset(object);
    reference.reset();

    EXPECT_EQ(Py_REFCNT(object), 1);
    Py_DECREF(object);
    PyGILState_Release(gilState);
}

TEST(PythonReference, ReleasesSynchronouslyFromDetachedThread)
{
    ensurePython();
    PyGILState_STATE gilState = PyGILState_Ensure();

    PyObject* object = PyList_New(0);
    ASSERT_NE(object, nullptr);
    Py_INCREF(object);

    Base::NativePythonReference reference(object);
    PyThreadState* threadState = PyEval_SaveThread();

    std::thread worker([reference = std::move(reference)]() mutable { reference.reset(); });
    worker.join();

    PyEval_RestoreThread(threadState);
    EXPECT_EQ(Py_REFCNT(object), 1);
    Py_DECREF(object);
    PyGILState_Release(gilState);
}

TEST(PythonReference, SmartPtrRetainsBorrowedObject)
{
    ensurePython();
    PyGILState_STATE gilState = PyGILState_Ensure();

    PyObject* object = PyList_New(0);
    ASSERT_NE(object, nullptr);

    {
        Py::SmartPtr reference(object);
        EXPECT_EQ(Py_REFCNT(object), 2);
    }

    EXPECT_EQ(Py_REFCNT(object), 1);
    Py_DECREF(object);
    PyGILState_Release(gilState);
}
