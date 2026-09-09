// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once
#include "TaskSketchBasedParameters.h"
#include "ViewProviderPad.h"
#include <Gui/Inventor/Draggers/Gizmo.h>

#include <array>

namespace Gui
{
class PrefQuantitySpinBox;
}

namespace PartDesignGui
{
class Ui_TaskRibParameters;
class Ui_TaskRibAdvancedParameters;

class TaskRibParameters: public TaskSketchBasedParameters
{
    Q_OBJECT

public:
    explicit TaskRibParameters(ViewProviderPad* view);
    ~TaskRibParameters() override;
    void apply() override;
    void finishSelection();
    QWidget* advancedPanel() const;

protected:
    void changeEvent(QEvent* event) override;

private:
    enum class Pick
    {
        None,
        Profile,
        Target,
        Direction,
        Pull
    };
    void select(Pick mode);
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;
    void refresh();
    void updateVisibility();
    void updateFeature();
    void setupGizmos();
    void setGizmoPositions();
    Pick picking = Pick::None;
    bool profileVisible = false;
    Gui::TaskView::TaskBox* advanced;
    std::unique_ptr<Ui_TaskRibParameters> ui;
    std::unique_ptr<Ui_TaskRibAdvancedParameters> advancedUi;
    std::array<Gui::PrefQuantitySpinBox*, 3> direction;
    std::array<Gui::PrefQuantitySpinBox*, 3> pullVector;
    std::vector<Gui::PrefQuantitySpinBox*> quantities;
    Gui::LinearGizmo* thicknessGizmo = nullptr;
    Gui::LinearGizmo* thickness2Gizmo = nullptr;
    Gui::LinearGizmo* lengthGizmo = nullptr;
    Gui::RotationGizmo* draftGizmo = nullptr;
    std::unique_ptr<Gui::GizmoContainer> gizmoContainer;
};

class TaskDlgRibParameters: public TaskDlgSketchBasedParameters
{
public:
    explicit TaskDlgRibParameters(ViewProviderPad* view);
    bool accept() override;
    bool reject() override;

private:
    TaskRibParameters* parameters;
};
}  // namespace PartDesignGui
