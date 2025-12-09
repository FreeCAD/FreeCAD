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

#include <cmath>
#include <vector>


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
    ADD_PROPERTY_TYPE(
        Rigid,
        (true),
        "General",
        (App::PropertyType)(App::Prop_None),
        "If the sub-assembly is set to Rigid, it will act "
        "as a rigid body. Else its joints will be taken into account."
    );

    ADD_PROPERTY_TYPE(
        LinkedObject,
        (nullptr),
        "General",
        (App::PropertyType)(App::Prop_None),
        "The linked assembly."
    );
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

        // A flexible sub-assembly cannot be grounded.
        // If a rigid sub-assembly has an object that is grounded, we also remove it.
        auto groundedJoints = getParentAssembly()->getGroundedJoints();
        for (auto* joint : groundedJoints) {
            auto* propObj = dynamic_cast<App::PropertyLink*>(
                joint->getPropertyByName("ObjectToGround")
            );
            if (!propObj) {
                continue;
            }
            auto* groundedObj = propObj->getValue();
            if (auto* linkElt = dynamic_cast<App::LinkElement*>(groundedObj)) {
                // hasObject does not handle link groups so we must handle it manually.
                groundedObj = linkElt->getLinkGroup();
            }

            if (Rigid.getValue() ? hasObject(groundedObj) : groundedObj == this) {
                getDocument()->removeObject(joint->getNameInDocument());
            }
        }

        if (Rigid.getValue()) {
            // movePlc needs to be computed before updateContents.
            App::DocumentObject* firstLink = nullptr;
            for (auto* obj : Group.getValues()) {
                if (obj && (obj->isDerivedFrom<App::Link>() || obj->isDerivedFrom<AssemblyLink>())) {
                    firstLink = obj;
                    break;
                }
            }

            if (firstLink) {
                App::DocumentObject* sourceObj = nullptr;
                if (auto* link = dynamic_cast<App::Link*>(firstLink)) {
                    sourceObj = link->getLinkedObject(false);  // Get non-recursive linked object
                }
                else if (auto* asmLink = dynamic_cast<AssemblyLink*>(firstLink)) {
                    sourceObj = asmLink->getLinkedAssembly();
                }

                if (sourceObj) {
                    auto* propSource = dynamic_cast<App::PropertyPlacement*>(
                        sourceObj->getPropertyByName("Placement")
                    );
                    auto* propLink = dynamic_cast<App::PropertyPlacement*>(
                        firstLink->getPropertyByName("Placement")
                    );

                    if (propSource && propLink) {
                        movePlc = propLink->getValue() * propSource->getValue().inverse();
                    }
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

                    if (obj->isLinkGroup()) {
                        auto* srcLink = static_cast<App::Link*>(obj);
                        const std::vector<App::DocumentObject*> srcElements
                            = srcLink->ElementList.getValues();

                        for (auto elt : srcElements) {
                            if (!elt) {
                                continue;
                            }

                            auto* prop = dynamic_cast<App::PropertyPlacement*>(
                                elt->getPropertyByName("Placement")
                            );
                            if (prop) {
                                prop->setValue(plc * prop->getValue());
                            }
                        }
                    }
                    else {
                        auto* prop = dynamic_cast<App::PropertyPlacement*>(
                            obj->getPropertyByName("Placement")
                        );
                        if (prop) {
                            prop->setValue(plc * prop->getValue());
                        }
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

    // Filter out child objects from Part-workbench features to get only top-level components.
    // An object is considered a child if it's referenced by another object's 'Base', 'Tool',
    // or 'Shapes' property within the same group.
    std::set<App::DocumentObject*> children;
    for (auto* obj : assemblyGroup) {
        if (auto* partFeat = dynamic_cast<PartApp::Feature*>(obj)) {
            if (auto* prop = dynamic_cast<App::PropertyLink*>(partFeat->getPropertyByName("Base"))) {
                if (prop->getValue()) {
                    children.insert(prop->getValue());
                }
            }
            if (auto* prop = dynamic_cast<App::PropertyLink*>(partFeat->getPropertyByName("Tool"))) {
                if (prop->getValue()) {
                    children.insert(prop->getValue());
                }
            }
            if (auto* prop
                = dynamic_cast<App::PropertyLinkList*>(partFeat->getPropertyByName("Shapes"))) {
                for (auto* shapeObj : prop->getValues()) {
                    children.insert(shapeObj);
                }
            }
        }
    }

    std::vector<App::DocumentObject*> topLevelComponents;
    std::copy_if(
        assemblyGroup.begin(),
        assemblyGroup.end(),
        std::back_inserter(topLevelComponents),
        [&children](App::DocumentObject* obj) { return children.find(obj) == children.end(); }
    );

    // We check if a component needs to be added to the AssemblyLink
    for (auto* obj : topLevelComponents) {
        if (!obj->isDerivedFrom<App::Part>() && !obj->isDerivedFrom<PartApp::Feature>()
            && !obj->isDerivedFrom<App::Link>()) {
            continue;
        }

        // Note, the user can have nested sub-assemblies.
        // In which case we need to add an AssemblyLink and not a Link.
        App::DocumentObject* link = nullptr;
        bool found = false;
        std::set<App::Link*> linkGroupsAdded;

        for (auto* obj2 : assemblyLinkGroup) {
            App::DocumentObject* linkedObj;

            auto* subAsmLink = freecad_cast<AssemblyLink*>(obj2);
            auto* link2 = dynamic_cast<App::Link*>(obj2);

            if (subAsmLink) {
                linkedObj = subAsmLink->getLinkedObject2(false);  // not recursive
            }
            else if (link2) {
                if (obj->isLinkGroup() && link2->isLinkGroup()) {
                    auto* srcLink = static_cast<App::Link*>(obj);
                    if ((srcLink->getTrueLinkedObject(false) == link2->getTrueLinkedObject(false))
                        && link2->ElementCount.getValue() == srcLink->ElementCount.getValue()
                        && linkGroupsAdded.find(srcLink) == linkGroupsAdded.end()) {
                        found = true;
                        link = obj2;
                        // In case where there are more than 2 link groups with the
                        // same number of elements.
                        linkGroupsAdded.insert(srcLink);

                        const std::vector<App::DocumentObject*> srcElements
                            = srcLink->ElementList.getValues();
                        const std::vector<App::DocumentObject*> newElements
                            = link2->ElementList.getValues();
                        for (int i = 0; i < srcElements.size(); ++i) {
                            objLinkMap[srcElements[i]] = newElements[i];
                        }
                        break;
                    }
                }
                else if (obj->isLinkGroup() && !link2->isLinkGroup()) {
                    continue;  // make sure we migrate sub assemblies that had link to linkgroups
                }
                linkedObj = link2->getLinkedObject(false);  // not recursive
            }
            else {
                // We consider only Links and AssemblyLinks
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

                App::DocumentObject* newObj
                    = doc->addObject("Assembly::AssemblyLink", obj->getNameInDocument());
                auto* subAsmLink = static_cast<AssemblyLink*>(newObj);
                subAsmLink->LinkedObject.setValue(obj);
                subAsmLink->Rigid.setValue(asmLink->Rigid.getValue());
                subAsmLink->Label.setValue(obj->Label.getValue());
                addObject(subAsmLink);
                link = subAsmLink;
            }
            else if (obj->isDerivedFrom<App::Link>() && obj->isLinkGroup()) {
                auto* srcLink = static_cast<App::Link*>(obj);

                auto* newLink = static_cast<App::Link*>(
                    doc->addObject("App::Link", obj->getNameInDocument())
                );
                newLink->LinkedObject.setValue(srcLink->getTrueLinkedObject(false));

                newLink->Label.setValue(obj->Label.getValue());
                addObject(newLink);

                newLink->ElementCount.setValue(srcLink->ElementCount.getValue());
                const std::vector<App::DocumentObject*> srcElements = srcLink->ElementList.getValues();
                const std::vector<App::DocumentObject*> newElements = newLink->ElementList.getValues();
                for (int i = 0; i < srcElements.size(); ++i) {
                    auto* newObj = newElements[i];
                    auto* srcObj = srcElements[i];
                    if (newObj && srcObj) {
                        syncPlacements(srcObj, newObj);
                    }
                    objLinkMap[srcObj] = newObj;
                }

                link = newLink;
            }
            else {
                App::DocumentObject* newObj = doc->addObject("App::Link", obj->getNameInDocument());
                auto* newLink = static_cast<App::Link*>(newObj);
                newLink->LinkedObject.setValue(obj);
                newLink->Label.setValue(obj->Label.getValue());
                addObject(newLink);
                link = newLink;
            }
        }

        objLinkMap[obj] = link;
    }

    // If the assemblyLink is rigid, then we keep all placements synchronized.
    if (isRigid()) {
        for (const auto& [sourceObj, linkObj] : objLinkMap) {
            syncPlacements(sourceObj, linkObj);
        }
    }

    // We check if a component needs to be removed from the AssemblyLink
    // NOTE: this is not being executed when a src link is deleted, because the link
    // is then in error, and so AssemblyLink::execute() does not get called.
    std::set<App::DocumentObject*> validLinks;
    for (const auto& pair : objLinkMap) {
        validLinks.insert(pair.second);
    }
    for (auto* obj : assemblyLinkGroup) {
        // We don't need to update assemblyLinkGroup after the addition since we're not removing
        // something we just added.
        if (!obj->isDerivedFrom<App::Part>() && !obj->isDerivedFrom<PartApp::Feature>()
            && !obj->isDerivedFrom<App::Link>()) {
            continue;
        }
        if (validLinks.find(obj) == validLinks.end()) {
            doc->removeObject(obj->getNameInDocument());
        }
    }
}

namespace
{
template<typename T>
void copyPropertyIfDifferent(
    App::DocumentObject* source,
    App::DocumentObject* target,
    const char* propertyName
)
{
    auto sourceProp = freecad_cast<T*>(source->getPropertyByName(propertyName));
    auto targetProp = freecad_cast<T*>(target->getPropertyByName(propertyName));
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

std::string replaceLastOccurrence(
    const std::string& str,
    const std::string& oldStr,
    const std::string& newStr
)
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

    std::vector<App::DocumentObject*> assemblyJoints
        = assembly->getJoints(assembly->isTouched(), false, false);
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
        copyPropertyIfDifferent<App::PropertyBool>(joint, lJoint, "Suppressed");
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


void AssemblyLink::handleJointReference(
    App::DocumentObject* joint,
    App::DocumentObject* lJoint,
    const char* refName
)
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

        std::vector<DocumentObject*> grp = Group.getValues();
        grp.insert(grp.begin(), jGroup);
        Group.setValues(grp);
    }
    return jGroup;
}

App::DocumentObject* AssemblyLink::getLinkedObject2(bool recursive) const
{
    auto* obj = LinkedObject.getValue();
    auto* assembly = freecad_cast<AssemblyObject*>(obj);
    if (assembly) {
        return assembly;
    }
    else {
        auto* assemblyLink = freecad_cast<AssemblyLink*>(obj);
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
    return freecad_cast<AssemblyObject*>(getLinkedObject2());
}

AssemblyObject* AssemblyLink::getParentAssembly() const
{
    std::vector<App::DocumentObject*> inList = getInList();
    for (auto* obj : inList) {
        auto* assembly = freecad_cast<AssemblyObject*>(obj);
        if (assembly) {
            return assembly;
        }
    }

    return nullptr;
}

bool AssemblyLink::isRigid() const
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

bool AssemblyLink::allowDuplicateLabel() const
{
    return true;
}

int AssemblyLink::numberOfComponents() const
{
    return isRigid() ? 1 : getLinkedAssembly()->numberOfComponents();
}

bool AssemblyLink::isEmpty() const
{
    return numberOfComponents() == 0;
}
