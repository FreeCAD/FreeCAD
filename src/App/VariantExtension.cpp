/****************************************************************************
 *   Copyright (c) 2025 Pieter Hijma <info@pieterhijma.net>                 *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include <Base/Console.h>
#include "App/Document.h"
#include "App/Expression.h"
#include "App/ObjectIdentifier.h"
#include "PreCompiled.h"

#ifndef _PreComp_
#include <boost/algorithm/cxx11/copy_if.hpp>
#include <boost/range/algorithm/find.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/for_each.hpp>
#include <boost/range/algorithm/set_algorithm.hpp>
#endif

#include "VariantExtension.h"

using boost::algorithm::copy_if;
using boost::range::find;
using boost::range::find_if;
using boost::range::for_each;
using boost::range::set_difference;
using std::vector;

using namespace App;

FC_LOG_LEVEL_INIT("App", true, true, true)

EXTENSION_PROPERTY_SOURCE(App::VariantExtension, App::GroupExtension)

VariantExtension::VariantExtension()
{
    EXTENSION_ADD_PROPERTY_TYPE(Support, (nullptr, nullptr), "", (App::PropertyType)(App::Prop_None), "Support of the geometry");
}


void VariantExtension::removeDynamicProperties(DocumentObject* obj) const
{
    const std::vector<std::string>& namesDynamicProps =
        obj->getDynamicPropertyNames();
    for (const auto& name : namesDynamicProps) {
        obj->removeDynamicProperty(name.c_str());
    }
}


void VariantExtension::extensionOnChanged(const App::Property* prop)
{
    if (prop == &Support) {
        removeDynamicProperties(getExtendedObject());
        adoptedExposedProps.clear();
        getExtendedObject()->touch();
    }

    auto it = find_if(adoptedExposedProps,
                      [prop](const App::Property* p) {
                          return strcmp(prop->getName(), p->getName()) == 0;
                      });
    if (it != adoptedExposedProps.end()) {
        getExtendedObject()->touch();
    }
    GroupExtension::extensionOnChanged(prop);
}

void VariantExtension::adoptProperty(const App::Property* prop)
{
    App::Property* newProp =
        getExtendedObject()->addDynamicProperty(prop->getTypeId().getName(), prop->getName(),
                                                prop->getGroup(), prop->getDocumentation());
    std::unique_ptr<App::Property> pcopy(prop->Copy());
    newProp->Paste(*pcopy);
}

void VariantExtension::adoptExposedProps(const DocumentObject* support)
{
    std::vector<App::Property*> exposedProps;
    support->getExposedPropertyList(exposedProps);

    std::set<const App::Property*> currentAdoptedExposedProps(adoptedExposedProps);
    adoptedExposedProps.clear();

    for (const auto prop : exposedProps) {
        if (currentAdoptedExposedProps.find(prop) == currentAdoptedExposedProps.end()) {
            adoptProperty(prop);
        }
        adoptedExposedProps.insert(prop);
    }

    std::set<const App::Property*> propsToRemove;
    set_difference(currentAdoptedExposedProps,
                   adoptedExposedProps,
                   std::inserter(propsToRemove, propsToRemove.end()));
    for_each(propsToRemove, [this](const App::Property* prop) {
        getExtendedObject()->removeDynamicProperty(prop->getName());
    });
}

std::vector<DocumentObject*> VariantExtension::getObjectsRequiringVarSets()
{
    std::vector<DocumentObject*> objectsRequiringContexts;
    DocumentObject* support = Support.getValue();
    if (support == nullptr) { return objectsRequiringContexts; }

    DocumentObject* extendedObject = getExtendedObject();
    std::vector<DocumentObject*> outList = support->getOutList();
    copy_if(outList, std::back_inserter(objectsRequiringContexts),
            [extendedObject](auto obj) {
        return obj != extendedObject;
    });
    return objectsRequiringContexts;
}


std::set<std::string> VariantExtension::getNamesPropsWithExposedExprs(const DocumentObject* obj,
                                                                      const DocumentObject* support) const
{
    // get the names of properties that have expressions that refer to an
    // exposed property in support.
    std::set<std::string> namesPropsExposed;

    auto insertNamePropObjWhenExposed = [&](const auto& deps, const std::string& namePropObj) {
        for (const auto& [namePropSupport, _] : deps) {
            if (support->isExposed(namePropSupport.c_str())) {
                namesPropsExposed.insert(namePropObj);
            }
        }
    };

    for (const auto& [objId, expr] : obj->ExpressionEngine.getExpressions()) {
        ExpressionDeps deps;
        expr->getDeps(deps);

        // not a direct deps.find because of difference in const between support and deps
        auto it = find_if(deps, [&](const auto& pair) {
            return pair.first == support;
        });
        if (it != deps.end()) {
            insertNamePropObjWhenExposed(it->second, objId.getPropertyName());
        }
    }

    return namesPropsExposed;
}


Property* VariantExtension::createPropertyContext(DocumentObject* objContext,
                                                  const DocumentObject* obj, const char* name)
{
    Property* prop = obj->getPropertyByName(name);
    Property* propContext = objContext->addDynamicProperty(prop->getTypeId().getName(), name,
                                                           prop->getGroup(), prop->getDocumentation());
    std::unique_ptr<App::Property> pcopy(prop->Copy());
    propContext->Paste(*pcopy);

    auto renameObjIds = [objContext, name](const auto& expressions) {
        for (const auto& [oldObjId, expression] : expressions) {
            if (oldObjId.getPropertyName() == name) {
                ObjectIdentifier newObjId(oldObjId);
                newObjId.setDocumentObjectName(objContext);
                std::shared_ptr<Expression> copiedExpr(expression->copy());
                objContext->setExpression(newObjId, copiedExpr);
            }
        }
    };

    renameObjIds(obj->ExpressionEngine.getExpressions());
    objContext->ExpressionEngine.execute(PropertyExpressionEngine::ExecuteNonOutput);

    return propContext;
}

void VariantExtension::adoptExpressionEngine(DocumentObject* objContext,
                                             const DocumentObject* obj,
                                             const DocumentObject* support) const
{
    std::set<std::string> namesPropsWithExposedExprs =
        getNamesPropsWithExposedExprs(obj, support);

    for (const auto& nameProp : namesPropsWithExposedExprs) {
        createPropertyContext(objContext, obj, nameProp.c_str());
    }
}

DocumentObject* VariantExtension::createNewContext(DocumentObject* obj, Document* doc)
{
    std::string name(obj->getNameInDocument());
    std::string newName = name + "Context";
    DocumentObject* objContext = doc->addObject("App::VarSet", newName.c_str());

    this->addObject(objContext);
    objToContext[obj] = objContext;
    contextToObj[objContext] = obj;

    return objContext;
}

void VariantExtension::arrangeContext(DocumentObject* obj, Document* doc, DocumentObject* support)
{
    DocumentObject* objContext = objToContext[obj];

    if (objContext) {
        removeDynamicProperties(objContext);
    }
    else {
        objContext = createNewContext(obj, doc);
    }
    adoptExpressionEngine(objContext, obj, support);
}

void VariantExtension::arrangeContexts()
{
    std::vector<DocumentObject*> objsRequiringVarSets = getObjectsRequiringVarSets();
    DocumentObject* extendedObject = getExtendedObject();
    Document* doc = extendedObject->getDocument();
    DocumentObject* support = Support.getValue();

    for (auto obj : objsRequiringVarSets) {
        arrangeContext(obj, doc, support);
    }
    objToContext[support] = extendedObject;
    contextToObj[extendedObject] = support;
}

std::vector<DocumentObject*> VariantExtension::getSortedDependencies(DocumentObject* support)
{
    const vector<DocumentObject*>& outList = support->getOutList();
    return Document::getDependencyList(outList, Document::DepSort);
}

void VariantExtension::pushContexts(const std::vector<DocumentObject*>& objs)
{
    for (const auto obj : objs) {
        DocumentObject* context = objToContext[obj];
        obj->pushContext(context);
    }
}

void VariantExtension::popContexts(const std::vector<DocumentObject*>& objs)
{
    for (const auto obj : objs) {
        DocumentObject* context = objToContext[obj];
        obj->popContext();
        context->purgeTouched();
    }
}

void VariantExtension::executeObjects(const std::vector<DocumentObject*>& objs)
{
    for (const auto obj : objs) {
        FC_MSG("  executing " << obj->getFullName());

        if (DocumentObject* context = objToContext[obj]; context != nullptr) {
            obj->executeWithContext(context);
        }
    }
}

DocumentObjectExecReturn* VariantExtension::extensionExecute()
{
    FC_MSG("VariantExtension::extensionExecute()");
    DocumentObject* support = Support.getValue();

    auto exit = [this]() {
        FC_MSG("end VariantExtension::extensionExecute()");
        return GroupExtension::extensionExecute();
    };

    if (support == nullptr) {
        return exit();
    }

    adoptExposedProps(support);
    arrangeContexts();

    auto objsWithContexts = getObjectsRequiringVarSets();
    objsWithContexts.push_back(support);

    auto sortedDeps = getSortedDependencies(support);
    auto sortedObjs = sortedDeps;
    sortedObjs.push_back(support);

    pushContexts(objsWithContexts);
    executeObjects(sortedObjs);
    popContexts(objsWithContexts);

    return exit();
}
