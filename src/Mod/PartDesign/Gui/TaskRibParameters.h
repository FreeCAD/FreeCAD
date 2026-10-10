// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "TaskSketchBasedParameters.h"
#include <Gui/Inventor/Draggers/Gizmo.h>
#include <gp_Ax3.hxx>

#include <string>

namespace PartDesignGui
{

class ViewProviderRib;
class Ui_TaskRibParameters;

class TaskRibParameters: public TaskSketchBasedParameters
{
    Q_OBJECT

public:
    explicit TaskRibParameters(ViewProviderRib* view);
    ~TaskRibParameters() override;
    void apply() override;
    void finishSelection();

protected:
    void changeEvent(QEvent* event) override;
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;

private:
    void selectProfile(bool enabled);
    void refreshProfile();
    gp_Ax3 sweepFrame() const;
    void refreshSweepAngle();
    void refreshEnums();
    void updateVisibility();
    void updateRib();
    void setupGizmos();
    void setGizmoPositions();

    std::unique_ptr<Ui_TaskRibParameters> ui;
    bool pickingProfile = false;
    std::string shownProfile;
    bool profileWasVisible = false;
    std::string selectedSource;
    std::string resolvedSource;
    std::unique_ptr<Gui::GizmoContainer> gizmoContainer;
    Gui::LinearGizmo* thicknessGizmo = nullptr;
    Gui::LinearGizmo* lengthGizmo = nullptr;
    Gui::RotationGizmo* draftGizmo = nullptr;
    Gui::RotationGizmo* sweepGizmo = nullptr;
    Gui::QuantitySpinBox* sweepDragAngle = nullptr;
};

class TaskDlgRibParameters: public TaskDlgSketchBasedParameters
{
public:
    explicit TaskDlgRibParameters(ViewProviderRib* view);
    bool accept() override;
    bool reject() override;

private:
    TaskRibParameters* parameters;
};

}  // namespace PartDesignGui
