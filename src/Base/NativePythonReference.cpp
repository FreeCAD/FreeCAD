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

#include <condition_variable>
#include <mutex>
#include <utility>

#include "Interpreter.h"
#include "NativePythonReference.h"
#include "Console.h"

namespace Base
{
namespace
{

enum class State
{
    Uninitialized,
    Running,
    Finalizing,
};

class NativePythonReferenceState
{
public:
    void markRunning() noexcept
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_state == State::Uninitialized) {
            _state = State::Running;
        }
    }

    bool acquire() noexcept
    {
        const char* error = nullptr;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_state == State::Uninitialized) {
                bool interpreterRunning = Py_IsInitialized();
#if PY_VERSION_HEX >= 0x030d0000
                interpreterRunning = interpreterRunning && !Py_IsFinalizing();
#endif
                if (interpreterRunning) {
                    _state = State::Running;
                }
            }

            if (_state != State::Running) {
                error = "Native Python ownership acquired after the finalization barrier; "
                        "the incoming reference was leaked";
            }
            else {
                ++_nativeReferences;
                return true;
            }
        }

        Base::Console().error("%s\n", error);
        return false;
    }

    void unregister() noexcept
    {
        bool violation = false;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_nativeReferences == 0) {
                violation = true;
            }
            else {
                --_nativeReferences;
            }
        }

        if (violation) {
            Base::Console().error(
                "Native Python ownership accounting underflow while transferring a reference\n"
            );
        }
    }

    void release(PyObject* object) noexcept
    {
        if (!object) {
            return;
        }

        bool releaseReference = false;
        bool leak = false;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_nativeReferences == 0) {
                leak = true;
            }
            else {
                --_nativeReferences;
            }

            if (!leak && _state == State::Running) {
#if PY_VERSION_HEX >= 0x030d0000
                if (Py_IsFinalizing()) {
                    leak = true;
                }
#endif
                if (leak) {
                    // The interpreter began finalization without going through
                    // this lifecycle barrier. Do not attach to it.
                }
                else {
                    // beginFinalization() cannot transition to Finalizing until
                    // this active release completes, so acquiring the thread state outside
                    // the mutex cannot race with interpreter finalization.
                    ++_activeReleases;
                    releaseReference = true;
                }
            }
            else {
                leak = true;
            }
        }

        if (leak) {
            Base::Console().error(
                "Native Python reference released outside the running lifecycle; leaking one "
                "reference\n"
            );
        }

        if (!releaseReference) {
            return;
        }

        {
            PyGILStateLocker lock;
            Py_DECREF(object);
        }
        finishRelease();
    }

    bool beginFinalization() noexcept
    {
        std::unique_lock<std::mutex> lock(_mutex);
        if (_state != State::Running) {
            return false;
        }

        _state = State::Finalizing;
        const std::size_t remaining = _nativeReferences;
        _condition.wait(lock, [this]() { return _activeReleases == 0; });
        const bool noReferences = remaining == 0;
        lock.unlock();
        if (!noReferences) {
            Base::Console().error(
                "Cannot finalize Python: %zu native Python references remain\n",
                remaining
            );
        }
        return noReferences;
    }

private:
    void finishRelease() noexcept
    {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            --_activeReleases;
        }
        _condition.notify_all();
    }

    std::mutex _mutex;
    std::condition_variable _condition;
    State _state = State::Uninitialized;
    unsigned int _activeReleases = 0;
    std::size_t _nativeReferences = 0;
};

NativePythonReferenceState& nativePythonReferences()
{
    static NativePythonReferenceState state;
    return state;
}

}  // namespace

bool NativePythonReference::acquire(PyObject* object) noexcept
{
    return object ? nativePythonReferences().acquire() : true;
}

void NativePythonReference::releaseOwned(PyObject* object) noexcept
{
    nativePythonReferences().release(object);
}

void NativePythonReference::unregister(PyObject* object) noexcept
{
    if (object) {
        nativePythonReferences().unregister();
    }
}

void NativePythonReference::markInterpreterRunning() noexcept
{
    nativePythonReferences().markRunning();
}

bool NativePythonReference::beginInterpreterFinalization() noexcept
{
    return nativePythonReferences().beginFinalization();
}

NativePythonReference::NativePythonReference(PyObject* object) noexcept
{
    if (acquire(object)) {
        _object = object;
    }
}

NativePythonReference::NativePythonReference(const Py::Object& object) noexcept
{
    resetBorrowed(object.ptr());
}

void setNativePythonCallable(PyObject* object, const char* name, NativePythonReference& reference) noexcept
{
    reference.reset();
    if (!object || !name || !PyObject_HasAttrString(object, name)) {
        return;
    }

    PyObject* callable = PyObject_GetAttrString(object, name);
    if (callable && PyCallable_Check(callable)) {
        reference.reset(callable);
    }
    else {
        Py_XDECREF(callable);
    }
}

NativePythonReference::~NativePythonReference()
{
    reset();
}

void NativePythonReference::reset(PyObject* object) noexcept
{
    if (object && !acquire(object)) {
        return;
    }

    PyObject* previous = std::exchange(_object, object);
    releaseOwned(previous);
}

void NativePythonReference::reset(const Py::Object& object) noexcept
{
    resetBorrowed(object.ptr());
}

void NativePythonReference::resetBorrowed(PyObject* object) noexcept
{
    if (!object) {
        reset();
        return;
    }

    // Reserve native ownership before touching the Python reference count so
    // the finalization barrier cannot miss this new owner.
    if (!acquire(object)) {
        return;
    }

    Py_INCREF(object);
    PyObject* previous = std::exchange(_object, object);
    releaseOwned(previous);
}

PyObject* NativePythonReference::release() noexcept
{
    PyObject* object = std::exchange(_object, nullptr);
    unregister(object);
    return object;
}

}  // namespace Base
