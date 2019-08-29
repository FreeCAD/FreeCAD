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

#include <boost/bind.hpp>

#include "Application.h"
#include "Document.h"
#include "DocumentObject.h"
#include "DocumentObserver.h"

using namespace App;


DocumentT::DocumentT()
{
}

DocumentT::DocumentT(Document* doc)
{
    document = doc->getName();
}

DocumentT::DocumentT(const std::string& name)
{
    document = name;
}

DocumentT::~DocumentT()
{
}

void DocumentT::operator=(const DocumentT& doc)
{
    if (this == &doc)
        return;
    document = doc.document;
}

void DocumentT::operator=(const Document* doc)
{
    document = doc->getName();
}

void DocumentT::operator=(const std::string& name)
{
    document = name;
}

Document* DocumentT::getDocument() const
{
    return GetApplication().getDocument(document.c_str());
}

std::string DocumentT::getDocumentName() const
{
    return document;
}

std::string DocumentT::getDocumentPython() const
{
    std::stringstream str;
    Document* doc = GetApplication().getActiveDocument();
    if (doc && document == doc->getName()) {
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

DocumentObjectT::DocumentObjectT()
{
}

DocumentObjectT::DocumentObjectT(const DocumentObject* obj)
{
    object = obj->getNameInDocument();
    label = obj->Label.getValue();
    document = obj->getDocument()->getName();
}

DocumentObjectT::DocumentObjectT(const Property* prop)
{
    *this = prop;
}

DocumentObjectT::~DocumentObjectT()
{
}

void DocumentObjectT::operator=(const DocumentObjectT& obj)
{
    if (this == &obj)
        return;
    object = obj.object;
    label = obj.label;
    document = obj.document;
    property = obj.property;
}

void DocumentObjectT::operator=(const DocumentObject* obj)
{
    object = obj->getNameInDocument();
    label = obj->Label.getValue();
    document = obj->getDocument()->getName();
    property.clear();
}

void DocumentObjectT::operator=(const Property *prop) {
    auto obj = dynamic_cast<const DocumentObject*>(prop->getContainer());
    assert(obj);
    object = obj->getNameInDocument();
    label = obj->Label.getValue();
    document = obj->getDocument()->getName();
    property = prop->getName();
}

Document* DocumentObjectT::getDocument() const
{
    return GetApplication().getDocument(document.c_str());
}

std::string DocumentObjectT::getDocumentName() const
{
    return document;
}

std::string DocumentObjectT::getDocumentPython() const
{
    std::stringstream str;
    Document* doc = GetApplication().getActiveDocument();
    if (doc && document == doc->getName()) {
        str << "App.ActiveDocument";
    }
    else {
        str << "App.getDocument(\""
            << document
            << "\")";
    }
    return str.str();
}

DocumentObject* DocumentObjectT::getObject() const
{
    DocumentObject* obj = 0;
    Document* doc = getDocument();
    if (doc) {
        obj = doc->getObject(object.c_str());
    }
    return obj;
}

std::string DocumentObjectT::getObjectName() const
{
    return object;
}

std::string DocumentObjectT::getObjectLabel() const
{
    return label;
}

std::string DocumentObjectT::getObjectPython() const
{
    std::stringstream str;
    Document* doc = GetApplication().getActiveDocument();
    if (doc && document == doc->getName()) {
        str << "App.ActiveDocument.";
    }
    else {
        str << "App.getDocument(\""
            << document
            << "\").";
    }

    str << object;
    return str.str();
}

std::string DocumentObjectT::getPropertyName() const {
    return property;
}

std::string DocumentObjectT::getPropertyPython() const
{
    std::stringstream str;
    str << "FreeCAD.getDocument('" << document 
        << "').getObject('" << object
        << "')";
    if(property.size())
        str << '.' << property;
    return str.str();
}

Property *DocumentObjectT::getProperty() const {
    auto obj = getObject();
    if(obj)
        return obj->getPropertyByName(property.c_str());
    return 0;
}
// -----------------------------------------------------------------------------

DocumentObserver::DocumentObserver() : _document(0)
{
    this->connectApplicationCreatedDocument = App::GetApplication().signalNewDocument.connect(boost::bind
        (&DocumentObserver::slotCreatedDocument, this, _1));
    this->connectApplicationDeletedDocument = App::GetApplication().signalDeleteDocument.connect(boost::bind
        (&DocumentObserver::slotDeletedDocument, this, _1));
}

DocumentObserver::DocumentObserver(Document* doc) : _document(0)
{
    // Connect to application and given document
    this->connectApplicationCreatedDocument = App::GetApplication().signalNewDocument.connect(boost::bind
        (&DocumentObserver::slotCreatedDocument, this, _1));
    this->connectApplicationDeletedDocument = App::GetApplication().signalDeleteDocument.connect(boost::bind
        (&DocumentObserver::slotDeletedDocument, this, _1));
    attachDocument(doc);
}

DocumentObserver::~DocumentObserver()
{
    // disconnect from application and document
    this->connectApplicationCreatedDocument.disconnect();
    this->connectApplicationDeletedDocument.disconnect();
    detachDocument();
}

Document* DocumentObserver::getDocument() const
{
    return this->_document;
}

void DocumentObserver::attachDocument(Document* doc)
{
    if (_document != doc) {
        detachDocument();
        _document = doc;

        this->connectDocumentCreatedObject = _document->signalNewObject.connect(boost::bind
            (&DocumentObserver::slotCreatedObject, this, _1));
        this->connectDocumentDeletedObject = _document->signalDeletedObject.connect(boost::bind
            (&DocumentObserver::slotDeletedObject, this, _1));
        this->connectDocumentChangedObject = _document->signalChangedObject.connect(boost::bind
            (&DocumentObserver::slotChangedObject, this, _1, _2));
        this->connectDocumentRecomputedObject = _document->signalRecomputedObject.connect(boost::bind
            (&DocumentObserver::slotRecomputedObject, this, _1));
        this->connectDocumentRecomputed = _document->signalRecomputed.connect(boost::bind
            (&DocumentObserver::slotRecomputedDocument, this, _1));
    }
}

void DocumentObserver::detachDocument()
{
    if (this->_document) {
        this->_document = 0;
        this->connectDocumentCreatedObject.disconnect();
        this->connectDocumentDeletedObject.disconnect();
        this->connectDocumentChangedObject.disconnect();
        this->connectDocumentRecomputedObject.disconnect();
        this->connectDocumentRecomputed.disconnect();
    }
}

void DocumentObserver::slotCreatedDocument(const App::Document& /*Doc*/)
{
}

void DocumentObserver::slotDeletedDocument(const App::Document& /*Doc*/)
{
}

void DocumentObserver::slotCreatedObject(const App::DocumentObject& /*Obj*/)
{
}

void DocumentObserver::slotDeletedObject(const App::DocumentObject& /*Obj*/)
{
}

void DocumentObserver::slotChangedObject(const App::DocumentObject& /*Obj*/, const App::Property& /*Prop*/)
{
}

void DocumentObserver::slotRecomputedObject(const DocumentObject& /*Obj*/)
{
}

void DocumentObserver::slotRecomputedDocument(const Document& /*doc*/)
{
}


// -----------------------------------------------------------------------------

DocumentObjectObserver::DocumentObjectObserver()
{
}

DocumentObjectObserver::~DocumentObjectObserver()
{
}

DocumentObjectObserver::const_iterator DocumentObjectObserver::begin() const
{
    return _objects.begin();
}

DocumentObjectObserver::const_iterator DocumentObjectObserver::end() const
{
    return _objects.end();
}

void DocumentObjectObserver::addToObservation(App::DocumentObject* obj)
{
    _objects.insert(obj);
}

void DocumentObjectObserver::removeFromObservation(App::DocumentObject* obj)
{
    _objects.erase(obj);
}

void DocumentObjectObserver::slotCreatedDocument(const App::Document&)
{
}

void DocumentObjectObserver::slotDeletedDocument(const App::Document& Doc)
{
    if (this->getDocument() == &Doc) {
        this->detachDocument();
        _objects.clear();
        cancelObservation();
    }
}

void DocumentObjectObserver::slotCreatedObject(const App::DocumentObject&)
{
}

void DocumentObjectObserver::slotDeletedObject(const App::DocumentObject& Obj)
{
    std::set<App::DocumentObject*>::iterator it = _objects.find
        (const_cast<App::DocumentObject*>(&Obj));
    if (it != _objects.end())
        _objects.erase(it);
    if (_objects.empty())
        cancelObservation();
}

void DocumentObjectObserver::slotChangedObject(const App::DocumentObject&,
                                               const App::Property&)
{
}

void DocumentObjectObserver::cancelObservation()
{
}
