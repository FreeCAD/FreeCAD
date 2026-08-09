// SPDX-License-Identifier: LGPL-2.1-or-later

#include "MbDAssembly.h"

#include <algorithm>
#include <array>
#include <cstring>

#include <App/Document.h>
#include <App/GroupExtension.h>
#include <App/Property.h>
#include <Base/Tools.h>

#include "MbDAction.h"
#include "MbDAssemblyPy.h"
#include "MbDFolders.h"
#include "MbDGroupUtils.h"
#include "MbDJoint.h"
#include "MbDMotion.h"
#include "MbDParameters.h"
#include "MbDPart.h"

PROPERTY_SOURCE(MbDFEM::MbDAssembly, App::Part)

namespace
{

struct AssemblyCategory
{
    App::PropertyLinkList MbDFEM::MbDAssembly::*children;
    App::DocumentObjectGroup* (MbDFEM::MbDAssembly::*getFolder)() const;
    App::DocumentObjectGroup* (MbDFEM::MbDAssembly::*ensureFolder)();
    const char* allowedType;
};

constexpr std::array<AssemblyCategory, 6> assemblyCategories = {{
    {&MbDFEM::MbDAssembly::assemblies,
     &MbDFEM::MbDAssembly::getAssembliesFolder,
     &MbDFEM::MbDAssembly::ensureAssembliesFolder,
     "MbDFEM::MbDAssembly"},
    {&MbDFEM::MbDAssembly::fixedparts,
     &MbDFEM::MbDAssembly::getFixedPartsFolder,
     &MbDFEM::MbDAssembly::ensureFixedPartsFolder,
     "MbDFEM::MbDPart"},
    {&MbDFEM::MbDAssembly::parts,
     &MbDFEM::MbDAssembly::getPartsFolder,
     &MbDFEM::MbDAssembly::ensurePartsFolder,
     "MbDFEM::MbDPart"},
    {&MbDFEM::MbDAssembly::joints,
     &MbDFEM::MbDAssembly::getJointsFolder,
     &MbDFEM::MbDAssembly::ensureJointsFolder,
     "MbDFEM::MbDJoint"},
    {&MbDFEM::MbDAssembly::motions,
     &MbDFEM::MbDAssembly::getMotionsFolder,
     &MbDFEM::MbDAssembly::ensureMotionsFolder,
     "MbDFEM::MbDMotion"},
    {&MbDFEM::MbDAssembly::actions,
     &MbDFEM::MbDAssembly::getActionsFolder,
     &MbDFEM::MbDAssembly::ensureActionsFolder,
     "MbDFEM::MbDAction"},
}};

App::DocumentObjectGroup* getFolder(const MbDFEM::MbDAssembly* assembly,
                                    const AssemblyCategory& category)
{
    return assembly ? (assembly->*(category.getFolder))() : nullptr;
}

App::DocumentObjectGroup* ensureFolder(MbDFEM::MbDAssembly* assembly,
                                       const AssemblyCategory& category)
{
    return assembly ? (assembly->*(category.ensureFolder))() : nullptr;
}

App::DocumentObjectGroup* matchingFolder(const char* subname,
                                         const char*& rest,
                                         const MbDFEM::MbDAssembly* assembly)
{
    rest = nullptr;
    const char* dot = subname ? std::strchr(subname, '.') : nullptr;
    if (!dot) {
        return nullptr;
    }

    const std::string segment(subname, dot);
    for (const auto& category : assemblyCategories) {
        auto* folder = getFolder(assembly, category);
        if (folder && segment == folder->getNameInDocument()) {
            rest = dot + 1;
            return folder;
        }
    }
    return nullptr;
}

App::DocumentObject* matchingParameter(const char* subname,
                                       const char*& rest,
                                       const MbDFEM::MbDAssembly* assembly)
{
    rest = nullptr;
    const char* dot = subname ? std::strchr(subname, '.') : nullptr;
    if (!dot) {
        return nullptr;
    }

    const std::string segment(subname, dot);
    for (auto* parameter : assembly->getParameterObjects()) {
        if (parameter && segment == parameter->getNameInDocument()) {
            rest = dot + 1;
            return parameter;
        }
    }
    return nullptr;
}

App::DocumentObject* findDirectChildByInternalName(const char* element,
                                                   const MbDFEM::MbDAssembly* assembly)
{
    if (!element || !*element) {
        return nullptr;
    }

    std::string name(element);
    if (!name.empty() && name.back() == '.') {
        name.pop_back();
    }

    for (const auto& category : assemblyCategories) {
        const auto childList = (assembly->*(category.children)).getValues();
        for (auto* child : childList) {
            if (!child) {
                continue;
            }
            if (name == child->getNameInDocument()) {
                return child;
            }
        }
    }
    for (auto* child : assembly->getParameterObjects()) {
        if (child && name == child->getNameInDocument()) {
            return child;
        }
    }
    return nullptr;
}

App::DocumentObject* matchingDirectChild(const char* subname,
                                         const char*& rest,
                                         const MbDFEM::MbDAssembly* assembly)
{
    rest = nullptr;
    const char* dot = subname ? std::strchr(subname, '.') : nullptr;
    if (!dot) {
        return findDirectChildByInternalName(subname, assembly);
    }

    const std::string segment(subname, dot);
    auto* child = findDirectChildByInternalName(segment.c_str(), assembly);
    if (child) {
        rest = dot + 1;
    }
    return child;
}

MbDFEM::MbDGravity* findExistingGravity(MbDFEM::MbDAssembly* assembly)
{
    auto* document = assembly ? assembly->getDocument() : nullptr;
    if (!document) {
        return nullptr;
    }

    const std::string canonicalName = std::string(assembly->getNameInDocument()) + "_Gravity";
    if (auto* canonical = freecad_cast<MbDFEM::MbDGravity*>(
            document->getObject(canonicalName.c_str()))) {
        return canonical;
    }

    for (auto* object : document->getObjectsOfType<MbDFEM::MbDGravity>()) {
        if (!object) {
            continue;
        }
        const std::string objectName = object->getNameInDocument();
        if (objectName.rfind(canonicalName, 0) == 0
            && std::strcmp(object->Label.getValue(), "Gravity") == 0) {
            return object;
        }
    }

    return nullptr;
}

std::vector<App::DocumentObject*> partObjectsFromFolder(App::DocumentObjectGroup* folder)
{
    if (!folder) {
        return {};
    }

    std::vector<App::DocumentObject*> parts;
    for (auto* object : folder->Group.getValues()) {
        if (object && object->isDerivedFrom<MbDFEM::MbDPart>()) {
            parts.push_back(object);
        }
    }
    return parts;
}

std::vector<App::DocumentObject*> partObjectsFromProperty(const App::PropertyLinkList& property)
{
    std::vector<App::DocumentObject*> parts;
    for (auto* object : property.getValues()) {
        if (object && object->isDerivedFrom<MbDFEM::MbDPart>()) {
            parts.push_back(object);
        }
    }
    return parts;
}

void appendUniqueObject(std::vector<App::DocumentObject*>& objects, App::DocumentObject* object)
{
    if (object && std::find(objects.begin(), objects.end(), object) == objects.end()) {
        objects.push_back(object);
    }
}

void setFolderObjects(App::DocumentObjectGroup* folder,
                      const std::vector<App::DocumentObject*>& objects)
{
    if (!folder) {
        return;
    }

    Base::ObjectStatusLocker<App::Property::Status, App::Property> guard(App::Property::User3,
                                                                         &folder->Group);
    folder->Group.setValues(objects);
}

void addToAssemblyGeoGroup(App::DocumentObject* owner, App::DocumentObject* child)
{
    if (!owner || !child) {
        return;
    }

    auto* group = owner->getExtensionByType<App::GroupExtension>();
    if (group) {
        MbDFEM::appendUnique(group->Group, child);
    }
}

}  // namespace

MbDFEM::MbDAssembly::MbDAssembly()
{
    ADD_PROPERTY_TYPE(assemblies,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_None,
                      "Subassemblies belonging to this assembly");
    assemblies.setScope(App::LinkScope::Child);
    ADD_PROPERTY_TYPE(fixedparts,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_None,
                      "Parts fixed to ground in this assembly");
    fixedparts.setScope(App::LinkScope::Child);
    ADD_PROPERTY_TYPE(parts,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_None,
                      "Parts belonging to this assembly");
    parts.setScope(App::LinkScope::Child);
    ADD_PROPERTY_TYPE(joints,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_None,
                      "Joints belonging to this assembly");
    joints.setScope(App::LinkScope::Child);
    ADD_PROPERTY_TYPE(motions,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_None,
                      "Motions belonging to this assembly");
    motions.setScope(App::LinkScope::Child);
    ADD_PROPERTY_TYPE(actions,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_None,
                      "Actions belonging to this assembly");
    actions.setScope(App::LinkScope::Child);
    ADD_PROPERTY_TYPE(_assembliesFolder,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_Hidden,
                      "Tree folder containing this assembly's subassemblies");
    _assembliesFolder.setScope(App::LinkScope::Hidden);
    ADD_PROPERTY_TYPE(_fixedPartsFolder,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_Hidden,
                      "Tree folder containing this assembly's fixed parts");
    _fixedPartsFolder.setScope(App::LinkScope::Hidden);
    ADD_PROPERTY_TYPE(_partsFolder,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_Hidden,
                      "Tree folder containing this assembly's parts");
    _partsFolder.setScope(App::LinkScope::Hidden);
    ADD_PROPERTY_TYPE(_jointsFolder,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_Hidden,
                      "Tree folder containing this assembly's joints");
    _jointsFolder.setScope(App::LinkScope::Hidden);
    ADD_PROPERTY_TYPE(_motionsFolder,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_Hidden,
                      "Tree folder containing this assembly's motions");
    _motionsFolder.setScope(App::LinkScope::Hidden);
    ADD_PROPERTY_TYPE(_actionsFolder,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_Hidden,
                      "Tree folder containing this assembly's actions");
    _actionsFolder.setScope(App::LinkScope::Hidden);
    ADD_PROPERTY_TYPE(_gravity,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_Hidden,
                      "Gravity object owned by this assembly");
    _gravity.setScope(App::LinkScope::Hidden);
    ADD_PROPERTY_TYPE(_simulationParameters,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_Hidden,
                      "Simulation parameters owned by this assembly");
    _simulationParameters.setScope(App::LinkScope::Hidden);
    ADD_PROPERTY_TYPE(_animationParameters,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_Hidden,
                      "Animation parameters owned by this assembly");
    _animationParameters.setScope(App::LinkScope::Hidden);
}

void MbDFEM::MbDAssembly::addAssembly(MbDAssembly* assembly)
{
    if (!assembly || assembly == this) {
        return;
    }

    addChildToListFolderAndGeoGroup(this, assemblies, ensureFolder(this, assemblyCategories[0]), assembly);
    ensureFixedPartsFolder();
    ensurePartsFolder();
}

void MbDFEM::MbDAssembly::addPart(MbDPart* part)
{
    if (!part) {
        return;
    }

    removeFixedPart(part);
    addChildToListFolderAndGeoGroup(this, parts, ensureFolder(this, assemblyCategories[2]), part);
    ensureFixedPartsFolder();
    synchronizePartCategories();
}

void MbDFEM::MbDAssembly::removePart(MbDPart* part)
{
    removeChildFromListFolderAndGeoGroup(this, parts, getPartsFolder(), part);
}

void MbDFEM::MbDAssembly::addFixedPart(MbDPart* part)
{
    if (!part) {
        return;
    }

    removePart(part);
    addChildToListFolderAndGeoGroup(this, fixedparts, ensureFolder(this, assemblyCategories[1]), part);
    ensurePartsFolder();
    synchronizePartCategories();
}

void MbDFEM::MbDAssembly::removeFixedPart(MbDPart* part)
{
    removeChildFromListFolderAndGeoGroup(this, fixedparts, getFixedPartsFolder(), part);
}

void MbDFEM::MbDAssembly::groundPart(MbDPart* part)
{
    if (!part) {
        return;
    }

    removePart(part);
    addFixedPart(part);
}

void MbDFEM::MbDAssembly::addJoint(MbDJoint* joint)
{
    if (!joint) {
        return;
    }

    addChildToListFolderAndGeoGroup(this, joints, ensureFolder(this, assemblyCategories[3]), joint);
    ensureMotionsFolder();
    ensureActionsFolder();
}

void MbDFEM::MbDAssembly::addMotion(MbDMotion* motion)
{
    if (!motion) {
        return;
    }

    addChildToListFolderAndGeoGroup(this, motions, ensureFolder(this, assemblyCategories[4]), motion);
    ensureJointsFolder();
    ensureActionsFolder();
}

void MbDFEM::MbDAssembly::addAction(MbDAction* action)
{
    if (!action) {
        return;
    }

    addChildToListFolderAndGeoGroup(this, actions, ensureFolder(this, assemblyCategories[5]), action);
    ensureJointsFolder();
    ensureMotionsFolder();
}

int MbDFEM::MbDAssembly::setElementVisible(const char* element, bool visible)
{
    auto* child = findDirectChildByInternalName(element, this);
    if (!child) {
        return App::Part::setElementVisible(element, visible);
    }

    child->Visibility.setValue(visible);
    return visible ? 1 : 0;
}

int MbDFEM::MbDAssembly::isElementVisible(const char* element) const
{
    if (!Visibility.getValue()) {
        return 0;
    }

    auto* child = findDirectChildByInternalName(element, this);
    if (!child) {
        return App::Part::isElementVisible(element);
    }

    return child->Visibility.getValue() ? 1 : 0;
}

App::DocumentObject* MbDFEM::MbDAssembly::getSubObject(const char* subname,
                                                       PyObject** pyObj,
                                                       Base::Matrix4D* mat,
                                                       bool transform,
                                                       int depth) const
{
    const char* rest = nullptr;
    auto* folder = matchingFolder(subname, rest, this);
    if (folder) {
        if (!rest || *rest == '\0') {
            return folder;
        }
        return App::Part::getSubObject(rest, pyObj, mat, transform, depth);
    }

    auto* parameter = matchingParameter(subname, rest, this);
    if (parameter) {
        return parameter;
    }

    auto* child = matchingDirectChild(subname, rest, this);
    if (child) {
        if (!rest || *rest == '\0') {
            return child;
        }
        return child->getSubObject(rest, pyObj, mat, transform, depth + 1);
    }

    return App::Part::getSubObject(subname, pyObj, mat, transform, depth);
}

App::DocumentObjectGroup* MbDFEM::MbDAssembly::getPartsFolder() const
{
    return dynamic_cast<App::DocumentObjectGroup*>(_partsFolder.getValue());
}

App::DocumentObjectGroup* MbDFEM::MbDAssembly::getFixedPartsFolder() const
{
    return dynamic_cast<App::DocumentObjectGroup*>(_fixedPartsFolder.getValue());
}

App::DocumentObjectGroup* MbDFEM::MbDAssembly::getAssembliesFolder() const
{
    return dynamic_cast<App::DocumentObjectGroup*>(_assembliesFolder.getValue());
}

App::DocumentObjectGroup* MbDFEM::MbDAssembly::getJointsFolder() const
{
    return dynamic_cast<App::DocumentObjectGroup*>(_jointsFolder.getValue());
}

App::DocumentObjectGroup* MbDFEM::MbDAssembly::getMotionsFolder() const
{
    return dynamic_cast<App::DocumentObjectGroup*>(_motionsFolder.getValue());
}

App::DocumentObjectGroup* MbDFEM::MbDAssembly::getActionsFolder() const
{
    return dynamic_cast<App::DocumentObjectGroup*>(_actionsFolder.getValue());
}

MbDFEM::MbDSimulationParameters* MbDFEM::MbDAssembly::getSimulationParameters() const
{
    return dynamic_cast<MbDFEM::MbDSimulationParameters*>(_simulationParameters.getValue());
}

MbDFEM::MbDAnimationParameters* MbDFEM::MbDAssembly::getAnimationParameters() const
{
    return dynamic_cast<MbDFEM::MbDAnimationParameters*>(_animationParameters.getValue());
}

MbDFEM::MbDGravity* MbDFEM::MbDAssembly::getGravity() const
{
    return dynamic_cast<MbDFEM::MbDGravity*>(_gravity.getValue());
}

std::vector<App::DocumentObjectGroup*> MbDFEM::MbDAssembly::getCategoryFolders() const
{
    std::vector<App::DocumentObjectGroup*> folders;
    folders.reserve(assemblyCategories.size());
    for (const auto& category : assemblyCategories) {
        if (auto* folder = getFolder(this, category)) {
            folders.push_back(folder);
        }
    }
    return folders;
}

std::vector<App::DocumentObject*> MbDFEM::MbDAssembly::getCategoryChildren() const
{
    std::vector<App::DocumentObject*> children;
    for (const auto& category : assemblyCategories) {
        const auto values = (this->*(category.children)).getValues();
        children.insert(children.end(), values.begin(), values.end());
    }
    return children;
}

std::vector<App::DocumentObject*> MbDFEM::MbDAssembly::getParameterObjects() const
{
    std::vector<App::DocumentObject*> parameters;
    parameters.reserve(3);
    if (auto* gravity = getGravity()) {
        parameters.push_back(gravity);
    }
    if (auto* simulation = getSimulationParameters()) {
        parameters.push_back(simulation);
    }
    if (auto* animation = getAnimationParameters()) {
        parameters.push_back(animation);
    }
    return parameters;
}

void MbDFEM::MbDAssembly::removeFromCategories(App::DocumentObject* child)
{
    if (!child) {
        return;
    }

    for (const auto& category : assemblyCategories) {
        removeAll(this->*(category.children), child);
        if (auto* folder = getFolder(this, category)) {
            if (folder->hasObject(child)) {
                folder->removeObject(child);
            }
        }
    }
}

void MbDFEM::MbDAssembly::synchronizePartCategories()
{
    auto* partsFolder = getPartsFolder();
    auto* fixedPartsFolder = getFixedPartsFolder();

    auto fixedValues = partObjectsFromFolder(fixedPartsFolder);
    if (fixedValues.empty() && !fixedparts.getValues().empty()) {
        fixedValues = partObjectsFromProperty(fixedparts);
    }

    auto partValues = partObjectsFromFolder(partsFolder);
    if (partValues.empty() && !parts.getValues().empty()) {
        partValues = partObjectsFromProperty(parts);
    }

    for (auto* fixedPart : fixedValues) {
        partValues.erase(std::remove(partValues.begin(), partValues.end(), fixedPart),
                         partValues.end());
    }

    std::vector<App::DocumentObject*> uniqueFixedValues;
    for (auto* fixedPart : fixedValues) {
        appendUniqueObject(uniqueFixedValues, fixedPart);
        addToAssemblyGeoGroup(this, fixedPart);
    }

    std::vector<App::DocumentObject*> uniquePartValues;
    for (auto* part : partValues) {
        appendUniqueObject(uniquePartValues, part);
        addToAssemblyGeoGroup(this, part);
    }

    Base::ObjectStatusLocker<App::Property::Status, App::Property> fixedGuard(
        App::Property::User3,
        &fixedparts);
    Base::ObjectStatusLocker<App::Property::Status, App::Property> partsGuard(
        App::Property::User3,
        &parts);
    fixedparts.setValues(uniqueFixedValues);
    parts.setValues(uniquePartValues);
    setFolderObjects(fixedPartsFolder, uniqueFixedValues);
    setFolderObjects(partsFolder, uniquePartValues);
}

void MbDFEM::MbDAssembly::onChanged(const App::Property* prop)
{
    App::Part::onChanged(prop);

    if ((prop == &parts && !parts.testStatus(App::Property::User3))
        || (prop == &fixedparts && !fixedparts.testStatus(App::Property::User3))) {
        synchronizePartCategories();
    }
}

void MbDFEM::MbDAssembly::onDocumentRestored()
{
    App::Part::onDocumentRestored();
    synchronizePartCategories();
}

App::DocumentObjectGroup* MbDFEM::MbDAssembly::ensurePartsFolder()
{
    if (auto* folder = getPartsFolder()) {
        return folder;
    }
    if (!getDocument()) {
        return nullptr;
    }

    const std::string name = std::string(getNameInDocument()) + "_Parts";
    auto* folder = static_cast<App::DocumentObjectGroup*>(
        getDocument()->addObject("MbDFEM::MbDPartsFolder", name.c_str()));
    folder->Label.setValue("Parts");
    _partsFolder.setValue(folder);
    return folder;
}

App::DocumentObjectGroup* MbDFEM::MbDAssembly::ensureFixedPartsFolder()
{
    if (auto* folder = getFixedPartsFolder()) {
        return folder;
    }
    if (!getDocument()) {
        return nullptr;
    }

    const std::string name = std::string(getNameInDocument()) + "_FixedParts";
    auto* folder = static_cast<App::DocumentObjectGroup*>(
        getDocument()->addObject("MbDFEM::MbDFixedPartsFolder", name.c_str()));
    folder->Label.setValue("FixedParts");
    _fixedPartsFolder.setValue(folder);
    return folder;
}

App::DocumentObjectGroup* MbDFEM::MbDAssembly::ensureAssembliesFolder()
{
    if (auto* folder = getAssembliesFolder()) {
        return folder;
    }
    if (!getDocument()) {
        return nullptr;
    }

    const std::string name = std::string(getNameInDocument()) + "_Assemblies";
    auto* folder = static_cast<App::DocumentObjectGroup*>(
        getDocument()->addObject("MbDFEM::MbDAssembliesFolder", name.c_str()));
    folder->Label.setValue("Assemblies");
    _assembliesFolder.setValue(folder);
    return folder;
}

App::DocumentObjectGroup* MbDFEM::MbDAssembly::ensureJointsFolder()
{
    if (auto* folder = getJointsFolder()) {
        return folder;
    }
    if (!getDocument()) {
        return nullptr;
    }

    const std::string name = std::string(getNameInDocument()) + "_Joints";
    auto* folder = static_cast<App::DocumentObjectGroup*>(
        getDocument()->addObject("MbDFEM::MbDJointsFolder", name.c_str()));
    folder->Label.setValue("Joints");
    _jointsFolder.setValue(folder);
    return folder;
}

App::DocumentObjectGroup* MbDFEM::MbDAssembly::ensureMotionsFolder()
{
    if (auto* folder = getMotionsFolder()) {
        return folder;
    }
    if (!getDocument()) {
        return nullptr;
    }

    const std::string name = std::string(getNameInDocument()) + "_Motions";
    auto* folder = static_cast<App::DocumentObjectGroup*>(
        getDocument()->addObject("MbDFEM::MbDMotionsFolder", name.c_str()));
    folder->Label.setValue("Motions");
    _motionsFolder.setValue(folder);
    return folder;
}

App::DocumentObjectGroup* MbDFEM::MbDAssembly::ensureActionsFolder()
{
    if (auto* folder = getActionsFolder()) {
        return folder;
    }
    if (!getDocument()) {
        return nullptr;
    }

    const std::string name = std::string(getNameInDocument()) + "_Actions";
    auto* folder = static_cast<App::DocumentObjectGroup*>(
        getDocument()->addObject("MbDFEM::MbDActionsFolder", name.c_str()));
    folder->Label.setValue("Actions");
    _actionsFolder.setValue(folder);
    return folder;
}

MbDFEM::MbDGravity* MbDFEM::MbDAssembly::ensureGravity()
{
    if (auto* gravityObject = getGravity()) {
        return gravityObject;
    }

    if (auto* existingGravity = findExistingGravity(this)) {
        _gravity.setValue(existingGravity);
        return existingGravity;
    }

    if (!getDocument()) {
        return nullptr;
    }

    const std::string name = std::string(getNameInDocument()) + "_Gravity";
    auto* gravityObject = static_cast<MbDFEM::MbDGravity*>(
        getDocument()->addObject("MbDFEM::MbDGravity", name.c_str()));
    gravityObject->Label.setValue("Gravity");
    _gravity.setValue(gravityObject);
    return gravityObject;
}

MbDFEM::MbDSimulationParameters* MbDFEM::MbDAssembly::ensureSimulationParameters()
{
    if (auto* parameters = getSimulationParameters()) {
        return parameters;
    }
    if (!getDocument()) {
        return nullptr;
    }

    const std::string name = std::string(getNameInDocument()) + "_SimulationParameters";
    auto* parameters = static_cast<MbDFEM::MbDSimulationParameters*>(
        getDocument()->addObject("MbDFEM::MbDSimulationParameters", name.c_str()));
    parameters->Label.setValue("SimulationParameters");
    _simulationParameters.setValue(parameters);
    return parameters;
}

MbDFEM::MbDAnimationParameters* MbDFEM::MbDAssembly::ensureAnimationParameters()
{
    if (auto* parameters = getAnimationParameters()) {
        return parameters;
    }
    if (!getDocument()) {
        return nullptr;
    }

    const std::string name = std::string(getNameInDocument()) + "_AnimationParameters";
    auto* parameters = static_cast<MbDFEM::MbDAnimationParameters*>(
        getDocument()->addObject("MbDFEM::MbDAnimationParameters", name.c_str()));
    parameters->Label.setValue("AnimationParameters");
    _animationParameters.setValue(parameters);
    return parameters;
}

PyObject* MbDFEM::MbDAssembly::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        PythonObject = Py::Object(new MbDAssemblyPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}
