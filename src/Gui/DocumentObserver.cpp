/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <sstream>
#endif

#include <functional>

#include "DocumentObserver.h"
#include "Application.h"
#include "Document.h"
#include "ViewProviderDocumentObject.h"
#include <App/Document.h>


using namespace Gui;
namespace sp = std::placeholders;


DocumentT::DocumentT() = default;

DocumentT::DocumentT(Document* doc)
{
    document = doc->getDocument()->getName();
}

DocumentT::DocumentT(const std::string& name)
{
    document = name;
}

DocumentT::DocumentT(const DocumentT& doc)
{
    document = doc.document;
}

DocumentT::~DocumentT() = default;

void DocumentT::operator=(const DocumentT& doc)
{
    if (this == &doc)
        return;
    document = doc.document;
}

void DocumentT::operator=(const Document* doc)
{
    document = doc->getDocument()->getName();
}

void DocumentT::operator=(const std::string& name)
{
    document = name;
}

Document* DocumentT::getDocument() const
{
    return Application::Instance->getDocument(document.c_str());
}

std::string DocumentT::getDocumentName() const
{
    return document;
}

std::string DocumentT::getGuiDocumentPython() const
{
    std::stringstream str;
    Document* doc = Application::Instance->activeDocument();
    if (doc && document == doc->getDocument()->getName()) {
        str << "Gui.ActiveDocument";
    }
    else {
        str << "Gui.getDocument(\""
            << document
            << "\")";
    }
    return str.str();
}

std::string DocumentT::getAppDocumentPython() const
{
    std::stringstream str;
    Document* doc = Application::Instance->activeDocument();
    if (doc && document == doc->getDocument()->getName()) {
        str << "App.ActiveDocument";
    }
    else {
        str << "App.getDocument(\""
            << document
            << "\")";
    }
    return str.str();
}

// -----------------------------------------------------------------------------

ViewProviderT::ViewProviderT() = default;

ViewProviderT::ViewProviderT(const ViewProviderT& other)
{
    *this = other;
}

ViewProviderT::ViewProviderT(ViewProviderT &&other)
{
    *this = std::move(other);
}

ViewProviderT::ViewProviderT(const ViewProviderDocumentObject* obj)
{
    *this = obj;
}

ViewProviderT::~ViewProviderT() = default;

ViewProviderT & ViewProviderT::operator=(const ViewProviderT& obj)
{
    if (this == &obj)
        return *this;
    object = obj.object;
    document = obj.document;
    return *this;
}

ViewProviderT &ViewProviderT::operator=(ViewProviderT&& obj)
{
    if (this == &obj)
        return *this;
    object = std::move(obj.object);
    document = std::move(obj.document);
    return *this;
}

void ViewProviderT::operator=(const ViewProviderDocumentObject* obj)
{
    if (!obj) {
        object.clear();
        document.clear();
    }
    else {
        object = obj->getObject()->getNameInDocument();
        document = obj->getObject()->getDocument()->getName();
    }
}

bool ViewProviderT::operator==(const ViewProviderT &other) const {
    return document == other.document
        && object == other.object;
}

Document* ViewProviderT::getDocument() const
{
    return Application::Instance->getDocument(document.c_str());
}

const std::string& ViewProviderT::getDocumentName() const
{
    return document;
}

std::string ViewProviderT::getGuiDocumentPython() const
{
    DocumentT doct(document);
    return doct.getGuiDocumentPython();
}

std::string ViewProviderT::getAppDocumentPython() const
{
    DocumentT doct(document);
    return doct.getAppDocumentPython();
}

ViewProviderDocumentObject* ViewProviderT::getViewProvider() const
{
    ViewProviderDocumentObject* obj = nullptr;
    Document* doc = getDocument();
    if (doc) {
        obj = dynamic_cast<ViewProviderDocumentObject*>(doc->getViewProviderByName(object.c_str()));
    }
    return obj;
}

const std::string& ViewProviderT::getObjectName() const
{
    return object;
}

std::string ViewProviderT::getObjectPython() const
{
    std::stringstream str;
    Document* doc = Application::Instance->activeDocument();
    if (doc && document == doc->getDocument()->getName()) {
        str << "Gui.ActiveDocument.";
    }
    else {
        str << "Gui.getDocument(\""
            << document
            << "\").";
    }

    str << object;
    return str.str();
}

// -----------------------------------------------------------------------------

class DocumentWeakPtrT::Private {
public:
    Private(Gui::Document* doc) : _document(doc) {
        if (doc) {
            //NOLINTBEGIN
            connectApplicationDeletedDocument = doc->signalDeleteDocument.connect(std::bind
                (&Private::deletedDocument, this, sp::_1));
            //NOLINTEND
        }
    }

    void deletedDocument(const Gui::Document& doc) {
        if (_document == &doc)
            reset();
    }
    void reset() {
        connectApplicationDeletedDocument.disconnect();
        _document = nullptr;
    }

    Gui::Document* _document;
    using Connection = boost::signals2::scoped_connection;
    Connection connectApplicationDeletedDocument;
};

DocumentWeakPtrT::DocumentWeakPtrT(Gui::Document* doc) noexcept
  : d(new Private(doc))
{
}

DocumentWeakPtrT::~DocumentWeakPtrT() = default;

void DocumentWeakPtrT::reset() noexcept
{
    d->reset();
}

bool DocumentWeakPtrT::expired() const noexcept
{
    return (d->_document == nullptr);
}

Gui::Document* DocumentWeakPtrT::operator*() const noexcept
{
    return d->_document;
}

Gui::Document* DocumentWeakPtrT::operator->() const noexcept
{
    return d->_document;
}

// -----------------------------------------------------------------------------

class ViewProviderWeakPtrT::Private {
public:
    Private(ViewProviderDocumentObject* obj) : object(obj) {
        set(obj);
    }
    void deletedDocument(const Gui::Document& doc) {
        // When deleting document then there is no way to undo it
        if (object && object->getDocument() == &doc) {
            reset();
        }
    }
    void createdObject(const Gui::ViewProvider& obj) noexcept {
        // When undoing the removal
        if (object == &obj) {
            indocument = true;
        }
    }
    void deletedObject(const Gui::ViewProvider& obj) noexcept {
        if (object == &obj) {
            indocument = false;
        }
    }
    void reset() {
        connectApplicationDeletedDocument.disconnect();
        connectDocumentCreatedObject.disconnect();
        connectDocumentDeletedObject.disconnect();
        object = nullptr;
        indocument = false;
    }
    void set(ViewProviderDocumentObject* obj) {
        object = obj;
        try {
            if (obj) {
                //NOLINTBEGIN
                Gui::Document* doc = obj->getDocument();
                indocument = true;
                connectApplicationDeletedDocument = doc->signalDeleteDocument.connect(std::bind
                    (&Private::deletedDocument, this, sp::_1));
                connectDocumentCreatedObject = doc->signalNewObject.connect(std::bind
                    (&Private::createdObject, this, sp::_1));
                connectDocumentDeletedObject = doc->signalDeletedObject.connect(std::bind
                    (&Private::deletedObject, this, sp::_1));
                //NOLINTEND
            }
        }
        catch (const Base::RuntimeError&) {
            // getDocument() may raise an exception
            object = nullptr;
            indocument = false;
        }
    }
    ViewProviderDocumentObject* get() const {
        return indocument ? object : nullptr;
    }

    Gui::ViewProviderDocumentObject* object;
    bool indocument{false};
    using Connection = boost::signals2::scoped_connection;
    Connection connectApplicationDeletedDocument;
    Connection connectDocumentCreatedObject;
    Connection connectDocumentDeletedObject;
};

ViewProviderWeakPtrT::ViewProviderWeakPtrT(ViewProviderDocumentObject* obj)
  : d(new Private(obj))
{
}

ViewProviderWeakPtrT::~ViewProviderWeakPtrT() = default;

ViewProviderDocumentObject* ViewProviderWeakPtrT::_get() const noexcept
{
    return d->get();
}

void ViewProviderWeakPtrT::reset()
{
    d->reset();
}

bool ViewProviderWeakPtrT::expired() const noexcept
{
    return !d->indocument;
}

ViewProviderWeakPtrT& ViewProviderWeakPtrT::operator= (ViewProviderDocumentObject* p)
{
    d->reset();
    d->set(p);
    return *this;
}

ViewProviderDocumentObject* ViewProviderWeakPtrT::operator*() const noexcept
{
    return d->get();
}

ViewProviderDocumentObject* ViewProviderWeakPtrT::operator->() const noexcept
{
    return d->get();
}

bool ViewProviderWeakPtrT::operator== (const ViewProviderWeakPtrT& p) const noexcept
{
    return d->get() == p.d->get();
}

bool ViewProviderWeakPtrT::operator!= (const ViewProviderWeakPtrT& p) const noexcept
{
    return d->get() != p.d->get();
}

// -----------------------------------------------------------------------------

DocumentObserver::DocumentObserver() = default;

DocumentObserver::DocumentObserver(Document* doc)
{
    attachDocument(doc);
}

DocumentObserver::~DocumentObserver() = default;

void DocumentObserver::attachDocument(Document* doc)
{
    detachDocument();

    if (!doc)
        return;

    //NOLINTBEGIN
    this->connectDocumentCreatedObject = doc->signalNewObject.connect(std::bind
        (&DocumentObserver::slotCreatedObject, this, sp::_1));
    this->connectDocumentDeletedObject = doc->signalDeletedObject.connect(std::bind
        (&DocumentObserver::slotDeletedObject, this, sp::_1));
    this->connectDocumentChangedObject = doc->signalChangedObject.connect(std::bind
        (&DocumentObserver::slotChangedObject, this, sp::_1, sp::_2));
    this->connectDocumentRelabelObject = doc->signalRelabelObject.connect(std::bind
        (&DocumentObserver::slotRelabelObject, this, sp::_1));
    this->connectDocumentActivateObject = doc->signalActivatedObject.connect(std::bind
        (&DocumentObserver::slotActivatedObject, this, sp::_1));
    this->connectDocumentEditObject = doc->signalInEdit.connect(std::bind
        (&DocumentObserver::slotEnterEditObject, this, sp::_1));
    this->connectDocumentResetObject = doc->signalResetEdit.connect(std::bind
        (&DocumentObserver::slotResetEditObject, this, sp::_1));
    this->connectDocumentUndo = doc->signalUndoDocument.connect(std::bind
        (&DocumentObserver::slotUndoDocument, this, sp::_1));
    this->connectDocumentRedo = doc->signalRedoDocument.connect(std::bind
        (&DocumentObserver::slotRedoDocument, this, sp::_1));
    this->connectDocumentDelete = doc->signalDeleteDocument.connect(std::bind
        (&DocumentObserver::slotDeleteDocument, this, sp::_1));
    //NOLINTEND
}

void DocumentObserver::detachDocument()
{
    this->connectDocumentCreatedObject.disconnect();
    this->connectDocumentDeletedObject.disconnect();
    this->connectDocumentChangedObject.disconnect();
    this->connectDocumentRelabelObject.disconnect();
    this->connectDocumentActivateObject.disconnect();
    this->connectDocumentEditObject.disconnect();
    this->connectDocumentResetObject.disconnect();
    this->connectDocumentUndo.disconnect();
    this->connectDocumentRedo.disconnect();
    this->connectDocumentDelete.disconnect();
}

void DocumentObserver::slotUndoDocument(const Document& /*Doc*/)
{
}

void DocumentObserver::slotRedoDocument(const Document& /*Doc*/)
{
}

void DocumentObserver::slotDeleteDocument(const Document& /*Doc*/)
{
}

void DocumentObserver::slotCreatedObject(const ViewProviderDocumentObject& /*Obj*/)
{
}

void DocumentObserver::slotDeletedObject(const ViewProviderDocumentObject& /*Obj*/)
{
}

void DocumentObserver::slotChangedObject(const ViewProviderDocumentObject& /*Obj*/,
                                         const App::Property& /*Prop*/)
{
}

void DocumentObserver::slotRelabelObject(const ViewProviderDocumentObject& /*Obj*/)
{
}

void DocumentObserver::slotActivatedObject(const ViewProviderDocumentObject& /*Obj*/)
{
}

void DocumentObserver::slotEnterEditObject(const ViewProviderDocumentObject& /*Obj*/)
{
}

void DocumentObserver::slotResetEditObject(const ViewProviderDocumentObject& /*Obj*/)
{
}
