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

#include "Container.h"
#include <Containers/ContainerPy.h>
#include "Exceptions.h"

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/GroupExtension.h>
#include <App/OriginGroupExtension.h>
#include <App/Origin.h>

//includes for Py methods
#include <App/DocumentPy.h>
#include <App/DocumentObjectPy.h>


using namespace App;

TYPESYSTEM_SOURCE(App::Container, App::ContainerBase)

std::vector<DocumentObject*> Container::dynamicChildren() const
{
    if (isNull())
        return std::vector<DocumentObject*>();
    std::vector<DocumentObject*> result;
    if(isADocument()){
        //fill a set of all objects. Then, for each container object, query its children and withdraw them from the set.
        std::set<DocumentObject*> resultset;
        auto allObjects = asDocument().getObjects();
        resultset.insert(allObjects.begin(), allObjects.end());

        for (DocumentObject* cnt: findAllContainers(*getDocument())){
            for (DocumentObject* child: Container(cnt).allChildren()){
                resultset.erase(child);
            }
        }

        std::copy(resultset.begin(), resultset.end(), std::back_inserter(result));
    } else if (isAGroup()) {
        result = asGroup().getDynamicObjects();
    } else if (isAnOrigin()) {
        //no dynamic children
    } else {
        assert(false /*unexpected type*/);
    }
    return result;
}

std::vector<DocumentObject*> Container::staticChildren() const
{
    if (isNull())
        return std::vector<DocumentObject*>();
    if (isADocument()) {
        return std::vector<DocumentObject*>();
    } else if (isAGroup()) {
        return asGroup().getStaticObjects();
    } else if (isAnOrigin()) {
        return asOrigin().OriginFeatures.getValues();
    } else {
        assert(false /*unexpected type*/);
        return std::vector<DocumentObject*>();
    }
}

std::vector<PropertyContainer*> Container::parents() const
{
    check();
    std::vector<PropertyContainer*> result;
    if (isADocumentObject()){
        result = getContainersOf(&asDocumentObject());
    } else if (isADocument()) {
        //no parent, return empty list
    } else {
        assert(false /*unexpected type*/);
    }
    return result;
}

bool Container::canAccept(DocumentObject* obj, bool b_throw) const
{
    try {
        if (!obj){
            throw Base::ValueError("Null object!");
        }
        if (isNull()){
            throw NullContainerError("Null container can't take any objects");
        }
        if (isADocumentObject()) {
            //the idea is to prevent a container to parent itself (directly or indirectly)
            //FIXME: ignore non-DAGness, maybe, and only test containership loops. Testing full DAG compatibility can be slow.
            if (!asDocumentObject().testIfLinkDAGCompatible(obj)){
                std::stringstream msg;
                msg << "Adding '" << (obj->getNameInDocument() ? obj->getNameInDocument() : "")
                    << "' to container " << getName() << " will result in dependency loop";
                throw ContainerTreeError(msg.str());
            }
        }
        if (isAGroup()){
            if (!asGroup().allowObject(obj)) {
                std::stringstream msg;
                msg << "Container " << getName() << " refuses to accept '" << (obj->getNameInDocument() ? obj->getNameInDocument() : "") << "'";
                throw RejectedByContainerError(msg.str());
            }
        } else if (isAnOrigin()) {
            throw ContainerUnsupportedError("Origin can't accept any objects");
        } else {
            if (!this->canAccept(obj->getTypeId().getName())){
                std::stringstream msg;
                msg << "Container " << getName() << " refuses to accept objects of type " << obj->getTypeId().getName();
                throw RejectedByContainerError(msg.str());
            }
        }
        return true;
    } catch (ContainerError&){
        if (b_throw)
            throw;
        else
            return false;
    }
}

bool Container::canAccept(const char* type, const char* pytype) const
{
    if (isNull())
        return false;
    Base::Type t = Base::Type::fromName(type);
    if (isADocument())
        return t.isDerivedFrom(DocumentObject::getClassTypeId());
    else if (isAGroup())
        return asGroup().allowObject(type, pytype);
    else if (isAnOrigin())
        return false;
    else
        assert(false /*unexpected type*/);
    return false;
}

DocumentObject* Container::newObject(const char* sType, const char* pObjectName, const char* pytype, bool isNew)
{
    check();
    if (isADocument()){
        return asDocument().newObject(sType, pObjectName, isNew);
    } else if (isAGroup()){
        if (!canAccept(sType, pytype) && isNew){
            std::stringstream msg;
            msg << "Container " << getName() << " refuses to accept objects of type " << sType;
            if (pytype)
                msg << "/" << pytype;
            throw RejectedByContainerError(msg.str());
        }
        if (!isNew)
            throw Base::ValueError("Can't create a non-new object in a group."); //FIXME: add support of isNew to GroupExtension
        return asGroup().newObject(sType, pObjectName);
    } else if (isAnOrigin()){
        throw RejectedByContainerError("Can't add objects to Origin");
    } else {
        assert(false /*unexpected type*/);
    }
    throw ContainerUnsupportedError("Unexpected error");
}

bool Container::adoptObject(DocumentObject* obj)
{
    check();
    canAccept(obj, /*b_throw=*/ true);
    if (obj->getDocument() != this->getDocument())
        return false;
    if (isADocument()){
        return (getContainerOf(obj) == this->getDocument());
    } else if (isAGroup()){
        auto containers = getContainersOf(obj);
        if (containers.size() == 0 || (containers.size() == 1 && Container(containers[0]).isADocument())) {
            asGroup().addObject(obj);
            return true;
        } else {
            return std::find(containers.begin(), containers.end(), this->object()) != containers.end();
        }
    } else if (isAnOrigin()){
        throw ContainerUnsupportedError("Can't add objects to Origin");
    } else {
        assert(false /*unexpected type*/);
    }
    throw ContainerUnsupportedError("Unexpected error");

}

void Container::addObject(DocumentObject* obj)
{
    //FIXME: check if can accept
    check();
    if (obj->getDocument() != this->getDocument())
        throw AlreadyInContainerError("Object is in another document");
    App::PropertyContainer* oldcnt = getContainerOf(obj);
    if (oldcnt == object())
        return; //already in, nothing to do
    if (oldcnt && oldcnt != this->getDocument() && oldcnt != object())
        Container(oldcnt).withdrawObject(obj);

    bool adopted = adoptObject(obj);
    assert(adopted);
    if (!adopted)
        throw ContainerUnsupportedError("Failed to add the object");
}

void Container::withdrawObject(DocumentObject* obj)
{
    check();
    if (!obj)
        throw Base::ValueError("Null object!");
    if (!hasDynamicObject(obj)){
        std::stringstream msg;
        msg << "Object named '";
        if (obj->getNameInDocument())
            msg << obj->getNameInDocument();
        if (hasStaticObject(obj)){
            msg << "' cannot be withdrawn from " << getName();
            throw SpecialChildError(msg.str());
        } else {
            msg << "' not found in " << getName();
            throw ObjectNotFoundError(msg.str());
        }
    }
    if (isADocument()){
        //nothing to do...
    } else if (isAGroup()) {
        asGroup().removeObject(obj);
    } else if (isAnOrigin()) {
        //should have been filtered out already. But...
        throw ContainerUnsupportedError("Cannot add or remove stuff from Origins");
    } else {
        assert(false /*unexpected type*/);
        throw ContainerUnsupportedError("Unexpected error");
    }
}

void Container::deleteObject(DocumentObject* obj)
{
    withdrawObject(obj);
    Document* doc = obj->getDocument();
    if (doc)
        doc->remObject(obj->getNameInDocument());
    else
        throw ObjectNotFoundError("Object is not in any document");
}

PyObject*Container::getPyObject()
{
    Container* cpy = new Container(object());
    return new ContainerPy(cpy);
}

bool Container::isAContainer(PropertyContainer* object)
{
    if (object == nullptr)
        return false;
    try {
        Container tmp(object);
    } catch (ContainerError&){
        return false;
    }
    return true;
}

std::vector<DocumentObject*> Container::findAllContainers(Document& doc)
{
    std::vector<DocumentObject*> containers = doc.getObjectsWithExtension(GroupExtension::getExtensionClassTypeId());
    std::vector<DocumentObject*> origins = doc.getObjectsOfType(Origin::getClassTypeId());
    containers.insert(containers.end(), origins.begin(), origins.end());
    return containers;
}

std::vector<PropertyContainer*> Container::getContainersOf(DocumentObject* obj)
{
    std::vector<PropertyContainer*> result;
    App::Document* doc = obj->getDocument();
    if (!doc)
        return result;
    //FIXME: make faster, by either using cached inlists, or by documentobjects remembering their groups
    for (DocumentObject* cnt: findAllContainers(*doc)){
        for (DocumentObject* child: Container(cnt).allChildren()){
            if (obj == child)
                result.push_back(cnt);
        }
    }
    if (result.size() == 0)
        result.push_back(doc);
    return result;
}

PropertyContainer* Container::getContainerOf(DocumentObject* obj)
{
    auto containers = getContainersOf(obj);
    if (containers.size() == 0){
        return nullptr;
    } else if (containers.size() == 1){
        return containers[0];
    } else {
        std::stringstream msg;
        msg << "getContainerOf: object " << obj->getNameInDocument() << " is contained by more than one container";
        throw ContainerTreeError(msg.str());
    }
}

Container::Container(PropertyContainer* pcObject)
    :ContainerBase(pcObject)
{
    if (!isNull()) //allow null, but not incorrect type
        check();
}

Container::~Container()
{

}




//--------------------------Py methods---------------------------

PyMethodDef Container::PyMethods[] = {
    {"isAContainer",       (PyCFunction) Container::sIsAConainer,       1,
     "isAContainer(object): tests if given object is recognized as being a container."},
    {"findAllContainers",       (PyCFunction) Container::sFindAllContainers,       1,
     "findAllContainers(document): returns all objects from given document that are containers (both direct children and nested ones).\n"
     "Returns plain objects (not wrapped in Container() interface). Document itself is not included into the list."},
    {"getContainersOf",       (PyCFunction) Container::sGetContainersOf,       1,
     "getContainersOf(object): finds all containers that this object is in.\n"
     "Typically, there's just one. But if container tree is broken, there can be more than one."},
    {"getContainerOf",       (PyCFunction) Container::sGetContainerOf,       1,
     "getContainerOf(object): returns the container that has given object.\n"
     "If object is contained by more than one container, ContainerTreeError is raised.\n"
     "If applied to a root (i.e. object is a document), None is returned."},

    {NULL, NULL, 0, NULL}		/* Sentinel */
};


PyObject* Container::sIsAConainer(PyObject* self, PyObject* args, PyObject* kwd)
{
    PyObject* obj = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &(PropertyContainerPy::Type), &obj))
        return 0;

    try {
        assert(obj);
        return Py::new_reference_to(Py::Boolean(
            Container::isAContainer(static_cast<PropertyContainerPy*>(obj)->getPropertyContainerPtr())   ));
    } CONTAINERPY_STDCATCH_METH;
}

PyObject* Container::sFindAllContainers(PyObject* self, PyObject* args, PyObject* kwd)
{
    PyObject* obj = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &(DocumentPy::Type), &obj))
        return 0;

    try {
        assert(obj);
        auto objs = Container::findAllContainers(*static_cast<DocumentPy*>(obj)->getDocumentPtr());
        Py::List ret;
        for (DocumentObject* cnt : objs){
            assert(cnt);
            ret.append(Py::asObject(cnt->getPyObject()));
        }
        return Py::new_reference_to(ret);
    } CONTAINERPY_STDCATCH_METH;
}

PyObject* Container::sGetContainersOf(PyObject* self, PyObject* args, PyObject* kwd)
{
    PyObject* obj = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &(PropertyContainerPy::Type), &obj))
        return 0;

    try {
        assert(obj);
        DocumentObjectPy* dobj = dynamic_cast<DocumentObjectPy*>(static_cast<PropertyContainerPy*>(obj));
        if (!dobj) //if Document was supplied
            return Py::new_reference_to(Py::List());

        auto objs = Container::getContainersOf(dobj->getDocumentObjectPtr());
        Py::List ret;
        for (PropertyContainer* cnt : objs){
            assert(cnt);
            ret.append(Py::asObject(Container(cnt).getPyObject()));
        }
        return Py::new_reference_to(ret);
    } CONTAINERPY_STDCATCH_METH;
}

PyObject* Container::sGetContainerOf(PyObject* self, PyObject* args, PyObject* kwd)
{
    PyObject* obj = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &(PropertyContainerPy::Type), &obj))
        return 0;

    try {
        assert(obj);
        DocumentObjectPy* dobj = dynamic_cast<DocumentObjectPy*>(static_cast<PropertyContainerPy*>(obj));
        if (!dobj) //if Document was supplied
            return Py::new_reference_to(Py::None());

        PropertyContainer* cnt = Container::getContainerOf(dobj->getDocumentObjectPtr());
        if (cnt)
            return Container(cnt).getPyObject();
        else
            return Py::new_reference_to(Py::None());
    } CONTAINERPY_STDCATCH_METH;
}
