
#include "PreCompiled.h"

#include "Container.h"

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
        result = asGroup().getObjects();
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
    } else if (isAnOriginGroup()) {
        OriginGroupExtension &g = asOriginGroup();
        if (g.Origin.getValue())
            return std::vector<DocumentObject*>({g.Origin.getValue(),});
        else
            return std::vector<DocumentObject*>();
    } else if (isAGroup()) {
        return std::vector<DocumentObject*>();
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
        //FIXME: make faster, by either using cached inlists, or by documentobjects remembering their groups
        for (DocumentObject* cnt: findAllContainers(*getDocument())){
            for (DocumentObject* child: Container(cnt).allChildren()){
                if (pcObject == child)
                    result.push_back(cnt);
            }
        }
        if (result.size() == 0)
            result.push_back(getDocument());
    } else if (isADocument()) {
        //no parent, return empty list
    } else {
        assert(false /*unexpected type*/);
    }
    return result;
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


Container::Container(PropertyContainer* pcObject)
    :ContainerBase(pcObject)
{
    if (!isNull()) //allow null, but not incorrect type
        check();
}

Container::~Container()
{

}
