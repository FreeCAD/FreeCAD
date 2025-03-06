// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2024 Ondsel <development@ondsel.com>                     *
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
#include <cmath>
#include <vector>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObjectGroup.h>
#include <App/FeaturePythonPyImp.h>
#include <App/Link.h>
#include <App/PropertyPythonObject.h>
#include <Base/Console.h>
#include <Base/Placement.h>
#include <Base/Rotation.h>
#include <Base/Tools.h>
#include <Base/Interpreter.h>

#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/Part/App/DatumFeature.h>

#include "AssemblyObject.h"
#include "AssemblyUtils.h"
#include "JointGroup.h"

#include "AssemblyLink.h"
#include "AssemblyLinkPy.h"

namespace PartApp = Part;

using namespace Assembly;

// ================================ Assembly Object ============================

PROPERTY_SOURCE(Assembly::AssemblyLink, App::Part)

AssemblyLink::AssemblyLink()
{
    ADD_PROPERTY_TYPE(Rigid,
                      (true),
                      "General",
                      (App::PropertyType)(App::Prop_None),
                      "If the sub-assembly is set to Rigid, it will act "
                      "as a rigid body. Else its joints will be taken into account.");

    ADD_PROPERTY_TYPE(LinkedObject,
                      (nullptr),
                      "General",
                      (App::PropertyType)(App::Prop_None),
                      "The linked assembly.");
}

AssemblyLink::~AssemblyLink() = default;

PyObject* AssemblyLink::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new AssemblyLinkPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

App::DocumentObjectExecReturn* AssemblyLink::execute()
{
    updateContents();

    return App::Part::execute();
}

void AssemblyLink::onChanged(const App::Property* prop)
{
    if (App::GetApplication().isRestoring()) {
        App::Part::onChanged(prop);
        return;
    }

    if (prop == &Rigid) {
        Base::Placement movePlc;

        if (Rigid.getValue()) {
            // movePlc needs to be computed before updateContents.
            if (!objLinkMap.empty()) {
                auto firstElement = *objLinkMap.begin();

                App::DocumentObject* obj = firstElement.first;
                App::DocumentObject* link = firstElement.second;
                auto* prop =
                    dynamic_cast<App::PropertyPlacement*>(obj->getPropertyByName("Placement"));
                auto* prop2 =
                    dynamic_cast<App::PropertyPlacement*>(link->getPropertyByName("Placement"));
                if (prop && prop2) {
                    movePlc = prop2->getValue() * prop->getValue().inverse();
                }
            }
        }

        updateContents();

        auto* propPlc = dynamic_cast<App::PropertyPlacement*>(getPropertyByName("Placement"));
        if (!propPlc) {
            return;
        }

        if (!Rigid.getValue()) {
            // when the assemblyLink becomes flexible, we need to make sure its placement is
            // identity or it's going to mess up moving parts placement within.
            Base::Placement plc = propPlc->getValue();
            if (!plc.isIdentity()) {
                propPlc->setValue(Base::Placement());

                // We need to apply the placement of the assembly link to the children or they will
                // move.
                std::vector<App::DocumentObject*> group = Group.getValues();
                for (auto* obj : group) {
                    if (!obj->isDerivedFrom<App::Part>() && !obj->isDerivedFrom<PartApp::Feature>()
                        && !obj->isDerivedFrom<App::Link>()) {
                        continue;
                    }

                    auto* prop =
                        dynamic_cast<App::PropertyPlacement*>(obj->getPropertyByName("Placement"));
                    if (prop) {
                        prop->setValue(plc * prop->getValue());
                    }
                }

                AssemblyObject::redrawJointPlacements(getJoints());
            }
        }
        else {
            // For the assemblylink not to move to origin, we need to update its placement.
            if (!movePlc.isIdentity()) {
                propPlc->setValue(movePlc);
            }
        }

        return;
    }
    App::Part::onChanged(prop);
}

void AssemblyLink::updateContents()
{
    synchronizeComponents();

    if (isRigid()) {
        ensureNoJointGroup();
    }
    else {
        synchronizeJoints();
    }

    purgeTouched();
}

void AssemblyLink::synchronizeComponents()
{
    App::Document* doc = getDocument();

    AssemblyObject* assembly = getLinkedAssembly();
    if (!assembly) {
        return;
    }

    objLinkMap.clear();

    std::vector<App::DocumentObject*> assemblyGroup = assembly->Group.getValues();
    std::vector<App::DocumentObject*> assemblyLinkGroup = Group.getValues();

    // We check if a component needs to be added to the AssemblyLink
    for (auto* obj : assemblyGroup) {
        if (!obj->isDerivedFrom<App::Part>() && !obj->isDerivedFrom<PartApp::Feature>()
            && !obj->isDerivedFrom<App::Link>()) {
            continue;
        }

        // Note, the user can have nested sub-assemblies.
        // In which case we need to add an AssemblyLink and not a Link.
        App::DocumentObject* link = nullptr;
        bool found = false;
        for (auto* obj2 : assemblyLinkGroup) {
            App::DocumentObject* linkedObj;

            auto* subAsmLink = dynamic_cast<AssemblyLink*>(obj2);
            auto* link2 = dynamic_cast<App::Link*>(obj2);
            if (subAsmLink) {
                linkedObj = subAsmLink->getLinkedObject2(false);  // not recursive
            }
            else if (link2) {
                linkedObj = link2->getLinkedObject(false);  // not recursive
            }
            else {
                // We consider only Links and AssemblyLinks in the AssemblyLink.
                continue;
            }


            if (linkedObj == obj) {
                found = true;
                link = obj2;
                break;
            }
        }
        if (!found) {
            // Add a link or a AssemblyLink to it in the AssemblyLink.
            if (obj->isDerivedFrom<AssemblyLink>()) {
                auto* asmLink = static_cast<AssemblyLink*>(obj);
                auto* subAsmLink = new AssemblyLink();
                doc->addObject(subAsmLink, obj->getNameInDocument());
                subAsmLink->LinkedObject.setValue(obj);
                subAsmLink->Rigid.setValue(asmLink->Rigid.getValue());
                subAsmLink->Label.setValue(obj->Label.getValue());
                addObject(subAsmLink);
                link = subAsmLink;
            }
            else {
                auto* appLink = new App::Link();
                doc->addObject(appLink, obj->getNameInDocument());
                appLink->LinkedObject.setValue(obj);
                appLink->Label.setValue(obj->Label.getValue());
                addObject(appLink);
                link = appLink;
            }
        }

        objLinkMap[obj] = link;
        // If the assemblyLink is rigid, then we keep the placement synchronized.
        if (isRigid()) {
            auto* plcProp =
                dynamic_cast<App::PropertyPlacement*>(obj->getPropertyByName("Placement"));
            auto* plcProp2 =
                dynamic_cast<App::PropertyPlacement*>(link->getPropertyByName("Placement"));
            if (plcProp && plcProp2) {
                if (!plcProp->getValue().isSame(plcProp2->getValue())) {
                    plcProp2->setValue(plcProp->getValue());
                }
            }
        }
    }

    // We check if a component needs to be removed from the AssemblyLink
    for (auto* link : assemblyLinkGroup) {
        // We don't need to update assemblyLinkGroup after the addition since we're not removing
        // something we just added.

        if (objLinkMap.find(link) != objLinkMap.end()) {
            doc->removeObject(link->getNameInDocument());
        }

        /*if (!link->isDerivedFrom<App::Link>() && !link->isDerivedFrom<AssemblyLink>()) {
            // AssemblyLink should contain only Links or assembly links.
            continue;
        }

        auto* linkedObj = link->getLinkedObject(false);  // not recursive

        bool found = false;
        for (auto* obj2 : assemblyGroup) {
            if (obj2 == linkedObj) {
                found = true;
                break;
            }
        }
        if (!found) {
            doc->removeObject(link->getNameInDocument());
        }*/
    }
}

namespace
{
template<typename T>
void copyPropertyIfDifferent(App::DocumentObject* source,
                             App::DocumentObject* target,
                             const char* propertyName)
{
    auto sourceProp = dynamic_cast<T*>(source->getPropertyByName(propertyName));
    auto targetProp = dynamic_cast<T*>(target->getPropertyByName(propertyName));
    if (sourceProp && targetProp && sourceProp->getValue() != targetProp->getValue()) {
        targetProp->setValue(sourceProp->getValue());
    }
}

std::string removeUpToName(const std::string& sub, const std::string& name)
{
    size_t pos = sub.find(name);
    if (pos != std::string::npos) {
        // Move the position to the character after the found substring and the following '.'
        pos += name.length() + 1;
        if (pos < sub.length()) {
            return sub.substr(pos);
        }
    }
    // If s2 is not found in s1, return the original string
    return sub;
}

std::string
replaceLastOccurrence(const std::string& str, const std::string& oldStr, const std::string& newStr)
{
    size_t pos = str.rfind(oldStr);
    if (pos != std::string::npos) {
        std::string result = str;
        result.replace(pos, oldStr.length(), newStr);
        return result;
    }
    return str;
}
};  // namespace

void AssemblyLink::synchronizeJoints()
{
    App::Document* doc = getDocument();
    AssemblyObject* assembly = getLinkedAssembly();
    if (!assembly) {
        return;
    }

    JointGroup* jGroup = ensureJointGroup();

    std::vector<App::DocumentObject*> assemblyJoints =
        assembly->getJoints(assembly->isTouched(), false, false);
    std::vector<App::DocumentObject*> assemblyLinkJoints = getJoints();

    // We delete the excess of joints if any
    for (size_t i = assemblyJoints.size(); i < assemblyLinkJoints.size(); ++i) {
        doc->removeObject(assemblyLinkJoints[i]->getNameInDocument());
    }

    // We make sure the joints match.
    for (size_t i = 0; i < assemblyJoints.size(); ++i) {
        App::DocumentObject* joint = assemblyJoints[i];
        App::DocumentObject* lJoint;
        if (i < assemblyLinkJoints.size()) {
            lJoint = assemblyLinkJoints[i];
        }
        else {
            auto ret = doc->copyObject({joint});
            if (ret.size() != 1) {
                continue;
            }
            lJoint = ret[0];
            jGroup->addObject(lJoint);
        }

        // Then we have to check the properties one by one.
        copyPropertyIfDifferent<App::PropertyBool>(joint, lJoint, "Activated");
        copyPropertyIfDifferent<App::PropertyFloat>(joint, lJoint, "Distance");
        copyPropertyIfDifferent<App::PropertyFloat>(joint, lJoint, "Distance2");
        copyPropertyIfDifferent<App::PropertyEnumeration>(joint, lJoint, "JointType");
        copyPropertyIfDifferent<App::PropertyPlacement>(joint, lJoint, "Offset1");
        copyPropertyIfDifferent<App::PropertyPlacement>(joint, lJoint, "Offset2");

        copyPropertyIfDifferent<App::PropertyBool>(joint, lJoint, "Detach1");
        copyPropertyIfDifferent<App::PropertyBool>(joint, lJoint, "Detach2");

        copyPropertyIfDifferent<App::PropertyFloat>(joint, lJoint, "AngleMax");
        copyPropertyIfDifferent<App::PropertyFloat>(joint, lJoint, "AngleMin");
        copyPropertyIfDifferent<App::PropertyFloat>(joint, lJoint, "LengthMax");
        copyPropertyIfDifferent<App::PropertyFloat>(joint, lJoint, "LengthMin");
        copyPropertyIfDifferent<App::PropertyBool>(joint, lJoint, "EnableAngleMax");
        copyPropertyIfDifferent<App::PropertyBool>(joint, lJoint, "EnableAngleMin");
        copyPropertyIfDifferent<App::PropertyBool>(joint, lJoint, "EnableLengthMax");
        copyPropertyIfDifferent<App::PropertyBool>(joint, lJoint, "EnableLengthMin");

        // The reference needs to be handled specifically
        handleJointReference(joint, lJoint, "Reference1");
        handleJointReference(joint, lJoint, "Reference2");
    }

    assemblyLinkJoints = getJoints();

    AssemblyObject::recomputeJointPlacements(assemblyLinkJoints);

    for (auto* joint : assemblyLinkJoints) {
        joint->purgeTouched();
    }
}


void AssemblyLink::handleJointReference(App::DocumentObject* joint,
                                        App::DocumentObject* lJoint,
                                        const char* refName)
{
    AssemblyObject* assembly = getLinkedAssembly();

    auto prop1 = dynamic_cast<App::PropertyXLinkSubHidden*>(joint->getPropertyByName(refName));
    auto prop2 = dynamic_cast<App::PropertyXLinkSubHidden*>(lJoint->getPropertyByName(refName));
    if (!prop1 || !prop2) {
        return;
    }

    App::DocumentObject* obj1 = nullptr;
    App::DocumentObject* obj2 = prop2->getValue();
    std::vector<std::string> subs1 = prop1->getSubValues();
    std::vector<std::string> subs2 = prop2->getSubValues();
    if (subs1.empty()) {
        return;
    }

    // Example :
    // Obj1 = docA-Asm1 Subs1 = ["part1.body.pad.face0", "part1.body.pad.vertex1"]
    // Obj1 = docA-Part Subs1 = ["Asm1.part1.body.pad.face0", "Asm1.part1.body.pad.vertex1"] // some
    // user may put the assembly inside a part... should become : Obj2 = docB-Asm2 Subs2 =
    // ["Asm1Link.part1.linkTobody.pad.face0", "Asm1Link.part1.linkTobody.pad.vertex1"] Obj2 =
    // docB-Part Sub2 = ["Asm2.Asm1Link.part1.linkTobody.pad.face0",
    // "Asm2.Asm1Link.part1.linkTobody.pad.vertex1"]

    std::string asmLink = getNameInDocument();
    for (auto& sub : subs1) {
        // First let's remove 'Asm1' name and everything before if any.
        sub = removeUpToName(sub, assembly->getNameInDocument());
        // Then we add the assembly link name.
        sub = asmLink + "." + sub;
        // Then the question is, is there more to prepend? Because the parent assembly may have some
        // parents So we check assemblyLink parents and prepend necessary parents.
        bool first = true;
        std::vector<App::DocumentObject*> inList = getInList();
        int limit = 0;
        while (!inList.empty() && limit < 20) {
            ++limit;
            bool found = false;
            for (auto* obj : inList) {
                if (obj->isDerivedFrom<App::Part>()) {
                    found = true;
                    if (first) {
                        first = false;
                    }
                    else {
                        std::string obj1Name = obj1->getNameInDocument();
                        sub = obj1Name + "." + sub;
                    }
                    obj1 = obj;
                    break;
                }
            }
            if (found) {
                inList = obj1->getInList();
            }
            else {
                inList = {};
            }
        }

        // Lastly we need to replace the object name by its link name.
        auto* obj = getObjFromRef(prop1);
        auto* link = objLinkMap[obj];
        if (!obj || !link) {
            return;
        }
        std::string objName = obj->getNameInDocument();
        std::string linkName = link->getNameInDocument();
        sub = replaceLastOccurrence(sub, objName, linkName);
    }
    // Now obj1 and the subs1 are what should be in obj2 and subs2 if the joint did not changed
    if (obj1 != obj2) {
        prop2->setValue(obj1);
    }
    bool changed = false;
    for (size_t i = 0; i < subs1.size(); ++i) {
        if (i >= subs2.size() || subs1[i] != subs2[i]) {
            changed = true;
            break;
        }
    }
    if (changed) {
        prop2->setSubValues(std::move(subs1));
    }
}

void AssemblyLink::ensureNoJointGroup()
{
    // Make sure there is no joint group
    JointGroup* jGroup = getJointGroup(this);
    if (jGroup) {
        // If there is a joint group, we delete it and its content.
        jGroup->removeObjectsFromDocument();
        getDocument()->removeObject(jGroup->getNameInDocument());
    }
}
JointGroup* AssemblyLink::ensureJointGroup()
{
    // Make sure there is a jointGroup
    JointGroup* jGroup = getJointGroup(this);
    if (!jGroup) {
        jGroup = new JointGroup();
        getDocument()->addObject(jGroup, tr("Joints").toStdString().c_str());

        // we want to add jgroup at the start, so we don't use
        // addObject(jGroup);
        std::vector<DocumentObject*> grp = Group.getValues();
        grp.insert(grp.begin(), jGroup);
        Group.setValues(grp);
    }
    return jGroup;
}

App::DocumentObject* AssemblyLink::getLinkedObject2(bool recursive) const
{
    auto* obj = LinkedObject.getValue();
    auto* assembly = dynamic_cast<AssemblyObject*>(obj);
    if (assembly) {
        return assembly;
    }
    else {
        auto* assemblyLink = dynamic_cast<AssemblyLink*>(obj);
        if (assemblyLink) {
            if (recursive) {
                return assemblyLink->getLinkedObject2(recursive);
            }
            else {
                return assemblyLink;
            }
        }
    }

    return nullptr;
}

AssemblyObject* AssemblyLink::getLinkedAssembly() const
{
    return dynamic_cast<AssemblyObject*>(getLinkedObject2());
}

AssemblyObject* AssemblyLink::getParentAssembly() const
{
    std::vector<App::DocumentObject*> inList = getInList();
    for (auto* obj : inList) {
        auto* assembly = dynamic_cast<AssemblyObject*>(obj);
        if (assembly) {
            return assembly;
        }
    }

    return nullptr;
}

bool AssemblyLink::isRigid()
{
    auto* prop = dynamic_cast<App::PropertyBool*>(getPropertyByName("Rigid"));
    if (!prop) {
        return true;
    }

    return prop->getValue();
}

std::vector<App::DocumentObject*> AssemblyLink::getJoints()
{
    JointGroup* jointGroup = getJointGroup(this);

    if (!jointGroup) {
        return {};
    }

    return jointGroup->getJoints();
}
