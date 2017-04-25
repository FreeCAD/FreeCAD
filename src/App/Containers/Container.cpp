
#include "PreCompiled.h"

#include "Container.h"
#include <Containers/ContainerPy.h>

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/GroupExtension.h>
#include <App/OriginGroupExtension.h>
#include <App/Origin.h>


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
            //FIXME: ignore non-DAGness, maybe, and only test containership loops
            asDocumentObject().testIfLinkDAGCompatible(obj);
        }
        if (isAGroup()){
            if (!asGroup().allowObject(obj))
                throw RejectedByContainerError();
        } else {
            if (!this->canAccept(obj->getTypeId().getName()))
                throw RejectedByContainerError("Container doesn't accept objects of this type");
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

DocumentObject* Container::newObject(const char* sType, const char* pObjectName, bool isNew)
{
    check();
    if (isADocument()){
        return asDocument().newObject(sType, pObjectName, isNew);
    } else if (isAGroup()){
        if (!canAccept(sType) && isNew)
            throw RejectedByContainerError("Container doesn't accept objects of this type");
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
        return asGroup().adoptObject(obj);
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
