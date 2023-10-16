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

#ifndef GUI_PYTHONTRACING_H
#define GUI_PYTHONTRACING_H

#include <QObject>
#include <Python.h>
#include <frameobject.h>
#include <memory>
#include <FCGlobal.h>


namespace Gui {

class GuiExport PythonTracing
{
public:
    PythonTracing();
    ~PythonTracing();

    PythonTracing(const PythonTracing&) = delete;
    PythonTracing(PythonTracing&&) = delete;
    PythonTracing& operator = (const PythonTracing&) = delete;
    PythonTracing& operator = (PythonTracing&&) = delete;

    /*!
     * \brief isActive
     * Returns true if the tracing is active, false otherwise.
     * \return
     */
    bool isActive() const;
    /*!
     * \brief activate
     * Activates the Python tracing.
     */
    void activate();
    /*!
     * \brief deactivate
     * Deactivates the Python tracing.
     */
    void deactivate();
    /*!
     * \brief fetchFromSettings
     * Fetch parameters from user settings.
     */
    void fetchFromSettings();
    /*!
     * \brief interrupt
     * If the tracing is enabled it interrupts the Python interpreter.
     * True is returned if the tracing is active, false otherwise.
     */
    bool interrupt() const;
    /*!
     * \brief setTimeout
     * Sets the interval after which the Qt event loop will be processed.
     */
    void setTimeout(int ms);
    /*!
     * \brief timeout
     * \return the timeout of processing the event loop.
     */
    int timeout() const;

private:
    void setPythonTraceEnabled(bool enabled) const;
    static int tracer_callback(PyObject *obj, PyFrameObject *frame, int what, PyObject *arg);

private:
    struct Private;
    std::unique_ptr<Private> d;
};

class GuiExport PythonTracingLocker
{
public:
    /*!
     * \brief PythonTracingLocker
     * Activates the passed tracing object.
     * \param trace
     */
    explicit PythonTracingLocker(PythonTracing& trace);
    /*!
     * Deactivates the tracing object.
     */
    ~PythonTracingLocker();

    PythonTracingLocker(const PythonTracingLocker&) = delete;
    PythonTracingLocker(PythonTracingLocker&&) = delete;
    PythonTracingLocker& operator = (const PythonTracingLocker&) = delete;
    PythonTracingLocker& operator = (PythonTracingLocker&&) = delete;

private:
    PythonTracing& trace;
};

class GuiExport PythonTracingWatcher : public QObject
{
    // NOLINTNEXTLINE
    Q_OBJECT

public:
    PythonTracingWatcher(QObject* parent = nullptr);
    ~PythonTracingWatcher() override;
    /*!
     * \brief eventFilter
     * Checks for Ctrl+C keyboard events and if pressed interrupts the Python interpreter.
     * \param object
     * \param event
     * \return
     */
    bool eventFilter(QObject* object, QEvent* event) override;

    /*!
     * \brief getTrace
     * Returns the Python tracing object. It's up to the calling instance to activate and decativate it.
     * \return PythonTracing
     */
    PythonTracing& getTrace();

    PythonTracingWatcher(const PythonTracingWatcher&) = delete;
    PythonTracingWatcher(PythonTracingWatcher&&) = delete;
    PythonTracingWatcher& operator = (const PythonTracingWatcher&) = delete;
    PythonTracingWatcher& operator = (PythonTracingWatcher&&) = delete;

private:
    PythonTracing trace;
};

} // namespace Gui

#endif // GUI_PYTHONTRACING_H
