// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 FreeCAD Project                                    *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either version 2 *
 *   of the License, or (at your option) any later version.                 *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <FCConfig.h>

#include <CXX/Extensions.hxx>
#include <utility>

namespace Base
{

class InterpreterSingleton;

/**
 * RAII owner for one Python reference held by native C++ code.
 *
 * The constructor adopts an existing owned reference. While Python is
 * running, destruction releases it synchronously, attaching the current
 * thread when necessary. Native owners must be destroyed before the
 * interpreter finalization boundary is crossed.
 */
class BaseExport NativePythonReference
{
public:
    NativePythonReference() noexcept = default;
    explicit NativePythonReference(PyObject* object) noexcept;
    explicit NativePythonReference(const Py::Object& object) noexcept;
    ~NativePythonReference();

    NativePythonReference(const NativePythonReference&) = delete;
    NativePythonReference& operator=(const NativePythonReference&) = delete;

    NativePythonReference(NativePythonReference&& other) noexcept
        : _object(std::exchange(other._object, nullptr))
    {}
    NativePythonReference& operator=(NativePythonReference&& other) noexcept
    {
        if (this != &other) {
            PyObject* previous = std::exchange(_object, std::exchange(other._object, nullptr));
            releaseOwned(previous);
        }
        return *this;
    }

    PyObject* get() const noexcept
    {
        return _object;
    }

    explicit operator bool() const noexcept
    {
        return _object != nullptr;
    }

    /** Replace the owned reference, adopting the new owned reference. */
    void reset(PyObject* object = nullptr) noexcept;
    /** Replace the owned reference, retaining a PyCXX object for native ownership. */
    void reset(const Py::Object& object) noexcept;
    /** Replace the owned reference by retaining a borrowed Python object. */
    void resetBorrowed(PyObject* object) noexcept;

    /** Release ownership without decrementing the Python reference count. */
    PyObject* release() noexcept;

private:
    static bool acquire(PyObject* object) noexcept;
    static void releaseOwned(PyObject* object) noexcept;
    static void unregister(PyObject* object) noexcept;

    // Interpreter lifecycle hooks. Only InterpreterSingleton may use these.
    static void markInterpreterRunning() noexcept;
    static bool beginInterpreterFinalization() noexcept;

    friend class InterpreterSingleton;

    PyObject* _object = nullptr;
};

/**
 * Store a callable attribute in a native-owned reference.
 *
 * The attribute lookup is performed by the caller while Python is attached;
 * the returned callable reference is adopted by reference.
 */
BaseExport void setNativePythonCallable(
    PyObject* object,
    const char* name,
    NativePythonReference& reference
) noexcept;

}  // namespace Base
