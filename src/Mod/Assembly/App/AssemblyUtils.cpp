// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2023 Ondsel <development@ondsel.com>                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/PropertyStandard.h>
// #include <App/DocumentObjectGroup.h>
#include <App/Link.h>
// #include <Base/Console.h>
#include <Base/Placement.h>
#include <Base/Tools.h>
#include <Base/Interpreter.h>

#include <Mod/Part/App/PartFeature.h>

#include "AssemblyUtils.h"

// ======================================= Utils ======================================
/*
namespace Assembly
{

Base::Placement getPlacementFromProp(App::DocumentObject* obj, const char* propName)
{
    Base::Placement plc = Base::Placement();
    auto* propPlacement = dynamic_cast<App::PropertyPlacement*>(obj->getPropertyByName(propName));
    if (propPlacement) {
        plc = propPlacement->getValue();
    }
    return plc;
}

/* // Currently unused
Base::Placement* getTargetPlacementRelativeTo(
    App::DocumentObject* targetObj, App::DocumentObject* part, App::DocumentObject* container,
    bool inContainerBranch, bool ignorePlacement = false)
{
    inContainerBranch = inContainerBranch || (!ignorePlacement && part == container);

    Base::Console().Warning("sub --------------\n");
    if (targetObj == part && inContainerBranch && !ignorePlacement) {
        Base::Console().Warning("found0\n");
        return &getPlacementFromProp(targetObj, "Placement");
    }

    if (auto group = dynamic_cast<App::DocumentObjectGroup*>(part)) {
        for (auto& obj : group->getOutList()) {
            auto foundPlacement = getTargetPlacementRelativeTo(
                targetObj, obj, container, inContainerBranch, ignorePlacement
            );
            if (foundPlacement != nullptr) {
                return foundPlacement;
            }
        }
    }
    else if (auto assembly = dynamic_cast<AssemblyObject*>(part)) {
        Base::Console().Warning("h3\n");
        for (auto& obj : assembly->getOutList()) {
            auto foundPlacement = getTargetPlacementRelativeTo(
                targetObj, obj, container, inContainerBranch
            );
            if (foundPlacement == nullptr) {
                continue;
            }

            if (!ignorePlacement) {
                *foundPlacement = getPlacementFromProp(part, "Placement") * *foundPlacement;
            }

            Base::Console().Warning("found\n");
            return foundPlacement;
        }
    }
    else if (auto link = dynamic_cast<App::Link*>(part)) {
        Base::Console().Warning("h4\n");
        auto linked_obj = link->getLinkedObject();

        if (dynamic_cast<App::Part*>(linked_obj) || dynamic_cast<AssemblyObject*>(linked_obj)) {
            for (auto& obj : linked_obj->getOutList()) {
                auto foundPlacement = getTargetPlacementRelativeTo(
                    targetObj, obj, container, inContainerBranch
                );
                if (foundPlacement == nullptr) {
                    continue;
                }

                *foundPlacement = getPlacementFromProp(link, "Placement") * *foundPlacement;
                return foundPlacement;
            }
        }

        auto foundPlacement = getTargetPlacementRelativeTo(
            targetObj, linked_obj, container, inContainerBranch, true
        );

        if (foundPlacement != nullptr && !ignorePlacement) {
            *foundPlacement = getPlacementFromProp(link, "Placement") * *foundPlacement;
        }

        Base::Console().Warning("found2\n");
        return foundPlacement;
    }

    return nullptr;
}

Base::Placement getGlobalPlacement(App::DocumentObject* targetObj, App::DocumentObject* container =
nullptr) { bool inContainerBranch = container == nullptr; auto rootObjects =
App::GetApplication().getActiveDocument()->getRootObjects(); for (auto& part : rootObjects) { auto
foundPlacement = getTargetPlacementRelativeTo(targetObj, part, container, inContainerBranch); if
(foundPlacement != nullptr) { Base::Placement plc(foundPlacement->toMatrix()); return plc;
        }
    }

    return Base::Placement();
}
*/
/*
double getJointDistance(App::DocumentObject* joint)
{
    double distance = 0.0;

    auto* prop = dynamic_cast<App::PropertyFloat*>(joint->getPropertyByName("Distance"));
    if (prop) {
        distance = prop->getValue();
    }

    return distance;
}

JointType getJointType(App::DocumentObject* joint)
{
    JointType jointType = JointType::Fixed;

    auto* prop = dynamic_cast<App::PropertyEnumeration*>(joint->getPropertyByName("JointType"));
    if (prop) {
        jointType = static_cast<JointType>(prop->getValue());
    }

    return jointType;
}

const char* getElementFromProp(App::DocumentObject* obj, const char* propName)
{
    auto* prop = dynamic_cast<App::PropertyString*>(obj->getPropertyByName(propName));
    if (!prop) {
        return "";
    }

    return prop->getValue();
}

std::string getElementTypeFromProp(App::DocumentObject* obj, const char* propName)
{
    // The prop is going to be something like 'Edge14' or 'Face7'. We need 'Edge' or 'Face'
    std::string elementType;
    for (char ch : std::string(getElementFromProp(obj, propName))) {
        if (std::isalpha(ch)) {
            elementType += ch;
        }
    }
    return elementType;
}

App::DocumentObject* getLinkObjFromProp(App::DocumentObject* joint,
    const char* propLinkName)
{
    auto* propObj = dynamic_cast<App::PropertyLink*>(joint->getPropertyByName(propLinkName));
    if (!propObj) {
        return nullptr;
    }
    return propObj->getValue();
}

App::DocumentObject* getObjFromNameProp(App::DocumentObject* joint,
    const char* pObjName,
    const char* pPart)
{
    auto* propObjName = dynamic_cast<App::PropertyString*>(joint->getPropertyByName(pObjName));
    if (!propObjName) {
        return nullptr;
    }
    std::string objName = std::string(propObjName->getValue());

    App::DocumentObject* containingPart = getLinkObjFromProp(joint, pPart);
    if (!containingPart) {
        return nullptr;
    }

    if (objName == containingPart->getNameInDocument()) {
        return containingPart;
    }

    if (containingPart->getTypeId().isDerivedFrom(App::Link::getClassTypeId())) {
        App::Link* link = dynamic_cast<App::Link*>(containingPart);

        containingPart = link->getLinkedObject();
        if (!containingPart) {
            return nullptr;
        }
    }

    for (auto obj : containingPart->getOutList()) {
        if (objName == obj->getNameInDocument()) {
            return obj;
        }
    }

    return nullptr;
}

App::DocumentObject* getLinkedObjFromNameProp(App::DocumentObject* joint,
    const char* pObjName,
    const char* pPart)
    {
        auto* obj = getObjFromNameProp(joint, pObjName, pPart);
        if (obj) {
            return obj->getLinkedObject(true);
        }
        return nullptr;
    }

} // namespace Assembly
*/
