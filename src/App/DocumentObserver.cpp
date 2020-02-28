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

#include <Base/Tools.h>
#include "Application.h"
#include "Document.h"
#include "DocumentObject.h"
#include "DocumentObserver.h"
#include "ComplexGeoData.h"
#include "GeoFeature.h"

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

const std::string &DocumentT::getDocumentName() const
{
    return document;
}

std::string DocumentT::getDocumentPython() const
{
    std::stringstream str;
    str << "App.getDocument(\"" << document << "\")";
    return str.str();
}

// -----------------------------------------------------------------------------

DocumentObjectT::DocumentObjectT()
{
}

DocumentObjectT::DocumentObjectT(const DocumentObjectT &other)
{
    *this = other;
}

DocumentObjectT::DocumentObjectT(DocumentObjectT &&other)
{
    *this = std::move(other);
}

DocumentObjectT::DocumentObjectT(const DocumentObject* obj)
{
    *this = obj;
}

DocumentObjectT::DocumentObjectT(const Property* prop)
{
    *this = prop;
}

DocumentObjectT::DocumentObjectT(const char *docName, const char *objName)
{
    if(docName)
        document = docName;
    if(objName)
        object = objName;
}

DocumentObjectT::~DocumentObjectT()
{
}

DocumentObjectT &DocumentObjectT::operator=(const DocumentObjectT& obj)
{
    if (this == &obj)
        return *this;
    object = obj.object;
    label = obj.label;
    document = obj.document;
    property = obj.property;
    return *this;
}

DocumentObjectT &DocumentObjectT::operator=(DocumentObjectT&& obj)
{
    if (this == &obj)
        return *this;
    object = std::move(obj.object);
    label = std::move(obj.label);
    document = std::move(obj.document);
    property = std::move(obj.property);
    return *this;
}

void DocumentObjectT::operator=(const DocumentObject* obj)
{
    if(!obj || !obj->getNameInDocument()) {
        object.clear();
        label.clear();
        document.clear();
        property.clear();
    } else {
        object = obj->getNameInDocument();
        label = obj->Label.getValue();
        document = obj->getDocument()->getName();
        property.clear();
    }
}

void DocumentObjectT::operator=(const Property *prop) {
    if(!prop || !prop->getName()
             || !prop->getContainer()
             || !prop->getContainer()->isDerivedFrom(App::DocumentObject::getClassTypeId()))
    {
        object.clear();
        label.clear();
        document.clear();
        property.clear();
    } else {
        auto obj = static_cast<App::DocumentObject*>(prop->getContainer());
        object = obj->getNameInDocument();
        label = obj->Label.getValue();
        document = obj->getDocument()->getName();
        property = prop->getName();
    }
}

bool DocumentObjectT::operator==(const DocumentObjectT &other) const {
    return document == other.document
        && object == other.object
        && label == other.label
        && property == other.property;
}

Document* DocumentObjectT::getDocument() const
{
    return GetApplication().getDocument(document.c_str());
}

const std::string& DocumentObjectT::getDocumentName() const
{
    return document;
}

std::string DocumentObjectT::getDocumentPython() const
{
    std::stringstream str;
    str << "FreeCAD.getDocument(\"" << document << "\")";
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

const std::string &DocumentObjectT::getObjectName() const
{
    return object;
}

const std::string &DocumentObjectT::getObjectLabel() const
{
    return label;
}

std::string DocumentObjectT::getObjectPython() const
{
    std::stringstream str;
    str << "FreeCAD.getDocument('" << document << "').getObject('" << object << "')";
    return str.str();
}

const std::string &DocumentObjectT::getPropertyName() const {
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

SubObjectT::SubObjectT()
{}

SubObjectT::SubObjectT(const SubObjectT &other)
    :DocumentObjectT(other), subname(other.subname)
{
}

SubObjectT::SubObjectT(SubObjectT &&other)
    :DocumentObjectT(std::move(other)), subname(std::move(other.subname))
{
}

SubObjectT::SubObjectT(const DocumentObject *obj, const char *s)
    :DocumentObjectT(obj),subname(s?s:"")
{
}

SubObjectT::SubObjectT(const DocumentObjectT& obj, const char *s)
    :DocumentObjectT(obj),subname(s?s:"")
{
}

SubObjectT::SubObjectT(const char *docName, const char *objName, const char *s)
    :DocumentObjectT(docName,objName), subname(s?s:"")
{
}

bool SubObjectT::operator<(const SubObjectT &other) const {
    if(getDocumentName() < other.getDocumentName())
        return true;
    if(getDocumentName() > other.getDocumentName())
        return false;
    if(getObjectName() < other.getObjectName())
        return true;
    if(getObjectName() > other.getObjectName())
        return false;
    if(getSubName() < other.getSubName())
        return true;
    if(getSubName() > other.getSubName())
        return false;
    return getPropertyName() < other.getPropertyName();
}

SubObjectT &SubObjectT::operator=(const SubObjectT& other)
{
    if (this == &other)
        return *this;
    static_cast<DocumentObjectT&>(*this) = other;
    subname = other.subname;
    return *this;
}

SubObjectT &SubObjectT::operator=(SubObjectT &&other)
{
    if (this == &other)
        return *this;
    static_cast<DocumentObjectT&>(*this) = std::move(other);
    subname = std::move(other.subname);
    return *this;
}

bool SubObjectT::operator==(const SubObjectT &other) const {
    return static_cast<const DocumentObjectT&>(*this) == other
        && subname == other.subname;
}

void SubObjectT::setSubName(const char *s) {
    subname = s?s:"";
}

const std::string &SubObjectT::getSubName() const {
    return subname;
}

std::string SubObjectT::getSubNameNoElement() const {
    return Data::ComplexGeoData::noElementName(subname.c_str());
}

const char *SubObjectT::getElementName() const {
    return Data::ComplexGeoData::findElementName(subname.c_str());
}

std::string SubObjectT::getNewElementName() const {
    std::pair<std::string, std::string> element;
    auto obj = getObject();
    if(!obj)
        return std::string();
    GeoFeature::resolveElement(obj,subname.c_str(),element);
    return std::move(element.first);
}

std::string SubObjectT::getOldElementName(int *index) const {
    std::pair<std::string, std::string> element;
    auto obj = getObject();
    if(!obj)
        return std::string();
    GeoFeature::resolveElement(obj,subname.c_str(),element);
    if(!index) 
        return std::move(element.second);
    std::size_t pos = element.second.find_first_of("0123456789");
    if(pos == std::string::npos)
        *index = -1;
    else {
        *index = std::atoi(element.second.c_str()+pos);
        element.second.resize(pos);
    }
    return std::move(element.second);
}

App::DocumentObject *SubObjectT::getSubObject() const {
    auto obj = getObject();
    if(obj)
        return obj->getSubObject(subname.c_str());
    return 0;
}

std::string SubObjectT::getSubObjectPython(bool force) const {
    if(!force && subname.empty())
        return getObjectPython();
    std::stringstream str;
    str << "(" << getObjectPython() << ",u'"
        << Base::Tools::escapedUnicodeFromUtf8(subname.c_str()) << "')";
    return str.str();
}

std::vector<App::DocumentObject*> SubObjectT::getSubObjectList() const {
    auto obj = getObject();
    if(obj)
        return obj->getSubObjectList(subname.c_str());
    return {};
}

// -----------------------------------------------------------------------------

class DocumentWeakPtrT::Private {
public:
    Private(App::Document* doc) : _document(doc) {
        if (doc) {
            connectApplicationDeletedDocument = App::GetApplication().signalDeleteDocument.connect(boost::bind
                (&Private::deletedDocument, this, _1));
        }
    }

    void deletedDocument(const App::Document& doc) {
        if (_document == &doc)
            reset();
    }
    void reset() {
        connectApplicationDeletedDocument.disconnect();
        _document = nullptr;
    }

    App::Document* _document;
    typedef boost::signals2::scoped_connection Connection;
    Connection connectApplicationDeletedDocument;
};

DocumentWeakPtrT::DocumentWeakPtrT(App::Document* doc) noexcept
  : d(new Private(doc))
{
}

DocumentWeakPtrT::~DocumentWeakPtrT()
{
}

void DocumentWeakPtrT::reset() noexcept
{
    d->reset();
}

bool DocumentWeakPtrT::expired() const noexcept
{
    return (d->_document == nullptr);
}

App::Document* DocumentWeakPtrT::operator->() noexcept
{
    return d->_document;
}

// -----------------------------------------------------------------------------

class DocumentObjectWeakPtrT::Private {
public:
    Private(App::DocumentObject* obj) : object(obj), indocument(false) {
        if (obj) {
            indocument = true;
            connectApplicationDeletedDocument = App::GetApplication().signalDeleteDocument.connect(boost::bind
                (&Private::deletedDocument, this, _1));
            App::Document* doc = obj->getDocument();
            connectDocumentCreatedObject = doc->signalNewObject.connect(boost::bind
                (&Private::createdObject, this, _1));
            connectDocumentDeletedObject = doc->signalDeletedObject.connect(boost::bind
                (&Private::deletedObject, this, _1));
        }
    }
    void deletedDocument(const App::Document& doc) {
        // When deleting document then there is no way to undo it
        if (object && object->getDocument() == &doc) {
            reset();
        }
    }
    void createdObject(const App::DocumentObject& obj) {
        // When undoing the removal
        if (object == &obj) {
            indocument = true;
        }
    }
    void deletedObject(const App::DocumentObject& obj) {
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
    App::DocumentObject* get() const {
        return indocument ? object : nullptr;
    }

    App::DocumentObject* object;
    bool indocument;
    typedef boost::signals2::scoped_connection Connection;
    Connection connectApplicationDeletedDocument;
    Connection connectDocumentCreatedObject;
    Connection connectDocumentDeletedObject;
};

DocumentObjectWeakPtrT::DocumentObjectWeakPtrT(App::DocumentObject* obj) noexcept
  : d(new Private(obj))
{
}

DocumentObjectWeakPtrT::~DocumentObjectWeakPtrT()
{

}

App::DocumentObject* DocumentObjectWeakPtrT::_get() const noexcept
{
    return d->get();
}

void DocumentObjectWeakPtrT::reset() noexcept
{
    d->reset();
}

bool DocumentObjectWeakPtrT::expired() const noexcept
{
    return !d->indocument;
}

App::DocumentObject* DocumentObjectWeakPtrT::operator->() noexcept
{
    return d->get();
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
