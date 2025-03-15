// SPDX-License-Identifier: LGPL-2.1-or-later

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

#include <Base/Tools.h>

#include "Application.h"
#include "ElementNamingUtils.h"
#include "Document.h"
#include "DocumentObserver.h"
#include "GeoFeature.h"
#include "Link.h"

using namespace App;
namespace sp = std::placeholders;


DocumentT::DocumentT() = default;

DocumentT::DocumentT(Document* doc)
    : document {doc->getName()}
{}

DocumentT::DocumentT(std::string name)
    : document {std::move(name)}
{}

DocumentT::DocumentT(const DocumentT& doc)
    : document {doc.document}
{}

DocumentT::DocumentT(DocumentT&& doc) noexcept
    : document {std::move(doc.document)}
{}

DocumentT::~DocumentT() = default;

DocumentT& DocumentT::operator=(const DocumentT& doc)
{
    if (this != &doc) {
        document = doc.document;
    }
    return *this;
}

DocumentT& DocumentT::operator=(DocumentT&& doc) noexcept
{
    if (this != &doc) {
        document = std::move(doc.document);
    }
    return *this;
}

DocumentT& DocumentT::operator=(const Document* doc)
{
    document = doc->getName();
    return *this;
}

DocumentT& DocumentT::operator=(const std::string& name)
{
    document = name;
    return *this;
}

Document* DocumentT::getDocument() const
{
    return GetApplication().getDocument(document.c_str());
}

const std::string& DocumentT::getDocumentName() const
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

DocumentObjectT::DocumentObjectT() = default;

DocumentObjectT::DocumentObjectT(const DocumentObjectT& other)
{
    *this = other;
}

DocumentObjectT::DocumentObjectT(DocumentObjectT&& other) noexcept
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

DocumentObjectT::DocumentObjectT(const Document* doc, const std::string& objName)
{
    if (doc && doc->getName()) {
        document = doc->getName();
    }
    object = objName;
}

DocumentObjectT::DocumentObjectT(const char* docName, const char* objName)
{
    if (docName) {
        document = docName;
    }
    if (objName) {
        object = objName;
    }
}

DocumentObjectT::~DocumentObjectT() = default;

DocumentObjectT& DocumentObjectT::operator=(const DocumentObjectT& obj)
{
    if (this == &obj) {
        return *this;
    }
    object = obj.object;
    label = obj.label;
    document = obj.document;
    property = obj.property;
    return *this;
}

DocumentObjectT& DocumentObjectT::operator=(DocumentObjectT&& obj) noexcept
{
    if (this == &obj) {
        return *this;
    }
    object = std::move(obj.object);
    label = std::move(obj.label);
    document = std::move(obj.document);
    property = std::move(obj.property);
    return *this;
}

DocumentObjectT& DocumentObjectT::operator=(const DocumentObject* obj)
{
    if (!obj || !obj->isAttachedToDocument()) {
        object.clear();
        label.clear();
        document.clear();
        property.clear();
    }
    else {
        object = obj->getNameInDocument();
        label = obj->Label.getValue();
        document = obj->getDocument()->getName();
        property.clear();
    }

    return *this;
}

DocumentObjectT& DocumentObjectT::operator=(const Property* prop)
{
    if (!prop || !prop->hasName() || !prop->getContainer()
        || !prop->getContainer()->isDerivedFrom<App::DocumentObject>()) {
        object.clear();
        label.clear();
        document.clear();
        property.clear();
    }
    else if (auto obj = freecad_cast<App::DocumentObject*>(prop->getContainer())) {
        object = obj->getNameInDocument();
        label = obj->Label.getValue();
        document = obj->getDocument()->getName();
        property = prop->getName();
    }

    return *this;
}

bool DocumentObjectT::operator==(const DocumentObjectT& other) const
{
    return document == other.document && object == other.object && label == other.label
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
    DocumentObject* obj = nullptr;
    if (auto doc = getDocument()) {
        obj = doc->getObject(object.c_str());
    }
    return obj;
}

const std::string& DocumentObjectT::getObjectName() const
{
    return object;
}

const char* DocumentObjectT::getNameInDocument() const
{
    return object.c_str();
}

bool DocumentObjectT::isAttachedToDocument() const
{
    if (auto obj = getObject()) {
        return obj->isAttachedToDocument();
    }

    return false;
}

const std::string& DocumentObjectT::getObjectLabel() const
{
    return label;
}

std::string DocumentObjectT::getObjectPython() const
{
    std::stringstream str;
    str << "FreeCAD.getDocument('" << document << "').getObject('" << object << "')";
    return str.str();
}

const std::string& DocumentObjectT::getPropertyName() const
{
    return property;
}

std::string DocumentObjectT::getPropertyPython() const
{
    std::stringstream str;
    str << getObjectPython();
    if (!property.empty()) {
        str << '.' << property;
    }
    return str.str();
}

Property* DocumentObjectT::getProperty() const
{
    return getPropertyByName(property.c_str());
}

Property* DocumentObjectT::getPropertyByName(const char* name) const
{
    if (auto obj = getObject()) {
        return obj->getPropertyByName(name);
    }
    return nullptr;
}

// -----------------------------------------------------------------------------

SubObjectT::SubObjectT() = default;

SubObjectT::SubObjectT(const SubObjectT&) = default;

SubObjectT::SubObjectT(SubObjectT&& other) noexcept
    : DocumentObjectT(std::move(other))
    , subname(std::move(other.subname))
{}

SubObjectT::SubObjectT(const DocumentObject* obj, const char* s)
    : DocumentObjectT(obj)
    , subname(s ? s : "")
{}

SubObjectT::SubObjectT(const DocumentObject* obj)
    : DocumentObjectT(obj)
{}

SubObjectT::SubObjectT(const DocumentObjectT& obj, const char* s)
    : DocumentObjectT(obj)
    , subname(s ? s : "")
{}

SubObjectT::SubObjectT(const char* docName, const char* objName, const char* s)
    : DocumentObjectT(docName, objName)
    , subname(s ? s : "")
{}

SubObjectT::~SubObjectT() = default;

bool SubObjectT::operator<(const SubObjectT& other) const
{
    if (getDocumentName() < other.getDocumentName()) {
        return true;
    }
    if (getDocumentName() > other.getDocumentName()) {
        return false;
    }
    if (getObjectName() < other.getObjectName()) {
        return true;
    }
    if (getObjectName() > other.getObjectName()) {
        return false;
    }
    if (getSubName() < other.getSubName()) {
        return true;
    }
    if (getSubName() > other.getSubName()) {
        return false;
    }
    return getPropertyName() < other.getPropertyName();
}

SubObjectT& SubObjectT::operator=(const SubObjectT& other)
{
    if (this == &other) {
        return *this;
    }
    static_cast<DocumentObjectT&>(*this) = other;  // NOLINT
    subname = other.subname;
    return *this;
}

SubObjectT& SubObjectT::operator=(SubObjectT&& other) noexcept
{
    if (this == &other) {
        return *this;
    }
    subname = std::move(other.subname);
    static_cast<DocumentObjectT&>(*this) = std::move(other);
    return *this;
}

SubObjectT& SubObjectT::operator=(const DocumentObjectT& other)
{
    if (this == &other) {
        return *this;
    }
    static_cast<DocumentObjectT&>(*this) = other;
    subname.clear();
    return *this;
}

SubObjectT& SubObjectT::operator=(const DocumentObject* other)
{
    static_cast<DocumentObjectT&>(*this) = other;
    subname.clear();
    return *this;
}

bool SubObjectT::operator==(const SubObjectT& other) const
{
    return static_cast<const DocumentObjectT&>(*this) == other && subname == other.subname;
}

namespace
{
bool normalizeConvertIndex(const std::vector<App::DocumentObject*>& objs, const unsigned int idx)
{
    if (auto ext = objs[idx - 1]->getExtensionByType<App::LinkBaseExtension>(true)) {
        if ((ext->getElementCountValue() != 0) && !ext->getShowElementValue()) {
            // If the parent is a collapsed link array element, then we
            // have to keep the index no matter what, because there is
            // no sub-object corresponding to an array element.
            return true;
        }
    }
    return false;
}
}  // namespace

bool SubObjectT::normalize(NormalizeOptions options)
{
    bool noElement = options.testFlag(NormalizeOption::NoElement);
    bool flatten = !options.testFlag(NormalizeOption::NoFlatten);
    bool keepSub = options.testFlag(NormalizeOption::KeepSubName);
    bool convertIndex = options.testFlag(NormalizeOption::ConvertIndex);

    std::ostringstream ss;
    std::vector<int> subs;
    auto obj = getObject();
    if (!obj) {
        return false;
    }
    auto objs = obj->getSubObjectList(subname.c_str(), &subs, flatten);
    if (objs.empty()) {
        return false;
    }
    for (unsigned i = 1; i < objs.size(); ++i) {
        // Keep digit-only subname, as it maybe an index to an array, which does
        // not expand its elements as objects.
        const char* end = subname.c_str() + subs[i];
        const char* sub = end - 2;
        for (;; --sub) {
            if (sub < subname.c_str()) {
                sub = subname.c_str();
                break;
            }
            if (*sub == '.') {
                ++sub;
                break;
            }
        }
        bool _keepSub {};
        if (!std::isdigit(sub[0])) {
            _keepSub = keepSub;
        }
        else if (!convertIndex) {
            _keepSub = true;
        }
        else {
            _keepSub = normalizeConvertIndex(objs, i);
        }
        if (_keepSub) {
            ss << std::string(sub, end);
        }
        else {
            ss << objs[i]->getNameInDocument() << ".";
        }
    }
    if (objs.size() > 1 && objs.front()->getSubObject(ss.str().c_str()) != objs.back()) {
        // something went wrong
        return false;
    }
    if (!noElement) {
        ss << getOldElementName();
    }
    std::string sub = ss.str();
    if (objs.front() != obj || subname != sub) {
        *this = objs.front();
        subname = std::move(sub);
        return true;
    }
    return false;
}

SubObjectT App::SubObjectT::normalized(NormalizeOptions options) const
{
    SubObjectT res(*this);
    res.normalize(options);
    return res;
}

void SubObjectT::setSubName(const char* s)
{
    subname = s ? s : "";
}

const std::string& SubObjectT::getSubName() const
{
    return subname;
}

std::string SubObjectT::getSubNameNoElement() const
{
    return Data::noElementName(subname.c_str());
}

const char* SubObjectT::getElementName() const
{
    return Data::findElementName(subname.c_str());
}

bool SubObjectT::hasSubObject() const
{
    return Data::findElementName(subname.c_str()) != subname.c_str();
}

bool SubObjectT::hasSubElement() const
{
    auto element = getElementName();
    return element && (element[0] != '\0');
}

std::string SubObjectT::getNewElementName() const
{
    ElementNamePair element;
    auto obj = getObject();
    if (!obj) {
        return {};
    }
    GeoFeature::resolveElement(obj, subname.c_str(), element);
    return std::move(element.newName);
}

std::string SubObjectT::getOldElementName(int* index) const
{
    ElementNamePair element;
    auto obj = getObject();
    if (!obj) {
        return {};
    }
    GeoFeature::resolveElement(obj, subname.c_str(), element);
    if (!index) {
        return std::move(element.oldName);
    }
    std::size_t pos = element.oldName.find_first_of("0123456789");
    if (pos == std::string::npos) {
        *index = -1;
    }
    else {
        *index = std::atoi(element.oldName.c_str() + pos);
        element.oldName.resize(pos);
    }
    return std::move(element.oldName);
}

App::DocumentObject* SubObjectT::getSubObject() const
{
    auto obj = getObject();
    if (obj) {
        return obj->getSubObject(subname.c_str());
    }
    return nullptr;
}

std::string SubObjectT::getSubObjectPython(bool force) const
{
    if (!force && subname.empty()) {
        return getObjectPython();
    }
    std::stringstream str;
    str << "(" << getObjectPython() << ",u'" << Base::Tools::escapedUnicodeFromUtf8(subname.c_str())
        << "')";
    return str.str();
}

std::vector<App::DocumentObject*> SubObjectT::getSubObjectList() const
{
    auto obj = getObject();
    if (obj) {
        return obj->getSubObjectList(subname.c_str());
    }
    return {};
}

std::string SubObjectT::getObjectFullName(const char* docName) const
{
    std::ostringstream ss;
    if (!docName || getDocumentName() != docName) {
        ss << getDocumentName();
        if (auto doc = getDocument()) {
            if (doc->Label.getStrValue() != getDocumentName()) {
                ss << "(" << doc->Label.getValue() << ")";
            }
        }
        ss << "#";
    }
    ss << getObjectName();
    if (!getObjectLabel().empty() && getObjectLabel() != getObjectName()) {
        ss << " (" << getObjectLabel() << ")";
    }
    return ss.str();
}

std::string SubObjectT::getSubObjectFullName(const char* docName) const
{
    if (subname.empty()) {
        return getObjectFullName(docName);
    }
    std::ostringstream ss;
    if (!docName || getDocumentName() != docName) {
        ss << getDocumentName();
        if (auto doc = getDocument()) {
            if (doc->Label.getStrValue() != getDocumentName()) {
                ss << "(" << doc->Label.getValue() << ")";
            }
        }
        ss << "#";
    }
    ss << getObjectName() << "." << subname;
    auto sobj = getSubObject();
    if (sobj && sobj->Label.getStrValue() != sobj->getNameInDocument()) {
        ss << " (" << sobj->Label.getValue() << ")";
    }
    return ss.str();
}

// -----------------------------------------------------------------------------

PropertyLinkT::PropertyLinkT()
    : toPython("None")
{}

PropertyLinkT::PropertyLinkT(DocumentObject* obj)
    : PropertyLinkT()
{
    if (obj) {
        std::ostringstream str;
        DocumentObjectT objT(obj);
        str << objT.getObjectPython();

        toPython = str.str();
    }
}

PropertyLinkT::PropertyLinkT(DocumentObject* obj, const std::vector<std::string>& subNames)
    : PropertyLinkT()
{
    if (obj) {
        std::ostringstream str;
        DocumentObjectT objT(obj);
        str << "(" << objT.getObjectPython() << ",[";
        for (const auto& it : subNames) {
            str << "'" << it << "',";
        }
        str << "])";

        toPython = str.str();
    }
}

PropertyLinkT::PropertyLinkT(const std::vector<DocumentObject*>& objs)
    : PropertyLinkT()
{
    if (!objs.empty()) {
        std::stringstream str;
        str << "[";
        for (std::size_t i = 0; i < objs.size(); i++) {
            if (i > 0) {
                str << ", ";
            }

            App::DocumentObject* obj = objs[i];
            if (obj) {
                DocumentObjectT objT(obj);
                str << objT.getObjectPython();
            }
            else {
                str << "None";
            }
        }

        str << "]";
    }
}

PropertyLinkT::PropertyLinkT(
    const std::vector<DocumentObject*>& objs,
    const std::vector<std::string>& subNames
)
    : PropertyLinkT()
{
    if (!objs.empty() && objs.size() == subNames.size()) {
        std::stringstream str;
        str << "[";
        for (std::size_t i = 0; i < subNames.size(); i++) {
            if (i > 0) {
                str << ",(";
            }
            else {
                str << "(";
            }

            App::DocumentObject* obj = objs[i];
            if (obj) {
                DocumentObjectT objT(obj);
                str << objT.getObjectPython();
            }
            else {
                str << "None";
            }

            str << ",";
            str << "'" << subNames[i] << "'";
            str << ")";
        }

        str << "]";
    }
}

std::string PropertyLinkT::getPropertyPython() const
{
    return toPython;
}

// -----------------------------------------------------------------------------

class DocumentWeakPtrT::Private
{
public:
    explicit Private(App::Document* doc)
        : _document(doc)
    {
        if (doc) {
            // NOLINTBEGIN
            connectApplicationDeletedDocument = App::GetApplication().signalDeleteDocument.connect(
                std::bind(&Private::deletedDocument, this, sp::_1)
            );
            // NOLINTEND
        }
    }

    void deletedDocument(const App::Document& doc)
    {
        if (_document == &doc) {
            reset();
        }
    }
    void reset()
    {
        connectApplicationDeletedDocument.disconnect();
        _document = nullptr;
    }

    App::Document* _document;
    using Connection = fastsignals::scoped_connection;
    Connection connectApplicationDeletedDocument;
};

DocumentWeakPtrT::DocumentWeakPtrT(App::Document* doc) noexcept
    : d(new Private(doc))
{}

DocumentWeakPtrT::~DocumentWeakPtrT() = default;

void DocumentWeakPtrT::reset() noexcept
{
    d->reset();
}

bool DocumentWeakPtrT::expired() const noexcept
{
    return (d->_document == nullptr);
}

App::Document* DocumentWeakPtrT::operator*() const noexcept
{
    return d->_document;
}

App::Document* DocumentWeakPtrT::operator->() const noexcept
{
    return d->_document;
}

// -----------------------------------------------------------------------------

class DocumentObjectWeakPtrT::Private
{
public:
    explicit Private(App::DocumentObject* obj)
        : object(obj)
    {
        set(obj);
    }
    void deletedDocument(const App::Document& doc)
    {
        // When deleting document then there is no way to undo it
        if (object && object->getDocument() == &doc) {
            reset();
        }
    }
    void createdObject(const App::DocumentObject& obj) noexcept
    {
        // When undoing the removal
        if (object == &obj) {
            indocument = true;
        }
    }
    void deletedObject(const App::DocumentObject& obj) noexcept
    {
        if (object == &obj) {
            indocument = false;
        }
    }
    void reset()
    {
        connectApplicationDeletedDocument.disconnect();
        connectDocumentCreatedObject.disconnect();
        connectDocumentDeletedObject.disconnect();
        object = nullptr;
        indocument = false;
    }
    void set(App::DocumentObject* obj)
    {
        object = obj;
        if (obj) {
            // NOLINTBEGIN
            indocument = true;
            connectApplicationDeletedDocument = App::GetApplication().signalDeleteDocument.connect(
                std::bind(&Private::deletedDocument, this, sp::_1)
            );
            App::Document* doc = obj->getDocument();
            connectDocumentCreatedObject = doc->signalNewObject.connect(
                std::bind(&Private::createdObject, this, sp::_1)
            );
            connectDocumentDeletedObject = doc->signalDeletedObject.connect(
                std::bind(&Private::deletedObject, this, sp::_1)
            );
            // NOLINTEND
        }
    }
    App::DocumentObject* get() const noexcept
    {
        return indocument ? object : nullptr;
    }

    App::DocumentObject* object;
    bool indocument {false};
    using Connection = fastsignals::scoped_connection;
    Connection connectApplicationDeletedDocument;
    Connection connectDocumentCreatedObject;
    Connection connectDocumentDeletedObject;
};

DocumentObjectWeakPtrT::DocumentObjectWeakPtrT(App::DocumentObject* obj)
    : d(new Private(obj))
{}

DocumentObjectWeakPtrT::~DocumentObjectWeakPtrT() = default;

App::DocumentObject* DocumentObjectWeakPtrT::_get() const noexcept
{
    return d->get();
}

DocumentObjectWeakPtrT::DocumentObjectWeakPtrT(DocumentObjectWeakPtrT&&) = default;
DocumentObjectWeakPtrT& DocumentObjectWeakPtrT::operator=(DocumentObjectWeakPtrT&&) = default;

void DocumentObjectWeakPtrT::reset()
{
    d->reset();
}

bool DocumentObjectWeakPtrT::expired() const noexcept
{
    return !d->indocument;
}

DocumentObjectWeakPtrT& DocumentObjectWeakPtrT::operator=(App::DocumentObject* p)
{
    d->reset();
    d->set(p);
    return *this;
}

App::DocumentObject* DocumentObjectWeakPtrT::operator*() const noexcept
{
    return d->get();
}

App::DocumentObject* DocumentObjectWeakPtrT::operator->() const noexcept
{
    return d->get();
}

bool DocumentObjectWeakPtrT::operator==(const DocumentObjectWeakPtrT& p) const noexcept
{
    return d->get() == p.d->get();
}

bool DocumentObjectWeakPtrT::operator!=(const DocumentObjectWeakPtrT& p) const noexcept
{
    return d->get() != p.d->get();
}

// -----------------------------------------------------------------------------

DocumentObserver::DocumentObserver()
    : _document(nullptr)
{
    // NOLINTBEGIN
    this->connectApplicationCreatedDocument = App::GetApplication().signalNewDocument.connect(
        std::bind(&DocumentObserver::slotCreatedDocument, this, sp::_1)
    );
    this->connectApplicationDeletedDocument = App::GetApplication().signalDeleteDocument.connect(
        std::bind(&DocumentObserver::slotDeletedDocument, this, sp::_1)
    );
    this->connectApplicationActivateDocument = App::GetApplication().signalActiveDocument.connect(
        std::bind(&DocumentObserver::slotActivateDocument, this, sp::_1)
    );
    // NOLINTEND
}

DocumentObserver::DocumentObserver(Document* doc)
    : DocumentObserver()
{
    // Connect to application and given document
    attachDocument(doc);
}

DocumentObserver::~DocumentObserver()
{
    // disconnect from application and document
    this->connectApplicationCreatedDocument.disconnect();
    this->connectApplicationDeletedDocument.disconnect();
    this->connectApplicationActivateDocument.disconnect();
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

        // NOLINTBEGIN
        this->connectDocumentCreatedObject = _document->signalNewObject.connect(
            std::bind(&DocumentObserver::slotCreatedObject, this, sp::_1)
        );
        this->connectDocumentDeletedObject = _document->signalDeletedObject.connect(
            std::bind(&DocumentObserver::slotDeletedObject, this, sp::_1)
        );
        this->connectDocumentChangedObject = _document->signalChangedObject.connect(
            std::bind(&DocumentObserver::slotChangedObject, this, sp::_1, sp::_2)
        );
        this->connectDocumentRecomputedObject = _document->signalRecomputedObject.connect(
            std::bind(&DocumentObserver::slotRecomputedObject, this, sp::_1)
        );
        this->connectDocumentRecomputed = _document->signalRecomputed.connect(
            std::bind(&DocumentObserver::slotRecomputedDocument, this, sp::_1)
        );
        // NOLINTEND
    }
}

void DocumentObserver::detachDocument()
{
    if (this->_document) {
        this->_document = nullptr;
        this->connectDocumentCreatedObject.disconnect();
        this->connectDocumentDeletedObject.disconnect();
        this->connectDocumentChangedObject.disconnect();
        this->connectDocumentRecomputedObject.disconnect();
        this->connectDocumentRecomputed.disconnect();
    }
}

void DocumentObserver::slotCreatedDocument(const App::Document& /*Doc*/)
{}

void DocumentObserver::slotDeletedDocument(const App::Document& /*Doc*/)
{}

void DocumentObserver::slotActivateDocument(const App::Document& /*Doc*/)
{}

void DocumentObserver::slotCreatedObject(const App::DocumentObject& /*Obj*/)
{}

void DocumentObserver::slotDeletedObject(const App::DocumentObject& /*Obj*/)
{}

void DocumentObserver::slotChangedObject(const App::DocumentObject& /*Obj*/, const App::Property& /*Prop*/)
{}

void DocumentObserver::slotRecomputedObject(const DocumentObject& /*Obj*/)
{}

void DocumentObserver::slotRecomputedDocument(const Document& /*doc*/)
{}


// -----------------------------------------------------------------------------

DocumentObjectObserver::DocumentObjectObserver() = default;

DocumentObjectObserver::~DocumentObjectObserver() = default;

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

void DocumentObjectObserver::slotCreatedDocument([[maybe_unused]] const App::Document& doc)
{}

void DocumentObjectObserver::slotDeletedDocument(const App::Document& Doc)
{
    if (this->getDocument() == &Doc) {
        this->detachDocument();
        _objects.clear();
        cancelObservation();
    }
}

void DocumentObjectObserver::slotCreatedObject([[maybe_unused]] const App::DocumentObject& obj)
{}

void DocumentObjectObserver::slotDeletedObject(const App::DocumentObject& Obj)
{
    auto it = _objects.find(const_cast<App::DocumentObject*>(&Obj));
    if (it != _objects.end()) {
        _objects.erase(it);
    }
    if (_objects.empty()) {
        cancelObservation();
    }
}

void DocumentObjectObserver::slotChangedObject(
    [[maybe_unused]] const App::DocumentObject& obj,
    [[maybe_unused]] const App::Property& prop
)
{}

void DocumentObjectObserver::cancelObservation()
{}
