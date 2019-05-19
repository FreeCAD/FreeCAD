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
#define signalCreatedDocument signalNewDocument
#define signalCreatedObject signalNewObject
#define signalRecomputedObject signalObjectRecomputed
#define signalRecomputedDocument signalRecomputed
#define signalActivateDocument signalActiveDocument
#define signalDeletedDocument signalDeleteDocument

#undef FC_PY_ELEMENT
#define FC_PY_ELEMENT(_name,...) do{\
        FC_PY_GetCallable(obj.ptr(),"slot" #_name, py##_name);\
        if(!py##_name.isNone())\
            connect##_name = App::GetApplication().signal##_name.connect(\
                    boost::bind(&DocumentObserverPython::slot##_name, this, ##__VA_ARGS__));\
    }while(0);

    FC_PY_DOC_OBSERVER
}

DocumentObserverPython::~DocumentObserverPython()
{
#undef FC_PY_ELEMENT
#define FC_PY_ELEMENT(_name,...) connect##_name.disconnect();
    FC_PY_DOC_OBSERVER
}

void DocumentObserverPython::slotCreatedDocument(const App::Document& Doc)
{
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0, Py::Object(const_cast<App::Document&>(Doc).getPyObject(), true));
        Base::pyCall(pyCreatedDocument.ptr(),args.ptr());
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
        Py::Tuple args(1);
        args.setItem(0, Py::Object(const_cast<App::Document&>(Doc).getPyObject(), true));
        Base::pyCall(pyDeletedDocument.ptr(),args.ptr());
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
        Py::Tuple args(1);
        args.setItem(0, Py::Object(const_cast<App::Document&>(Doc).getPyObject(), true));
        Base::pyCall(pyRelabelDocument.ptr(),args.ptr());
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
        Py::Tuple args(1);
        args.setItem(0, Py::Object(const_cast<App::Document&>(Doc).getPyObject(), true));
        Base::pyCall(pyActivateDocument.ptr(),args.ptr());
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
        Py::Tuple args(1);
        args.setItem(0, Py::Object(const_cast<App::Document&>(Doc).getPyObject(), true));
        Base::pyCall(pyUndoDocument.ptr(),args.ptr());
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
        Py::Tuple args(1);
        args.setItem(0, Py::Object(const_cast<App::Document&>(Doc).getPyObject(), true));
        Base::pyCall(pyRedoDocument.ptr(),args.ptr());
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void DocumentObserverPython::slotUndo()
{
    Base::PyGILStateLocker lock;
    try {
        Base::pyCall(pyUndo.ptr());
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void DocumentObserverPython::slotRedo()
{
    Base::PyGILStateLocker lock;
    try {
        Base::pyCall(pyRedo.ptr());
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void DocumentObserverPython::slotBeforeCloseTransaction(bool abort)
{
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0, Py::Boolean(abort));
        Base::pyCall(pyBeforeCloseTransaction.ptr(),args.ptr());
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void DocumentObserverPython::slotCloseTransaction(bool abort)
{
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0, Py::Boolean(abort));
        Base::pyCall(pyCloseTransaction.ptr(),args.ptr());
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
        Py::Tuple args(2);
        args.setItem(0, Py::Object(const_cast<App::Document&>(Doc).getPyObject(), true));
        // If a property is touched but not part of a document object then its name is null.
        // In this case the slot function must not be called.
        const char* prop_name = Doc.getPropertyName(&Prop);
        if (prop_name) {
            args.setItem(1, Py::String(prop_name));
            Base::pyCall(pyBeforeChangeDocument.ptr(),args.ptr());
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
        Py::Tuple args(2);
        args.setItem(0, Py::Object(const_cast<App::Document&>(Doc).getPyObject(), true));
        // If a property is touched but not part of a document object then its name is null.
        // In this case the slot function must not be called.
        const char* prop_name = Doc.getPropertyName(&Prop);
        if (prop_name) {
            args.setItem(1, Py::String(prop_name));
            Base::pyCall(pyChangedDocument.ptr(),args.ptr());
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
        Py::Tuple args(1);
        args.setItem(0, Py::Object(const_cast<App::DocumentObject&>(Obj).getPyObject(), true));
        Base::pyCall(pyCreatedObject.ptr(),args.ptr());
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
        Py::Tuple args(1);
        args.setItem(0, Py::Object(const_cast<App::DocumentObject&>(Obj).getPyObject(), true));
        Base::pyCall(pyDeletedObject.ptr(),args.ptr());
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
        Py::Tuple args(2);
        args.setItem(0, Py::Object(const_cast<App::DocumentObject&>(Obj).getPyObject(), true));
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

void DocumentObserverPython::slotChangedObject(const App::DocumentObject& Obj,
                                               const App::Property& Prop)
{
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(2);
        args.setItem(0, Py::Object(const_cast<App::DocumentObject&>(Obj).getPyObject(), true));
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

void DocumentObserverPython::slotRecomputedObject(const App::DocumentObject& Obj)
{
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0, Py::Object(const_cast<App::DocumentObject&>(Obj).getPyObject(), true));
        Base::pyCall(pyRecomputedObject.ptr(),args.ptr());
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
        Py::Tuple args(1);
        args.setItem(0, Py::Object(const_cast<App::Document&>(doc).getPyObject(), true));
        Base::pyCall(pyRecomputedDocument.ptr(),args.ptr());
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void DocumentObserverPython::slotBeforeRecomputeDocument(const App::Document& doc)
{
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0, Py::Object(const_cast<App::Document&>(doc).getPyObject(), true));
        Base::pyCall(pyBeforeRecomputeDocument.ptr(),args.ptr());
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
        Py::Tuple args(2);
        args.setItem(0, Py::Object(const_cast<App::Document&>(doc).getPyObject(), true));
        args.setItem(1, Py::String(str));
        Base::pyCall(pyOpenTransaction.ptr(),args.ptr());
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
        Py::Tuple args(1);
        args.setItem(0, Py::Object(const_cast<App::Document&>(doc).getPyObject(), true));
        Base::pyCall(pyCommitTransaction.ptr(),args.ptr());
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
        Py::Tuple args(1);
        args.setItem(0, Py::Object(const_cast<App::Document&>(doc).getPyObject(), true));
        Base::pyCall(pyAbortTransaction.ptr(),args.ptr());
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
        auto container = Prop.getContainer();
        Py::Tuple args(2);
        args.setItem(0, Py::Object(container->getPyObject(), true));
        // If a property is touched but not part of a document object then its name is null.
        // In this case the slot function must not be called.
        const char* prop_name = container->getPropertyName(&Prop);
        if (prop_name) {
            args.setItem(1, Py::String(prop_name));
            Base::pyCall(pyAppendDynamicProperty.ptr(),args.ptr());
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
        auto container = Prop.getContainer();
        Py::Tuple args(2);
        args.setItem(0, Py::Object(container->getPyObject(), true));
        // If a property is touched but not part of a document object then its name is null.
        // In this case the slot function must not be called.
        const char* prop_name = container->getPropertyName(&Prop);
        if (prop_name) {
            args.setItem(1, Py::String(prop_name));
            Base::pyCall(pyRemoveDynamicProperty.ptr(),args.ptr());
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }    
}

void DocumentObserverPython::slotChangePropertyEditor(const App::Document &, const App::Property& Prop)
{
    Base::PyGILStateLocker lock;
    try {
        auto container = Prop.getContainer();
        Py::Tuple args(2);
        args.setItem(0, Py::Object(container->getPyObject(), true));
        // If a property is touched but not part of a document object then its name is null.
        // In this case the slot function must not be called.
        const char* prop_name = container->getPropertyName(&Prop);
        if (prop_name) {
            args.setItem(1, Py::String(prop_name));
            Base::pyCall(pyChangePropertyEditor.ptr(),args.ptr());
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
        Py::Tuple args(2);
        args.setItem(0, Py::Object(const_cast<App::Document&>(doc).getPyObject(), true));
        args.setItem(1, Py::String(file));
        Base::pyCall(pyStartSaveDocument.ptr(),args.ptr());
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
        Py::Tuple args(2);
        args.setItem(0, Py::Object(const_cast<App::Document&>(doc).getPyObject(), true));
        args.setItem(1, Py::String(file));
        Base::pyCall(pyFinishSaveDocument.ptr(),args.ptr());
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}
