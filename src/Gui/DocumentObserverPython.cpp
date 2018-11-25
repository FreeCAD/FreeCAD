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

#ifndef _PreComp_
#endif

#include "Application.h"
#include "Document.h"
#include "ViewProvider.h"
#include "ViewProviderDocumentObject.h"
#include "DocumentObserverPython.h"
#include <Base/Interpreter.h>
#include <Base/Console.h>

using namespace Gui;

std::vector<DocumentObserverPython*> DocumentObserverPython::_instances;

void DocumentObserverPython::addObserver(const Py::Object& obj)
{
    _instances.push_back(new DocumentObserverPython(obj));
}

void DocumentObserverPython::removeObserver(const Py::Object& obj)
{
    DocumentObserverPython* obs=0;
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
    this->connectApplicationCreatedDocument = Gui::Application::Instance->signalNewDocument.connect(boost::bind
        (&DocumentObserverPython::slotCreatedDocument, this, _1));
    this->connectApplicationDeletedDocument = Gui::Application::Instance->signalDeleteDocument.connect(boost::bind
        (&DocumentObserverPython::slotDeletedDocument, this, _1));
    this->connectApplicationRelabelDocument = Gui::Application::Instance->signalRelabelDocument.connect(boost::bind
        (&DocumentObserverPython::slotRelabelDocument, this, _1));
    this->connectApplicationRenameDocument = Gui::Application::Instance->signalRenameDocument.connect(boost::bind
        (&DocumentObserverPython::slotRelabelDocument, this, _1));
    this->connectApplicationActivateDocument = Gui::Application::Instance->signalActiveDocument.connect(boost::bind
        (&DocumentObserverPython::slotActivateDocument, this, _1));

    this->connectDocumentCreatedObject = Gui::Application::Instance->signalNewObject.connect(boost::bind
        (&DocumentObserverPython::slotCreatedObject, this, _1));
    this->connectDocumentDeletedObject = Gui::Application::Instance->signalDeletedObject.connect(boost::bind
        (&DocumentObserverPython::slotDeletedObject, this, _1));
    this->connectDocumentChangedObject = Gui::Application::Instance->signalChangedObject.connect(boost::bind
        (&DocumentObserverPython::slotChangedObject, this, _1, _2));
    
    this->connectDocumentObjectInEdit = Gui::Application::Instance->signalInEdit.connect(boost::bind
        (&DocumentObserverPython::slotInEdit, this, _1));
    this->connectDocumentObjectResetEdit = Gui::Application::Instance->signalResetEdit.connect(boost::bind
        (&DocumentObserverPython::slotResetEdit, this, _1));
    
}

DocumentObserverPython::~DocumentObserverPython()
{
    this->connectApplicationCreatedDocument.disconnect();
    this->connectApplicationDeletedDocument.disconnect();
    this->connectApplicationRelabelDocument.disconnect();
    this->connectApplicationRenameDocument.disconnect();
    this->connectApplicationActivateDocument.disconnect();

    this->connectDocumentCreatedObject.disconnect();
    this->connectDocumentDeletedObject.disconnect();
    this->connectDocumentChangedObject.disconnect();
    
    this->connectDocumentObjectInEdit.disconnect();
    this->connectDocumentObjectResetEdit.disconnect();
}

void DocumentObserverPython::slotCreatedDocument(const Gui::Document& Doc)
{
    Base::PyGILStateLocker lock;
    try {
        if (this->inst.hasAttr(std::string("slotCreatedDocument"))) {
            Py::Callable method(this->inst.getAttr(std::string("slotCreatedDocument")));
            Py::Tuple args(1);
            args.setItem(0, Py::Object(const_cast<Gui::Document&>(Doc).getPyObject(), true));
            method.apply(args);
        }
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
        if (this->inst.hasAttr(std::string("slotDeletedDocument"))) {
            Py::Callable method(this->inst.getAttr(std::string("slotDeletedDocument")));
            Py::Tuple args(1);
            args.setItem(0, Py::Object(const_cast<Gui::Document&>(Doc).getPyObject(), true));
            method.apply(args);
        }
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
        if (this->inst.hasAttr(std::string("slotRelabelDocument"))) {
            Py::Callable method(this->inst.getAttr(std::string("slotRelabelDocument")));
            Py::Tuple args(1);
            args.setItem(0, Py::Object(const_cast<Gui::Document&>(Doc).getPyObject(), true));
            method.apply(args);
        }
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
        if (this->inst.hasAttr(std::string("slotRenameDocument"))) {
            Py::Callable method(this->inst.getAttr(std::string("slotRenameDocument")));
            Py::Tuple args(1);
            args.setItem(0, Py::Object(const_cast<Gui::Document&>(Doc).getPyObject(), true));
            method.apply(args);
        }
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
        if (this->inst.hasAttr(std::string("slotActivateDocument"))) {
            Py::Callable method(this->inst.getAttr(std::string("slotActivateDocument")));
            Py::Tuple args(1);
            args.setItem(0, Py::Object(const_cast<Gui::Document&>(Doc).getPyObject(), true));
            method.apply(args);
        }
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
        if (this->inst.hasAttr(std::string("slotCreatedObject"))) {
            Py::Callable method(this->inst.getAttr(std::string("slotCreatedObject")));
            Py::Tuple args(1);
            args.setItem(0, Py::Object(const_cast<Gui::ViewProvider&>(Obj).getPyObject(), true));
            method.apply(args);
        }
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
        if (this->inst.hasAttr(std::string("slotDeletedObject"))) {
            Py::Callable method(this->inst.getAttr(std::string("slotDeletedObject")));
            Py::Tuple args(1);
            args.setItem(0, Py::Object(const_cast<Gui::ViewProvider&>(Obj).getPyObject(), true));
            method.apply(args);
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
        if (this->inst.hasAttr(std::string("slotChangedObject"))) {
            Py::Callable method(this->inst.getAttr(std::string("slotChangedObject")));
            Py::Tuple args(2);
            args.setItem(0, Py::Object(const_cast<Gui::ViewProvider&>(Obj).getPyObject(), true));
            // If a property is touched but not part of a document object then its name is null.
            // In this case the slot function must not be called.
            const char* prop_name = Obj.getPropertyName(&Prop);
            if (prop_name) {
                args.setItem(1, Py::String(prop_name));
                method.apply(args);
            }
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
        if (this->inst.hasAttr(std::string("slotInEdit"))) {
            Py::Callable method(this->inst.getAttr(std::string("slotInEdit")));
            Py::Tuple args(1);
            args.setItem(0, Py::Object(const_cast<Gui::ViewProviderDocumentObject&>(Obj).getPyObject(), true));
            method.apply(args);
        }
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
        if (this->inst.hasAttr(std::string("slotResetEdit"))) {
            Py::Callable method(this->inst.getAttr(std::string("slotResetEdit")));
            Py::Tuple args(1);
            args.setItem(0, Py::Object(const_cast<Gui::ViewProviderDocumentObject&>(Obj).getPyObject(), true));
            method.apply(args);
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}
