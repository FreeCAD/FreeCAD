// SPDX-License-Identifier: LGPL-2.1-or-later

#include "MbDAssembly.h"

#include <algorithm>

#include <App/Document.h>

#include "MbDAction.h"
#include "MbDAssemblyPy.h"
#include "MbDFolders.h"
#include "MbDJoint.h"
#include "MbDMarker.h"
#include "MbDMotion.h"
#include "MbDPart.h"

PROPERTY_SOURCE(MbDFEM::MbDAssembly, App::DocumentObject)

MbDFEM::MbDAssembly::MbDAssembly()
{
    ADD_PROPERTY_TYPE(Placement,
                      (Base::Placement()),
                      "MbDFEM",
                      App::Prop_None,
                      "Placement of this assembly");
    ADD_PROPERTY_TYPE(assemblies,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_None,
                      "Subassemblies belonging to this assembly");
    ADD_PROPERTY_TYPE(parts,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_None,
                      "Parts belonging to this assembly");
    ADD_PROPERTY_TYPE(markers,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_None,
                      "Markers belonging to this assembly");
    ADD_PROPERTY_TYPE(joints,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_None,
                      "Joints belonging to this assembly");
    ADD_PROPERTY_TYPE(motions,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_None,
                      "Motions belonging to this assembly");
    ADD_PROPERTY_TYPE(actions,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_None,
                      "Actions belonging to this assembly");
    ADD_PROPERTY_TYPE(_partsFolder,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_Hidden,
                      "Tree folder containing this assembly's parts");
    ADD_PROPERTY_TYPE(_assembliesFolder,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_Hidden,
                      "Tree folder containing this assembly's subassemblies");
    ADD_PROPERTY_TYPE(_markersFolder,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_Hidden,
                      "Tree folder containing this assembly's markers");
    ADD_PROPERTY_TYPE(_jointsFolder,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_Hidden,
                      "Tree folder containing this assembly's joints");
    ADD_PROPERTY_TYPE(_motionsFolder,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_Hidden,
                      "Tree folder containing this assembly's motions");
    ADD_PROPERTY_TYPE(_actionsFolder,
                      (nullptr),
                      "MbDFEM",
                      App::Prop_Hidden,
                      "Tree folder containing this assembly's actions");
}

void MbDFEM::MbDAssembly::addAssembly(MbDAssembly* assembly)
{
    auto values = assemblies.getValues();
    if (assembly && assembly != this && std::find(values.begin(), values.end(), assembly) == values.end()) {
        values.push_back(assembly);
        assemblies.setValues(values);
    }

    ensureMarkersFolder();
    if (assembly && assembly != this) {
        auto* folder = ensureAssembliesFolder();
        if (folder && !folder->hasObject(assembly)) {
            folder->addObject(assembly);
        }
    }
    ensurePartsFolder();
}

void MbDFEM::MbDAssembly::addPart(MbDPart* part)
{
    auto values = parts.getValues();
    if (part && std::find(values.begin(), values.end(), part) == values.end()) {
        values.push_back(part);
        parts.setValues(values);
    }

    ensureMarkersFolder();
    if (part) {
        auto* folder = ensurePartsFolder();
        if (folder && !folder->hasObject(part)) {
            folder->addObject(part);
        }
    }
}

void MbDFEM::MbDAssembly::addMarker(MbDMarker* marker)
{
    auto values = markers.getValues();
    if (marker && std::find(values.begin(), values.end(), marker) == values.end()) {
        values.push_back(marker);
        markers.setValues(values);
    }

    if (marker) {
        auto* folder = ensureMarkersFolder();
        if (folder && !folder->hasObject(marker)) {
            folder->addObject(marker);
        }
    }
    ensurePartsFolder();
}

void MbDFEM::MbDAssembly::addJoint(MbDJoint* joint)
{
    auto values = joints.getValues();
    if (joint && std::find(values.begin(), values.end(), joint) == values.end()) {
        values.push_back(joint);
        joints.setValues(values);
    }

    if (joint) {
        auto* folder = ensureJointsFolder();
        if (folder && !folder->hasObject(joint)) {
            folder->addObject(joint);
        }
    }
    ensureMotionsFolder();
    ensureActionsFolder();
}

void MbDFEM::MbDAssembly::addMotion(MbDMotion* motion)
{
    auto values = motions.getValues();
    if (motion && std::find(values.begin(), values.end(), motion) == values.end()) {
        values.push_back(motion);
        motions.setValues(values);
    }

    if (motion) {
        auto* folder = ensureMotionsFolder();
        if (folder && !folder->hasObject(motion)) {
            folder->addObject(motion);
        }
    }
    ensureJointsFolder();
    ensureActionsFolder();
}

void MbDFEM::MbDAssembly::addAction(MbDAction* action)
{
    auto values = actions.getValues();
    if (action && std::find(values.begin(), values.end(), action) == values.end()) {
        values.push_back(action);
        actions.setValues(values);
    }

    if (action) {
        auto* folder = ensureActionsFolder();
        if (folder && !folder->hasObject(action)) {
            folder->addObject(action);
        }
    }
    ensureJointsFolder();
    ensureMotionsFolder();
}

App::DocumentObjectGroup* MbDFEM::MbDAssembly::getPartsFolder() const
{
    return dynamic_cast<App::DocumentObjectGroup*>(_partsFolder.getValue());
}

App::DocumentObjectGroup* MbDFEM::MbDAssembly::getAssembliesFolder() const
{
    return dynamic_cast<App::DocumentObjectGroup*>(_assembliesFolder.getValue());
}

App::DocumentObjectGroup* MbDFEM::MbDAssembly::getMarkersFolder() const
{
    return dynamic_cast<App::DocumentObjectGroup*>(_markersFolder.getValue());
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

App::DocumentObjectGroup* MbDFEM::MbDAssembly::ensureMarkersFolder()
{
    if (auto* folder = getMarkersFolder()) {
        return folder;
    }
    if (!getDocument()) {
        return nullptr;
    }

    const std::string name = std::string(getNameInDocument()) + "_Markers";
    auto* folder = static_cast<App::DocumentObjectGroup*>(
        getDocument()->addObject("MbDFEM::MbDMarkersFolder", name.c_str()));
    folder->Label.setValue("Markers");
    _markersFolder.setValue(folder);
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

PyObject* MbDFEM::MbDAssembly::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        PythonObject = Py::Object(new MbDAssemblyPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}
