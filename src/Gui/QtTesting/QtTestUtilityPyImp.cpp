// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 David Carter <dcarter@david.carter.ca>             *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include <Python.h>

#include <QApplication>
#include <QFileDialog>

#include <Base/PyWrapParseTupleAndKeywords.h>

#include "MainWindow.h"

#include "QtTesting/QtTestUtility.h"

#include "QtTesting/QtTestUtilityPy.h"
#include "QtTesting/QtTestUtilityPy.cpp"


using namespace QtTesting;


// returns a string which represents the object e.g. when printed in python
std::string QtTestUtilityPy::representation() const
{
    return "<QtTesting::QtTestUtility>";
}

PyObject* QtTestUtilityPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int QtTestUtilityPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}


PyObject* QtTestUtilityPy::play(PyObject* args, PyObject* kwds)
{
    QStringList tests;

    PyObject* filePy = Py_None;
    static const std::array<const char*, 2> kwds_record {"file", nullptr};
    if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "O", kwds_record, &filePy)) {
        if (PyUnicode_Check(filePy)) {
            Py::String pyName(filePy);
            if (!pyName.isNone()) {
                tests.append(QString::fromStdString(pyName.as_string()));
            }
        }
        else if (PyList_Check(filePy)) {
        }
        else if (filePy != Py_None) {
            throw Py::TypeError("Expected string, list, or None");
        }
    }
    else {
        throw Py::TypeError("Expected string, list, or None");
    }

    auto mainWindow = Gui::getMainWindow();
    auto& testUtility = mainWindow->getTestUtility();
    bool pass = false;
    if (!tests.isEmpty()) {
        pass = testUtility.playTests(tests);
    }

    if (pass) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

PyObject* QtTestUtilityPy::playingTest(PyObject* args)
{
    auto mainWindow = Gui::getMainWindow();
    auto& testUtility = mainWindow->getTestUtility();
    auto playing = testUtility.playingTest();

    if (playing) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

PyObject* QtTestUtilityPy::stopTests(PyObject* args)
{
    Base::Console().log("Stopping playback\n");

    auto mainWindow = Gui::getMainWindow();
    auto& testUtility = mainWindow->getTestUtility();
    testUtility.stopTests();

    Py_RETURN_NONE;
}


PyObject* QtTestUtilityPy::record(PyObject* args, PyObject* kwds)
{
    QString filename;
    PyObject* filePy = Py_None;
    static const std::array<const char*, 2> kwds_record {"file", nullptr};
    if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "|O", kwds_record, &filePy)) {
        if (PyUnicode_Check(filePy)) {
            Py::String pyName(filePy);
            if (!pyName.isNone()) {
                filename = QString::fromStdString(pyName.as_string());
            }
        }
    }
    else {
        throw Py::TypeError("Expected string or None");
    }

    auto mainWindow = Gui::getMainWindow();
    auto& testUtility = mainWindow->getTestUtility();
    if (filename.isEmpty()) {
        filename = QFileDialog::getSaveFileName(
            mainWindow,
            QStringLiteral("Test File Name"),
            QString(),
            QStringLiteral("XML Files (*.xml)")
        );
    }
    if (!filename.isEmpty()) {
        QApplication::setActiveWindow(mainWindow);
        testUtility.recordTests(filename);
    }

    Py_RETURN_NONE;
}

PyObject* QtTestUtilityPy::stopRecording(PyObject* args)
{
    Base::Console().log("Stopping recording\n");

    auto mainWindow = Gui::getMainWindow();
    auto& testUtility = mainWindow->getTestUtility();
    if (testUtility.recorder()->isRecording()) {
        // testUtility.stopRecords(1);
        testUtility.recorder()->stop(1);
    }

    Py_RETURN_NONE;
}

PyObject* QtTestUtilityPy::pauseRecording(PyObject* args)
{
    Base::Console().log("Pausing recording\n");

    auto mainWindow = Gui::getMainWindow();
    auto& testUtility = mainWindow->getTestUtility();
    testUtility.pauseRecords(true);

    Py_RETURN_NONE;
}

PyObject* QtTestUtilityPy::resumeRecording(PyObject* args)
{
    Base::Console().log("Resuming recording\n");

    auto mainWindow = Gui::getMainWindow();
    auto& testUtility = mainWindow->getTestUtility();
    testUtility.pauseRecords(false);

    Py_RETURN_NONE;
}
