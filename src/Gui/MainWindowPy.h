/***************************************************************************
 *   Copyright (c) 2021 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#pragma once

#include <Base/PyObjectBase.h>
#include <CXX/Extensions.hxx>
#include <QPointer>
#include <FCGlobal.h>

namespace Gui
{
class MainWindow;

class GuiExport MainWindowPy: public Py::PythonExtension<MainWindowPy>
{
public:
    static void init_type();
    static PyObject* extension_object_new(PyTypeObject* subtype, PyObject* /*args*/, PyObject* /*kwds*/);

    static Py::Object createWrapper(MainWindow* mw);
    static Py::Object type();
    static Py::ExtensionObject<MainWindowPy> create(MainWindow* mw);

    explicit MainWindowPy(MainWindow* mw);
    ~MainWindowPy() override;

    Py::Object repr() override;

    Py::Object getWindows(const Py::Tuple&);
    Py::Object getWindowsOfType(const Py::Tuple&);
    Py::Object setActiveWindow(const Py::Tuple&);
    Py::Object getActiveWindow(const Py::Tuple&);
    Py::Object addWindow(const Py::Tuple&);
    Py::Object removeWindow(const Py::Tuple&);

    Py::Object showHint(const Py::Tuple&);
    Py::Object hideHint(const Py::Tuple&);

private:
    QPointer<MainWindow> _mw;
};

}  // namespace Gui
