/***************************************************************************
 *   Copyright (c) Victor Titov (DeepSOIC)   <vv.titov@gmail.com> 2017     *
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

#include "ContainerBase.h"
#include "Container.h"
#include "Exceptions.h"

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/PropertyContainerPy.h>
#include <App/GroupExtension.h>
#include <App/GeoFeatureGroupExtension.h>
#include <App/Origin.h>
#include <App/Application.h>

#include <unordered_set>

using namespace App;

TYPESYSTEM_SOURCE_ABSTRACT(App::ContainerBase, Base::BaseClass)

ContainerBase::ContainerBase(PropertyContainer* pcObject)
    : pcObject(nullptr)
{
    if (pcObject) {
        if (pcObject->isDerivedFrom(PropertyContainer::getClassTypeId())) {
            this->ref = Py::asObject(pcObject->getPyObject());
            this->pcObject = static_cast<PropertyContainerPy*>(ref.ptr());
        }
    }
}

ContainerBase::~ContainerBase()
{
}

PropertyContainer* ContainerBase::object() const
{
    if (this->pcObject && this->pcObject->isValid())
        return pcObject->getPropertyContainerPtr();
    else
        return nullptr;
}

std::string ContainerBase::getName() const
{
    check();
    if (isADocument()){
        return asDocument().getName();
    } else if (isADocumentObject()){
        const char* nm = asDocumentObject().getNameInDocument();
        return std::string(nm ? nm : "");
    } else {
        assert(false /*unexpected type*/);
        throw Base::TypeError("Unexpected object type");
    }
}

std::vector<DocumentObject*> ContainerBase::allChildren() const
{
    auto result = staticChildren();
    auto dynamicchildren = dynamicChildren();
    result.insert(result.end(), dynamicchildren.begin(), dynamicchildren.end());
    return result;
}

std::vector<DocumentObject*> ContainerBase::dynamicChildrenRecursive() const
{
    return recursiveChildren(true, false);
}

std::vector<DocumentObject*> ContainerBase::staticChildrenRecursive() const
{
    return recursiveChildren(false, true);
}

std::vector<DocumentObject*> ContainerBase::allChildrenRecursive() const
{
    return recursiveChildren(true, true);
}

bool ContainerBase::hasObject(const DocumentObject* obj) const
{
    auto children = this->allChildren();
    return std::find(children.begin(), children.end(), obj) != children.end();
}

bool ContainerBase::hasDynamicObject(const DocumentObject* obj) const
{
    //TODO: Py interface!
    auto children = this->dynamicChildren();
    return std::find(children.begin(), children.end(), obj) != children.end();
}

bool ContainerBase::hasStaticObject(const DocumentObject* obj) const
{
    //TODO: Py interface!
    auto children = this->staticChildren();
    return std::find(children.begin(), children.end(), obj) != children.end();
}

PropertyContainer* ContainerBase::parent() const
{
    if (isRoot())
        return nullptr;
    auto parents = this->parents();
    if (parents.size() == 0) {
        return nullptr;
    } else if (parents.size() == 1) {
        return parents[0];
    } else {
        throw ContainerTreeError("Object has more than one parent");
    }
}

DocumentObject* ContainerBase::getObject(const char* objectName) const
{
    check();
    auto obj = getDocument()->getObject(objectName);
    if (!(hasObject(obj))){
        std::stringstream msg;
        msg << "Object named '" << objectName << "' not found in " << getName();
        throw ObjectNotFoundError(msg.str());
    }
    return obj;
}

Document* ContainerBase::getDocument() const
{
    check();
    if (isADocument())
        return static_cast<Document*>(object());
    else
        return static_cast<DocumentObject*>(object())->getDocument();
}

bool ContainerBase::isADocument() const
{
    return object() && object()->isDerivedFrom(Document::getClassTypeId());
}

bool ContainerBase::isAGroup() const
{
    if (object() && object()->isDerivedFrom(DocumentObject::getClassTypeId())){
        auto dobj = static_cast<DocumentObject*>(object());
        if (dobj->hasExtension(GroupExtension::getExtensionClassTypeId()))
            return true;
    }
    return false;
}

bool ContainerBase::isAGeoGroup() const
{
    if (object() && object()->isDerivedFrom(DocumentObject::getClassTypeId())){
        auto dobj = static_cast<DocumentObject*>(object());
        if (dobj->hasExtension(GeoFeatureGroupExtension::getExtensionClassTypeId()))
            return true;
    }
    return false;
}

bool ContainerBase::isAnOrigin() const
{
    return object() && object()->isDerivedFrom(Origin::getClassTypeId());
}

bool ContainerBase::isAWorkspace() const
{
    return isAGeoGroup() || isADocument();
}

Document& ContainerBase::asDocument() const
{
    if (!isADocument())
        throw Base::TypeError("Container::asDocument: Not a document!");
    return static_cast<Document&>(*object());
}

GroupExtension& ContainerBase::asGroup() const
{
    if (!isAGroup())
        throw Base::TypeError("Container::asGroup: Not a group!");
    return static_cast<GroupExtension&>(*static_cast<DocumentObject*>(object())->getExtensionByType<GroupExtension>());
}

GeoFeatureGroupExtension& ContainerBase::asGeoGroup() const
{
    if (!isAGeoGroup())
        throw Base::TypeError("Container::asOriginGroup: Not an origingroup!");
    return static_cast<GeoFeatureGroupExtension&>(*static_cast<DocumentObject*>(object())->getExtensionByType<GeoFeatureGroupExtension>());
}

Origin& ContainerBase::asOrigin() const
{
    if (!isAnOrigin())
        throw Base::TypeError("Container::asOrigin: Not an origin!");
    return static_cast<Origin&>(*object());
}

DocumentObject& ContainerBase::asDocumentObject() const
{
    if (!isADocumentObject())
        throw Base::TypeError("Container::asDocumentObject: Not a document-object!");
    return static_cast<DocumentObject&>(*object());
}

void ContainerBase::check() const
{
    if (!this->object())
        throw NullContainerError("Container is Null");
    if (!(  isADocument()
            || isAGroup() //OriginGroup covered by this too
            || isAnOrigin()   ))
        throw NotAContainerError("Unknown container type");
}

std::vector<DocumentObject*> ContainerBase::recursiveChildren(bool b_dynamic, bool b_static) const
{
    //dependency-loop-proof recursive container tree explorer
    std::unordered_set<DocumentObject*> visited;
    std::vector<App::DocumentObject*> list_of_children;
    //actual traversal is packed into a separate function with extra arguments, for infinite recursion protection.
    recursiveChildrenRec(object(), b_dynamic, b_static, visited, list_of_children);
    return list_of_children;
}

void ContainerBase::recursiveChildrenRec(PropertyContainer* cnt, bool b_dynamic, bool b_static, std::unordered_set<DocumentObject*>& visited, std::vector<DocumentObject*>& result)
{
    //visit static and dynamic children in separate loops
    if (b_static || b_dynamic) { //there can be dynamic children buried under static, so we must always traverse them
        for (App::DocumentObject* child: Container(cnt).staticChildren()){
            if (visited.find(child) == visited.end()){
                visited.insert(child);
                if (b_static)
                    result.push_back(child);
                if (Container::isAContainer(child))
                    recursiveChildrenRec(child, b_dynamic, b_static, visited, result);
            }
        }
    }
    if (b_dynamic) { //all static childern buried under dynamic are considered dynamic, so don't traverse dynamic if only static are requested
        for (App::DocumentObject* child: Container(cnt).dynamicChildren()){
            if (visited.find(child) == visited.end()){
                visited.insert(child);
                result.push_back(child);
                if (Container::isAContainer(child))
                    recursiveChildrenRec(child, true, true, visited, result); //static children of dynamic subcontainers are to be included into dynamic children
            }
        }
    }

}
