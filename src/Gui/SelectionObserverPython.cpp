/***************************************************************************
 *   Copyright (c) 2022 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include "PreCompiled.h"

#include "SelectionObserverPython.h"
#include <Base/Interpreter.h>


FC_LOG_LEVEL_INIT("Selection",false,true,true)

using namespace Gui;

std::vector<SelectionObserverPython*> SelectionObserverPython::_instances;

SelectionObserverPython::SelectionObserverPython(const Py::Object& obj, ResolveMode resolve)
    : SelectionObserver(true, resolve), inst(obj)
{
#undef FC_PY_ELEMENT
#define FC_PY_ELEMENT(_name) FC_PY_GetCallable(obj.ptr(),#_name,py_##_name);
    FC_PY_SEL_OBSERVER
}

SelectionObserverPython::~SelectionObserverPython() = default;

void SelectionObserverPython::addObserver(const Py::Object& obj, ResolveMode resolve)
{
    _instances.push_back(new SelectionObserverPython(obj, resolve));
}

void SelectionObserverPython::removeObserver(const Py::Object& obj)
{
    SelectionObserverPython* obs=nullptr;
    for (std::vector<SelectionObserverPython*>::iterator it =
        _instances.begin(); it != _instances.end(); ++it) {
        if ((*it)->inst == obj) {
            obs = *it;
            _instances.erase(it);
            break;
        }
    }

    delete obs;
}

void SelectionObserverPython::onSelectionChanged(const SelectionChanges& msg)
{
    switch (msg.Type)
    {
    case SelectionChanges::AddSelection:
        addSelection(msg);
        break;
    case SelectionChanges::RmvSelection:
        removeSelection(msg);
        break;
    case SelectionChanges::SetSelection:
        setSelection(msg);
        break;
    case SelectionChanges::ClrSelection:
        clearSelection(msg);
        break;
    case SelectionChanges::SetPreselect:
        setPreselection(msg);
        break;
    case SelectionChanges::RmvPreselect:
        removePreselection(msg);
        break;
    case SelectionChanges::PickedListChanged:
        pickedListChanged();
        break;
    default:
        break;
    }
}

void SelectionObserverPython::pickedListChanged()
{
    if(py_pickedListChanged.isNone())
        return;
    Base::PyGILStateLocker lock;
    try {
        Py::Callable(py_pickedListChanged).apply(Py::Tuple());
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void SelectionObserverPython::addSelection(const SelectionChanges& msg)
{
    if(py_addSelection.isNone())
        return;
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(4);
        args.setItem(0, Py::String(msg.pDocName ? msg.pDocName : ""));
        args.setItem(1, Py::String(msg.pObjectName ? msg.pObjectName : ""));
        args.setItem(2, Py::String(msg.pSubName ? msg.pSubName : ""));
        Py::Tuple tuple(3);
        tuple[0] = Py::Float(msg.x);
        tuple[1] = Py::Float(msg.y);
        tuple[2] = Py::Float(msg.z);
        args.setItem(3, tuple);
        Base::pyCall(py_addSelection.ptr(),args.ptr());
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void SelectionObserverPython::removeSelection(const SelectionChanges& msg)
{
    if(py_removeSelection.isNone())
        return;
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(3);
        args.setItem(0, Py::String(msg.pDocName ? msg.pDocName : ""));
        args.setItem(1, Py::String(msg.pObjectName ? msg.pObjectName : ""));
        args.setItem(2, Py::String(msg.pSubName ? msg.pSubName : ""));
        Base::pyCall(py_removeSelection.ptr(),args.ptr());
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void SelectionObserverPython::setSelection(const SelectionChanges& msg)
{
    if(py_setSelection.isNone())
        return;
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0, Py::String(msg.pDocName ? msg.pDocName : ""));
        Base::pyCall(py_setSelection.ptr(),args.ptr());
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void SelectionObserverPython::clearSelection(const SelectionChanges& msg)
{
    if(py_clearSelection.isNone())
        return;
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0, Py::String(msg.pDocName ? msg.pDocName : ""));
        Base::pyCall(py_clearSelection.ptr(),args.ptr());
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void SelectionObserverPython::setPreselection(const SelectionChanges& msg)
{
    if(py_setPreselection.isNone())
        return;
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(3);
        args.setItem(0, Py::String(msg.pDocName ? msg.pDocName : ""));
        args.setItem(1, Py::String(msg.pObjectName ? msg.pObjectName : ""));
        args.setItem(2, Py::String(msg.pSubName ? msg.pSubName : ""));
        Base::pyCall(py_setPreselection.ptr(),args.ptr());
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void SelectionObserverPython::removePreselection(const SelectionChanges& msg)
{
    if(py_removePreselection.isNone())
        return;
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(3);
        args.setItem(0, Py::String(msg.pDocName ? msg.pDocName : ""));
        args.setItem(1, Py::String(msg.pObjectName ? msg.pObjectName : ""));
        args.setItem(2, Py::String(msg.pSubName ? msg.pSubName : ""));
        Base::pyCall(py_removePreselection.ptr(),args.ptr());
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}
