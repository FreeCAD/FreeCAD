// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <App/Part.h>
#include <App/DocumentObjectGroup.h>
#include <App/PropertyGeo.h>
#include <App/PropertyLinks.h>
#include <Mod/MbDFEM/MbDFEMGlobal.h>
#include <vector>

namespace MbDFEM
{

class MbDPart;
class MbDJoint;
class MbDMotion;
class MbDAction;
class MbDGravity;
class MbDSimulationParameters;
class MbDAnimationParameters;

class MbDFEMExport MbDAssembly: public App::Part
{
    PROPERTY_HEADER_WITH_OVERRIDE(MbDFEM::MbDAssembly);

public:
    MbDAssembly();
    ~MbDAssembly() override = default;

    App::PropertyLinkList assemblies;
    App::PropertyLinkList fixedparts;
    App::PropertyLinkList parts;
    App::PropertyLinkList joints;
    App::PropertyLinkList motions;
    App::PropertyLinkList actions;

    void addAssembly(MbDAssembly* assembly);
    void addPart(MbDPart* part);
    void removePart(MbDPart* part);
    void addFixedPart(MbDPart* part);
    void removeFixedPart(MbDPart* part);
    void groundPart(MbDPart* part);
    void addJoint(MbDJoint* joint);
    void addMotion(MbDMotion* motion);
    void addAction(MbDAction* action);

    int setElementVisible(const char* element, bool visible) override;
    int isElementVisible(const char* element) const override;
    App::DocumentObject* getSubObject(const char* subname,
                                      PyObject** pyObj = nullptr,
                                      Base::Matrix4D* mat = nullptr,
                                      bool transform = true,
                                      int depth = 0) const override;
    PyObject* getPyObject() override;

    App::DocumentObjectGroup* getAssembliesFolder() const;
    App::DocumentObjectGroup* getPartsFolder() const;
    App::DocumentObjectGroup* getFixedPartsFolder() const;
    App::DocumentObjectGroup* getJointsFolder() const;
    App::DocumentObjectGroup* getMotionsFolder() const;
    App::DocumentObjectGroup* getActionsFolder() const;
    MbDGravity* getGravity() const;
    MbDSimulationParameters* getSimulationParameters() const;
    MbDAnimationParameters* getAnimationParameters() const;
    std::vector<App::DocumentObjectGroup*> getCategoryFolders() const;
    std::vector<App::DocumentObject*> getCategoryChildren() const;
    std::vector<App::DocumentObject*> getParameterObjects() const;
    void removeFromCategories(App::DocumentObject* child);
    App::DocumentObjectGroup* ensureAssembliesFolder();
    App::DocumentObjectGroup* ensurePartsFolder();
    App::DocumentObjectGroup* ensureFixedPartsFolder();
    App::DocumentObjectGroup* ensureJointsFolder();
    App::DocumentObjectGroup* ensureMotionsFolder();
    App::DocumentObjectGroup* ensureActionsFolder();
    MbDGravity* ensureGravity();
    MbDSimulationParameters* ensureSimulationParameters();
    MbDAnimationParameters* ensureAnimationParameters();

    const char* getViewProviderName() const override
    {
        return "MbDFEMGui::ViewProviderMbDAssembly";
    }

private:
    App::PropertyLink _assembliesFolder;
    App::PropertyLink _fixedPartsFolder;
    App::PropertyLink _partsFolder;
    App::PropertyLink _jointsFolder;
    App::PropertyLink _motionsFolder;
    App::PropertyLink _actionsFolder;
    App::PropertyLink _gravity;
    App::PropertyLink _simulationParameters;
    App::PropertyLink _animationParameters;

};

}  // namespace MbDFEM
