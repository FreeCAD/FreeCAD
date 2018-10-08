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

    this->connectDocumentBeforeChange = App::GetApplication().signalBeforeChangeDocument.connect(boost::bind
        (&DocumentObserverPython::slotBeforeChangeDocument, this, _1, _2));
    this->connectDocumentChanged = App::GetApplication().signalChangedDocument.connect(boost::bind
        (&DocumentObserverPython::slotChangedDocument, this, _1, _2));
    this->connectDocumentCreatedObject = App::GetApplication().signalNewObject.connect(boost::bind
        (&DocumentObserverPython::slotCreatedObject, this, _1));
    this->connectDocumentDeletedObject = App::GetApplication().signalDeletedObject.connect(boost::bind
        (&DocumentObserverPython::slotDeletedObject, this, _1));
    this->connectDocumentBeforeChangeObject = App::GetApplication().signalBeforeChangeObject.connect(boost::bind
        (&DocumentObserverPython::slotBeforeChangeObject, this, _1, _2));
    this->connectDocumentChangedObject = App::GetApplication().signalChangedObject.connect(boost::bind
        (&DocumentObserverPython::slotChangedObject, this, _1, _2));

    this->connectDocumentObjectRecomputed = App::GetApplication().signalObjectRecomputed.connect(boost::bind
        (&DocumentObserverPython::slotRecomputedObject, this, _1));
    this->connectDocumentRecomputed = App::GetApplication().signalRecomputed.connect(boost::bind
        (&DocumentObserverPython::slotRecomputedDocument, this, _1));

    this->connectDocumentOpenTransaction = App::GetApplication().signalOpenTransaction.connect(boost::bind
        (&DocumentObserverPython::slotOpenTransaction, this, _1, _2));
    this->connectDocumentCommitTransaction = App::GetApplication().signalCommitTransaction.connect(boost::bind
        (&DocumentObserverPython::slotCommitTransaction, this, _1));
    this->connectDocumentAbortTransaction = App::GetApplication().signalAbortTransaction.connect(boost::bind
        (&DocumentObserverPython::slotAbortTransaction, this, _1));

    this->connectDocumentStartSave = App::GetApplication().signalStartSaveDocument.connect(boost::bind
        (&DocumentObserverPython::slotStartSaveDocument, this, _1, _2));
    this->connectDocumentFinishSave = App::GetApplication().signalFinishSaveDocument.connect(boost::bind
        (&DocumentObserverPython::slotFinishSaveDocument, this, _1, _2));

    this->connectObjectAppendDynamicProperty = App::GetApplication().signalAppendDynamicProperty.connect(boost::bind
        (&DocumentObserverPython::slotAppendDynamicProperty, this, _1));
    this->connectObjectRemoveDynamicProperty = App::GetApplication().signalRemoveDynamicProperty.connect(boost::bind
        (&DocumentObserverPython::slotRemoveDynamicProperty, this, _1));
    this->connectObjectChangePropertyEditor = App::GetApplication().signalChangePropertyEditor.connect(boost::bind
        (&DocumentObserverPython::slotChangePropertyEditor, this, _1));
}

DocumentObserverPython::~DocumentObserverPython()
{
    this->connectApplicationCreatedDocument.disconnect();
    this->connectApplicationDeletedDocument.disconnect();
    this->connectApplicationRelabelDocument.disconnect();
    this->connectApplicationActivateDocument.disconnect();
    this->connectApplicationUndoDocument.disconnect();
    this->connectApplicationRedoDocument.disconnect();

    this->connectDocumentBeforeChange.disconnect();
    this->connectDocumentChanged.disconnect();
    this->connectDocumentCreatedObject.disconnect();
    this->connectDocumentDeletedObject.disconnect();
    this->connectDocumentBeforeChangeObject.disconnect();
    this->connectDocumentChangedObject.disconnect();
    this->connectDocumentObjectRecomputed.disconnect();
    this->connectDocumentRecomputed.disconnect();
    this->connectDocumentOpenTransaction.disconnect();
    this->connectDocumentCommitTransaction.disconnect();
    this->connectDocumentAbortTransaction.disconnect();
    this->connectDocumentStartSave.disconnect();
    this->connectDocumentFinishSave.disconnect();

    this->connectObjectAppendDynamicProperty.disconnect();
    this->connectObjectRemoveDynamicProperty.disconnect();
    this->connectObjectChangePropertyEditor.disconnect();
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

void DocumentObserverPython::slotBeforeChangeDocument(const App::Document& Doc, const App::Property& Prop) 
{   
    Base::PyGILStateLocker lock;
    try {
        if (this->inst.hasAttr(std::string("slotBeforeChangeDocument"))) {
            Py::Callable method(this->inst.getAttr(std::string("slotBeforeChangeDocument")));
            Py::Tuple args(2);
            args.setItem(0, Py::Object(const_cast<App::Document&>(Doc).getPyObject(), true));
            // If a property is touched but not part of a document object then its name is null.
            // In this case the slot function must not be called.
            const char* prop_name = Doc.getPropertyName(&Prop);
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

void DocumentObserverPython::slotChangedDocument(const App::Document& Doc, const App::Property& Prop) 
{
    Base::PyGILStateLocker lock;
    try {
        if (this->inst.hasAttr(std::string("slotChangedDocument"))) {
            Py::Callable method(this->inst.getAttr(std::string("slotChangedDocument")));
            Py::Tuple args(2);
            args.setItem(0, Py::Object(const_cast<App::Document&>(Doc).getPyObject(), true));
            // If a property is touched but not part of a document object then its name is null.
            // In this case the slot function must not be called.
            const char* prop_name = Doc.getPropertyName(&Prop);
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

void DocumentObserverPython::slotBeforeChangeObject(const App::DocumentObject& Obj,
                                               const App::Property& Prop)
{
    Base::PyGILStateLocker lock;
    try {
        if (this->inst.hasAttr(std::string("slotBeforeChangeObject"))) {
            Py::Callable method(this->inst.getAttr(std::string("slotBeforeChangeObject")));
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

void DocumentObserverPython::slotRecomputedObject(const App::DocumentObject& Obj)
{
    Base::PyGILStateLocker lock;
    try {
        if (this->inst.hasAttr(std::string("slotRecomputedObject"))) {
            Py::Callable method(this->inst.getAttr(std::string("slotRecomputedObject")));
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

void DocumentObserverPython::slotRecomputedDocument(const App::Document& doc)
{
    Base::PyGILStateLocker lock;
    try {
        if (this->inst.hasAttr(std::string("slotRecomputedDocument"))) {
            Py::Callable method(this->inst.getAttr(std::string("slotRecomputedDocument")));
            Py::Tuple args(1);
            args.setItem(0, Py::Object(const_cast<App::Document&>(doc).getPyObject(), true));
            method.apply(args);
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void DocumentObserverPython::slotOpenTransaction(const App::Document& doc, std::string str)
{
    Base::PyGILStateLocker lock;
    try {
        if (this->inst.hasAttr(std::string("slotOpenTransaction"))) {
            Py::Callable method(this->inst.getAttr(std::string("slotOpenTransaction")));
            Py::Tuple args(2);
            args.setItem(0, Py::Object(const_cast<App::Document&>(doc).getPyObject(), true));
            args.setItem(1, Py::String(str));
            method.apply(args);
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void DocumentObserverPython::slotCommitTransaction(const App::Document& doc)
{
    Base::PyGILStateLocker lock;
    try {
        if (this->inst.hasAttr(std::string("slotCommitTransaction"))) {
            Py::Callable method(this->inst.getAttr(std::string("slotCommitTransaction")));
            Py::Tuple args(1);
            args.setItem(0, Py::Object(const_cast<App::Document&>(doc).getPyObject(), true));
            method.apply(args);
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void DocumentObserverPython::slotAbortTransaction(const App::Document& doc)
{
    Base::PyGILStateLocker lock;
    try {
        if (this->inst.hasAttr(std::string("slotAbortTransaction"))) {
            Py::Callable method(this->inst.getAttr(std::string("slotAbortTransaction")));
            Py::Tuple args(1);
            args.setItem(0, Py::Object(const_cast<App::Document&>(doc).getPyObject(), true));
            method.apply(args);
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void DocumentObserverPython::slotAppendDynamicProperty(const App::Property& Prop)
{
    Base::PyGILStateLocker lock;
    try {
        if (this->inst.hasAttr(std::string("slotAppendDynamicProperty"))) {
            auto container = Prop.getContainer();
            Py::Callable method(this->inst.getAttr(std::string("slotAppendDynamicProperty")));
            Py::Tuple args(2);
            args.setItem(0, Py::Object(container->getPyObject(), true));
            // If a property is touched but not part of a document object then its name is null.
            // In this case the slot function must not be called.
            const char* prop_name = container->getPropertyName(&Prop);
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

void DocumentObserverPython::slotRemoveDynamicProperty(const App::Property& Prop)
{
    Base::PyGILStateLocker lock;
    try {
        if (this->inst.hasAttr(std::string("slotRemoveDynamicProperty"))) {
            auto container = Prop.getContainer();
            Py::Callable method(this->inst.getAttr(std::string("slotRemoveDynamicProperty")));
            Py::Tuple args(2);
            args.setItem(0, Py::Object(container->getPyObject(), true));
            // If a property is touched but not part of a document object then its name is null.
            // In this case the slot function must not be called.
            const char* prop_name = container->getPropertyName(&Prop);
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

void DocumentObserverPython::slotChangePropertyEditor(const App::Property& Prop)
{
    Base::PyGILStateLocker lock;
    try {
        if (this->inst.hasAttr(std::string("slotChangePropertyEditor"))) {
            auto container = Prop.getContainer();
            Py::Callable method(this->inst.getAttr(std::string("slotChangePropertyEditor")));
            Py::Tuple args(2);
            args.setItem(0, Py::Object(container->getPyObject(), true));
            // If a property is touched but not part of a document object then its name is null.
            // In this case the slot function must not be called.
            const char* prop_name = container->getPropertyName(&Prop);
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

void DocumentObserverPython::slotStartSaveDocument(const App::Document& doc, const std::string& file)
{
    Base::PyGILStateLocker lock;
    try {
        if (this->inst.hasAttr(std::string("slotStartSaveDocument"))) {
            Py::Callable method(this->inst.getAttr(std::string("slotStartSaveDocument")));
            Py::Tuple args(2);
            args.setItem(0, Py::Object(const_cast<App::Document&>(doc).getPyObject(), true));
            args.setItem(1, Py::String(file));
            method.apply(args);
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void DocumentObserverPython::slotFinishSaveDocument(const App::Document& doc, const std::string& file)
{
    Base::PyGILStateLocker lock;
    try {
        if (this->inst.hasAttr(std::string("slotFinishSaveDocument"))) {
            Py::Callable method(this->inst.getAttr(std::string("slotFinishSaveDocument")));
            Py::Tuple args(2);
            args.setItem(0, Py::Object(const_cast<App::Document&>(doc).getPyObject(), true));
            args.setItem(1, Py::String(file));
            method.apply(args);
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}
