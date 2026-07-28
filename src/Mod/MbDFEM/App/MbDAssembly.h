// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <App/DocumentObject.h>
#include <App/DocumentObjectGroup.h>
#include <App/PropertyGeo.h>
#include <App/PropertyLinks.h>
#include <Mod/MbDFEM/MbDFEMGlobal.h>

namespace MbDFEM
{

class MbDMarker;
class MbDPart;
class MbDJoint;
class MbDMotion;
class MbDAction;

class MbDFEMExport MbDAssembly: public App::DocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(MbDFEM::MbDAssembly);

public:
    MbDAssembly();
    ~MbDAssembly() override = default;

    App::PropertyPlacement Placement;
    App::PropertyLinkList assemblies;
    App::PropertyLinkList parts;
    App::PropertyLinkList markers;
    App::PropertyLinkList joints;
    App::PropertyLinkList motions;
    App::PropertyLinkList actions;

    void addAssembly(MbDAssembly* assembly);
    void addPart(MbDPart* part);
    void addMarker(MbDMarker* marker);
    void addJoint(MbDJoint* joint);
    void addMotion(MbDMotion* motion);
    void addAction(MbDAction* action);

    PyObject* getPyObject() override;

    App::DocumentObjectGroup* getAssembliesFolder() const;
    App::DocumentObjectGroup* getPartsFolder() const;
    App::DocumentObjectGroup* getMarkersFolder() const;
    App::DocumentObjectGroup* getJointsFolder() const;
    App::DocumentObjectGroup* getMotionsFolder() const;
    App::DocumentObjectGroup* getActionsFolder() const;

    const char* getViewProviderName() const override
    {
        return "MbDFEMGui::ViewProviderMbDAssembly";
    }

private:
    App::PropertyLink _assembliesFolder;
    App::PropertyLink _partsFolder;
    App::PropertyLink _markersFolder;
    App::PropertyLink _jointsFolder;
    App::PropertyLink _motionsFolder;
    App::PropertyLink _actionsFolder;

    App::DocumentObjectGroup* ensureAssembliesFolder();
    App::DocumentObjectGroup* ensurePartsFolder();
    App::DocumentObjectGroup* ensureMarkersFolder();
    App::DocumentObjectGroup* ensureJointsFolder();
    App::DocumentObjectGroup* ensureMotionsFolder();
    App::DocumentObjectGroup* ensureActionsFolder();
};

}  // namespace MbDFEM
