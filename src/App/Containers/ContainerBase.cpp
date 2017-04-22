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

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/GroupExtension.h>
#include <App/OriginGroupExtension.h>
#include <App/Origin.h>

#include <unordered_set>

using namespace App;

TYPESYSTEM_SOURCE_ABSTRACT(App::ContainerBase, Base::BaseClass)

ContainerBase::~ContainerBase()
{

}

std::string ContainerBase::getName() const
{
    check();
    if (isADocument())
        return asDocument().getName();
    else if (isADocumentObject())
        return asDocumentObject().getNameInDocument();
    else {
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
        msg << "Object named '" << objectName << "'' not found in " << getName();
        throw ObjectNotFoundError(msg.str());
    }
    return obj;
}

Document* ContainerBase::getDocument() const
{
    check();
    if (isADocument())
        return static_cast<Document*>(pcObject);
    else
        return static_cast<DocumentObject*>(pcObject)->getDocument();
}

bool ContainerBase::isADocument() const
{
    return pcObject && pcObject->isDerivedFrom(Document::getClassTypeId());
}

bool ContainerBase::isAGroup() const
{
    if (pcObject && pcObject->isDerivedFrom(DocumentObject::getClassTypeId())){
        auto dobj = static_cast<DocumentObject*>(pcObject);
        if (dobj->hasExtension(GroupExtension::getExtensionClassTypeId()))
            return true;
    }
    return false;
}

bool ContainerBase::isAnOriginGroup() const
{
    if (pcObject && pcObject->isDerivedFrom(DocumentObject::getClassTypeId())){
        auto dobj = static_cast<DocumentObject*>(pcObject);
        if (dobj->hasExtension(OriginGroupExtension::getExtensionClassTypeId()))
            return true;
    }
    return false;
}

bool ContainerBase::isAnOrigin() const
{
    return pcObject && pcObject->isDerivedFrom(Origin::getClassTypeId());
}

Document& ContainerBase::asDocument() const
{
    if (!isADocument())
        throw Base::TypeError("Container::asDocument: Not a document!");
    return static_cast<Document&>(*pcObject);
}

GroupExtension& ContainerBase::asGroup() const
{
    if (!isAGroup())
        throw Base::TypeError("Container::asGroup: Not a group!");
    return static_cast<GroupExtension&>(*static_cast<DocumentObject*>(pcObject)->getExtensionByType<GroupExtension>());
}

OriginGroupExtension& ContainerBase::asOriginGroup() const
{
    if (!isAnOriginGroup())
        throw Base::TypeError("Container::asOriginGroup: Not an origingroup!");
    return static_cast<OriginGroupExtension&>(*static_cast<DocumentObject*>(pcObject)->getExtensionByType<OriginGroupExtension>());
}

Origin& ContainerBase::asOrigin() const
{
    if (!isAnOrigin())
        throw Base::TypeError("Container::asOrigin: Not an origin!");
    return static_cast<Origin&>(*pcObject);
}

DocumentObject& ContainerBase::asDocumentObject() const
{
    if (!isADocumentObject())
        throw Base::TypeError("Container::asDocumentObject: Not a document-object!");
    return static_cast<DocumentObject&>(*pcObject);
}

void ContainerBase::check() const
{
    if (!this->pcObject)
        throw NullContainerError("Container is Null");
    if (!(  isADocument()
            || isAGroup() //OriginGroup covered by this too
            || isAnOrigin()   ))
        throw NotAContainerError("Unknown container type");
}

std::vector<DocumentObject*> ContainerBase::recursiveChildren(bool b_dynamic, bool b_static) const
{
    //dependency-loop-proof non-recursive tree explorer... copied over from Mod/Show/DepGraphTools.py
    typedef std::vector<PropertyContainer*> List;
    typedef std::unordered_set<PropertyContainer*> Set;
    List list_traversing_now; //list of containers
    Set set_of_visited;
    std::vector<App::DocumentObject*> list_of_children;

    list_traversing_now.push_back(pcObject); //seed

    //outline: loop through all children of containers in list_traversing_now.
    //If a container is encountered among them, add it to
    //list_to_be_traversed_next. Repeat until no more containers are
    //encountered. This results in a wave-like collecting: first, shallow
    //children are gathered, then deeper and deeper.

    //For guarding against dependency-loops, a set of containers already visited is kept around.

    while ((list_traversing_now.size()) > 0){
        List list_to_be_traversed_next;
        for (PropertyContainer* cnt :list_traversing_now){
            //visit static and dynamic children in separate loops
            for (App::DocumentObject* child: Container(cnt).dynamicChildren()){
                if (set_of_visited.find(child) != set_of_visited.end()){
                    set_of_visited.insert(child);
                    if (b_dynamic)
                        list_of_children.push_back(child);
                    if (Container::isAContainer(child))
                        list_to_be_traversed_next.push_back(child);
                }
            }
            for (App::DocumentObject* child: Container(cnt).staticChildren()){
                if (set_of_visited.find(child) != set_of_visited.end()){
                    set_of_visited.insert(child);
                    if (b_static)
                        list_of_children.push_back(child);
                    if (Container::isAContainer(child))
                        list_to_be_traversed_next.push_back(child);
                }
            }
        }
        std::swap(list_traversing_now, list_to_be_traversed_next); list_to_be_traversed_next.clear();
    }
    return list_of_children;
}

//------------------------------------------------------------------------------------------------
TYPESYSTEM_SOURCE(App::ContainerError             , Base::Exception)
TYPESYSTEM_SOURCE(App::ContainerTreeError         , App::ContainerError)
TYPESYSTEM_SOURCE(App::AlreadyInContainerError    , App::ContainerError)
TYPESYSTEM_SOURCE(App::ContainerUnsupportedError  , App::ContainerError)
TYPESYSTEM_SOURCE(App::NotAContainerError         , App::ContainerError)
TYPESYSTEM_SOURCE(App::SpecialChildError          , App::ContainerError)
TYPESYSTEM_SOURCE(App::NullContainerError         , App::ContainerError)
TYPESYSTEM_SOURCE(App::ObjectNotFoundError        , App::ContainerError)

void ContainerError::initContainerExceptionTypes()
{
    ContainerError            ::init();
    ContainerTreeError        ::init();
    AlreadyInContainerError   ::init();
    ContainerUnsupportedError ::init();
    NotAContainerError        ::init();
    SpecialChildError         ::init();
    NullContainerError        ::init();
    ObjectNotFoundError       ::init();
}
