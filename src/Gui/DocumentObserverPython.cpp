/***************************************************************************
 *   Copyright (c) 2018 Stefan Tröger <stefantroeger@gmx.net>              *
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

#include <Base/Interpreter.h>

#include "Application.h"
#include "Document.h"
#include "DocumentObserverPython.h"
#include "ViewProvider.h"
#include "ViewProviderDocumentObject.h"


using namespace Gui;
namespace sp = std::placeholders;

std::vector<DocumentObserverPython*> DocumentObserverPython::_instances;

void DocumentObserverPython::addObserver(const Py::Object& obj)
{
    _instances.push_back(new DocumentObserverPython(obj));
}

void DocumentObserverPython::removeObserver(const Py::Object& obj)
{
    DocumentObserverPython* obs=nullptr;
    for (std::vector<DocumentObserverPython*>::iterator it =
        _instances.begin(); it != _instances.end(); ++it) {
        if ((*it)->inst == obj) {
            obs = *it;
            _instances.erase(it);
            break;
        }
    }

    delete obs;
}

DocumentObserverPython::DocumentObserverPython(const Py::Object& obj) : inst(obj)
{
    //NOLINTBEGIN
#define FC_PY_ELEMENT_ARG1(_name1, _name2) do {\
        FC_PY_GetCallable(obj.ptr(), "slot" #_name1, py##_name1.py);\
        if (!py##_name1.py.isNone())\
            py##_name1.slot = Application::Instance->signal##_name2.connect(\
                    std::bind(&DocumentObserverPython::slot##_name1, this, sp::_1));\
    }\
    while(0);

#define FC_PY_ELEMENT_ARG2(_name1, _name2) do {\
        FC_PY_GetCallable(obj.ptr(), "slot" #_name1, py##_name1.py);\
        if (!py##_name1.py.isNone())\
            py##_name1.slot = Application::Instance->signal##_name2.connect(\
                    std::bind(&DocumentObserverPython::slot##_name1, this, sp::_1, sp::_2));\
    }\
    while(0);

    FC_PY_ELEMENT_ARG1(CreatedDocument, NewDocument)
    FC_PY_ELEMENT_ARG1(DeletedDocument, DeleteDocument)
    FC_PY_ELEMENT_ARG1(RelabelDocument, RelabelDocument)
    FC_PY_ELEMENT_ARG1(RenameDocument, RenameDocument)
    FC_PY_ELEMENT_ARG1(ActivateDocument, ActiveDocument)
    FC_PY_ELEMENT_ARG1(CreatedObject, NewObject)
    FC_PY_ELEMENT_ARG1(DeletedObject, DeletedObject)
    FC_PY_ELEMENT_ARG2(BeforeChangeObject, BeforeChangeObject)
    FC_PY_ELEMENT_ARG2(ChangedObject, ChangedObject)
    FC_PY_ELEMENT_ARG1(InEdit, InEdit)
    FC_PY_ELEMENT_ARG1(ResetEdit, ResetEdit)
    //NOLINTEND
}

DocumentObserverPython::~DocumentObserverPython() = default;

void DocumentObserverPython::slotCreatedDocument(const Gui::Document& Doc)
{
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0, Py::asObject(const_cast<Gui::Document&>(Doc).getPyObject()));
        Base::pyCall(pyCreatedDocument.ptr(),args.ptr());
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void DocumentObserverPython::slotDeletedDocument(const Gui::Document& Doc)
{
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0, Py::asObject(const_cast<Gui::Document&>(Doc).getPyObject()));
        Base::pyCall(pyDeletedDocument.ptr(),args.ptr());
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void DocumentObserverPython::slotRelabelDocument(const Gui::Document& Doc)
{
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0, Py::asObject(const_cast<Gui::Document&>(Doc).getPyObject()));
        Base::pyCall(pyRelabelDocument.ptr(),args.ptr());
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void DocumentObserverPython::slotRenameDocument(const Gui::Document& Doc)
{
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0, Py::asObject(const_cast<Gui::Document&>(Doc).getPyObject()));
        Base::pyCall(pyRenameDocument.ptr(),args.ptr());
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void DocumentObserverPython::slotActivateDocument(const Gui::Document& Doc)
{
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0, Py::asObject(const_cast<Gui::Document&>(Doc).getPyObject()));
        Base::pyCall(pyActivateDocument.ptr(),args.ptr());
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void DocumentObserverPython::slotCreatedObject(const Gui::ViewProvider& Obj)
{
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0, Py::asObject(const_cast<Gui::ViewProvider&>(Obj).getPyObject()));
        Base::pyCall(pyCreatedObject.ptr(),args.ptr());
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void DocumentObserverPython::slotDeletedObject(const Gui::ViewProvider& Obj)
{
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0, Py::asObject(const_cast<Gui::ViewProvider&>(Obj).getPyObject()));
        Base::pyCall(pyDeletedObject.ptr(),args.ptr());
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void DocumentObserverPython::slotBeforeChangeObject(const Gui::ViewProvider& Obj,
                                                    const App::Property& Prop)
{
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(2);
        args.setItem(0, Py::asObject(const_cast<Gui::ViewProvider&>(Obj).getPyObject()));
        // If a property is touched but not part of a document object then its name is null.
        // In this case the slot function must not be called.
        const char* prop_name = Obj.getPropertyName(&Prop);
        if (prop_name) {
            args.setItem(1, Py::String(prop_name));
            Base::pyCall(pyBeforeChangeObject.ptr(),args.ptr());
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void DocumentObserverPython::slotChangedObject(const Gui::ViewProvider& Obj,
                                               const App::Property& Prop)
{
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(2);
        args.setItem(0, Py::asObject(const_cast<Gui::ViewProvider&>(Obj).getPyObject()));
        // If a property is touched but not part of a document object then its name is null.
        // In this case the slot function must not be called.
        const char* prop_name = Obj.getPropertyName(&Prop);
        if (prop_name) {
            args.setItem(1, Py::String(prop_name));
            Base::pyCall(pyChangedObject.ptr(),args.ptr());
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void DocumentObserverPython::slotInEdit(const Gui::ViewProviderDocumentObject& Obj)
{
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0, Py::asObject(const_cast<Gui::ViewProviderDocumentObject&>(Obj).getPyObject()));
        Base::pyCall(pyInEdit.ptr(),args.ptr());
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void DocumentObserverPython::slotResetEdit(const Gui::ViewProviderDocumentObject& Obj)
{
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0, Py::asObject(const_cast<Gui::ViewProviderDocumentObject&>(Obj).getPyObject()));
        Base::pyCall(pyResetEdit.ptr(),args.ptr());
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}
