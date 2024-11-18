// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QApplication>
#include <QKeyEvent>
#include <QTime>
#include <QGuiApplication>
#endif

#include "PythonTracing.h"
#include <App/Application.h>
#include <Base/Interpreter.h>

using namespace Gui;

struct PythonTracing::Private
{
    bool active{false};
    int timeout{200}; //NOLINT

    // NOLINTBEGIN
    static int profilerInterval;
    static bool profilerDisabled;
    // NOLINTEND
};

// NOLINTBEGIN
int PythonTracing::Private::profilerInterval = 200;
bool PythonTracing::Private::profilerDisabled = false;
// NOLINTEND

PythonTracing::PythonTracing()
    : d{std::make_unique<Private>()}
{
}

PythonTracing::~PythonTracing() = default;

bool PythonTracing::isActive() const
{
    return d->active;
}

void PythonTracing::activate()
{
    d->active = true;
    setPythonTraceEnabled(true);
}

void PythonTracing::deactivate()
{
    d->active = false;
    setPythonTraceEnabled(false);
}

void PythonTracing::fetchFromSettings()
{
    const long defaultTimeout = 200;

    auto parameterGroup = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/PythonConsole");
    int interval = static_cast<int>(parameterGroup->GetInt("ProfilerInterval", defaultTimeout));
    setTimeout(interval);
}

bool PythonTracing::interrupt() const
{
    if (isActive()) {
        PyErr_SetInterrupt();
        return true;
    }

    return false;
}

void PythonTracing::setTimeout(int ms)
{
    d->timeout = ms;
}

int PythonTracing::timeout() const
{
    return d->timeout;
}

void PythonTracing::setPythonTraceEnabled(bool enabled) const
{
    Py_tracefunc trace = nullptr;
    if (enabled && timeout() > 0) {
        Private::profilerInterval = timeout();
        trace = &tracer_callback;
    }
    else {
        Private::profilerDisabled = true;
    }

    Base::PyGILStateLocker lock;
    PyEval_SetTrace(trace, nullptr);
}

/*
 * This callback ensures that Qt runs its event loop (i.e. updates the GUI, processes keyboard and
 * mouse events, etc.) at least every 200 ms, even when there is long-running Python code on the
 * main thread. It is registered as the global trace function of the Python environment.
 */
int PythonTracing::tracer_callback(PyObject *obj, PyFrameObject *frame, int what, PyObject *arg)
{
    Q_UNUSED(obj)
    Q_UNUSED(frame)
    Q_UNUSED(what)
    Q_UNUSED(arg)

    static QTime lastCalledTime = QTime::currentTime();
    QTime currTime = QTime::currentTime();

    // if previous code object was executed
    if (Private::profilerDisabled) {
        Private::profilerDisabled = false;
        lastCalledTime = currTime;
    }

    int ms = lastCalledTime.msecsTo(currTime);

    if (ms >= Private::profilerInterval) {
        lastCalledTime = currTime;
        QGuiApplication::processEvents();
    }

    return 0;
}

// ------------------------------------------------------------------------------------------------

PythonTracingLocker::PythonTracingLocker(PythonTracing& trace)
    : trace{trace}
{
    trace.activate();
}

PythonTracingLocker::~PythonTracingLocker()
{
    trace.deactivate();
}

// ------------------------------------------------------------------------------------------------

PythonTracingWatcher::PythonTracingWatcher(QObject* parent)
    : QObject(parent)
{
    qApp->installEventFilter(this);
}

PythonTracingWatcher::~PythonTracingWatcher()
{
    qApp->removeEventFilter(this);
}

bool PythonTracingWatcher::eventFilter(QObject* object, QEvent* event)
{
    if (event && event->type() == QEvent::ShortcutOverride) {
        auto kevent = static_cast<QKeyEvent*>(event);
        if (kevent->key() == Qt::Key_C && kevent->modifiers() == Qt::ControlModifier) {
            if (trace.interrupt()) {
                return true;
            }
        }
    }

    return QObject::eventFilter(object, event);
}

PythonTracing& PythonTracingWatcher::getTrace()
{
    trace.fetchFromSettings();
    return trace;
}
