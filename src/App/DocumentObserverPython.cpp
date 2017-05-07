/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include "DocumentObject.h"
#include "DocumentObserverPython.h"
#include <Base/Interpreter.h>
#include <Base/Console.h>

using namespace App;

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
    this->connectApplicationCreatedDocument = App::GetApplication().signalNewDocument.connect(boost::bind
        (&DocumentObserverPython::slotCreatedDocument, this, _1));
    this->connectApplicationDeletedDocument = App::GetApplication().signalDeleteDocument.connect(boost::bind
        (&DocumentObserverPython::slotDeletedDocument, this, _1));
    this->connectApplicationRelabelDocument = App::GetApplication().signalRelabelDocument.connect(boost::bind
        (&DocumentObserverPython::slotRelabelDocument, this, _1));
    this->connectApplicationActivateDocument = App::GetApplication().signalActiveDocument.connect(boost::bind
        (&DocumentObserverPython::slotActivateDocument, this, _1));
    this->connectApplicationUndoDocument = App::GetApplication().signalUndoDocument.connect(boost::bind
        (&DocumentObserverPython::slotUndoDocument, this, _1));
    this->connectApplicationRedoDocument = App::GetApplication().signalRedoDocument.connect(boost::bind
        (&DocumentObserverPython::slotRedoDocument, this, _1));

    this->connectDocumentCreatedObject = App::GetApplication().signalNewObject.connect(boost::bind
        (&DocumentObserverPython::slotCreatedObject, this, _1));
    this->connectDocumentDeletedObject = App::GetApplication().signalDeletedObject.connect(boost::bind
        (&DocumentObserverPython::slotDeletedObject, this, _1));
    this->connectDocumentChangedObject = App::GetApplication().signalChangedObject.connect(boost::bind
        (&DocumentObserverPython::slotChangedObject, this, _1, _2));

    this->connectDocActiveContainer = App::GetApplication().signalDocActiveContainer.connect(boost::bind
        (&DocumentObserverPython::slotDocActiveContainer, this, _1, _2, _3));
    this->connectAppActiveContainer = App::GetApplication().signalActiveContainer.connect(boost::bind
        (&DocumentObserverPython::slotAppActiveContainer, this, _1, _2));
}

DocumentObserverPython::~DocumentObserverPython()
{
    this->connectApplicationCreatedDocument.disconnect();
    this->connectApplicationDeletedDocument.disconnect();
    this->connectApplicationRelabelDocument.disconnect();
    this->connectApplicationActivateDocument.disconnect();
    this->connectApplicationUndoDocument.disconnect();
    this->connectApplicationRedoDocument.disconnect();

    this->connectDocumentCreatedObject.disconnect();
    this->connectDocumentDeletedObject.disconnect();
    this->connectDocumentChangedObject.disconnect();

    this->connectDocActiveContainer.disconnect();
    this->connectAppActiveContainer.disconnect();
}

void DocumentObserverPython::slotCreatedDocument(const App::Document& Doc)
{
    Base::PyGILStateLocker lock;
    try {
        if (this->inst.hasAttr(std::string("slotCreatedDocument"))) {
            Py::Callable method(this->inst.getAttr(std::string("slotCreatedDocument")));
            Py::Tuple args(1);
            args.setItem(0, Py::Object(const_cast<App::Document&>(Doc).getPyObject(), true));
            method.apply(args);
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void DocumentObserverPython::slotDeletedDocument(const App::Document& Doc)
{
    Base::PyGILStateLocker lock;
    try {
        if (this->inst.hasAttr(std::string("slotDeletedDocument"))) {
            Py::Callable method(this->inst.getAttr(std::string("slotDeletedDocument")));
            Py::Tuple args(1);
            args.setItem(0, Py::Object(const_cast<App::Document&>(Doc).getPyObject(), true));
            method.apply(args);
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void DocumentObserverPython::slotRelabelDocument(const App::Document& Doc)
{
    Base::PyGILStateLocker lock;
    try {
        if (this->inst.hasAttr(std::string("slotRelabelDocument"))) {
            Py::Callable method(this->inst.getAttr(std::string("slotRelabelDocument")));
            Py::Tuple args(1);
            args.setItem(0, Py::Object(const_cast<App::Document&>(Doc).getPyObject(), true));
            method.apply(args);
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void DocumentObserverPython::slotActivateDocument(const App::Document& Doc)
{
    Base::PyGILStateLocker lock;
    try {
        if (this->inst.hasAttr(std::string("slotActivateDocument"))) {
            Py::Callable method(this->inst.getAttr(std::string("slotActivateDocument")));
            Py::Tuple args(1);
            args.setItem(0, Py::Object(const_cast<App::Document&>(Doc).getPyObject(), true));
            method.apply(args);
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void DocumentObserverPython::slotUndoDocument(const App::Document& Doc)
{
    Base::PyGILStateLocker lock;
    try {
        if (this->inst.hasAttr(std::string("slotUndoDocument"))) {
            Py::Callable method(this->inst.getAttr(std::string("slotUndoDocument")));
            Py::Tuple args(1);
            args.setItem(0, Py::Object(const_cast<App::Document&>(Doc).getPyObject(), true));
            method.apply(args);
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void DocumentObserverPython::slotRedoDocument(const App::Document& Doc)
{
    Base::PyGILStateLocker lock;
    try {
        if (this->inst.hasAttr(std::string("slotRedoDocument"))) {
            Py::Callable method(this->inst.getAttr(std::string("slotRedoDocument")));
            Py::Tuple args(1);
            args.setItem(0, Py::Object(const_cast<App::Document&>(Doc).getPyObject(), true));
            method.apply(args);
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void DocumentObserverPython::slotDocActiveContainer(App::Document* doc, App::Container newContainer, App::Container oldContainer)
{
    Base::PyGILStateLocker lock;
    try {
        if (this->inst.hasAttr(std::string("slotDocActiveContainer"))) {
            Py::Callable method(this->inst.getAttr(std::string("slotDocActiveContainer")));
            Py::Tuple args(3);
            if (doc)
                args.setItem(0, Py::Object(doc->getPyObject(), true));
            args.setItem(1, Py::Object(newContainer.getPyObject(), true));
            args.setItem(2, Py::Object(oldContainer.getPyObject(), true));
            method.apply(args);
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void DocumentObserverPython::slotAppActiveContainer(App::Container newContainer, App::Container oldContainer)
{
    Base::PyGILStateLocker lock;
    try {
        if (this->inst.hasAttr(std::string("slotAppActiveContainer"))) {
            Py::Callable method(this->inst.getAttr(std::string("slotAppActiveContainer")));
            Py::Tuple args(2);
            args.setItem(0, Py::Object(newContainer.getPyObject(), true));
            args.setItem(1, Py::Object(oldContainer.getPyObject(), true));
            method.apply(args);
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void DocumentObserverPython::slotCreatedObject(const App::DocumentObject& Obj)
{
    Base::PyGILStateLocker lock;
    try {
        if (this->inst.hasAttr(std::string("slotCreatedObject"))) {
            Py::Callable method(this->inst.getAttr(std::string("slotCreatedObject")));
            Py::Tuple args(1);
            args.setItem(0, Py::Object(const_cast<App::DocumentObject&>(Obj).getPyObject(), true));
            method.apply(args);
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void DocumentObserverPython::slotDeletedObject(const App::DocumentObject& Obj)
{
    Base::PyGILStateLocker lock;
    try {
        if (this->inst.hasAttr(std::string("slotDeletedObject"))) {
            Py::Callable method(this->inst.getAttr(std::string("slotDeletedObject")));
            Py::Tuple args(1);
            args.setItem(0, Py::Object(const_cast<App::DocumentObject&>(Obj).getPyObject(), true));
            method.apply(args);
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void DocumentObserverPython::slotChangedObject(const App::DocumentObject& Obj,
                                               const App::Property& Prop)
{
    Base::PyGILStateLocker lock;
    try {
        if (this->inst.hasAttr(std::string("slotChangedObject"))) {
            Py::Callable method(this->inst.getAttr(std::string("slotChangedObject")));
            Py::Tuple args(2);
            args.setItem(0, Py::Object(const_cast<App::DocumentObject&>(Obj).getPyObject(), true));
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
