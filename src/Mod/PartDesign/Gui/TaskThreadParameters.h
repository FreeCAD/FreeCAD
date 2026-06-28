// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "TaskDressUpParameters.h"
#include "ViewProviderThread.h"
#include <memory>

class Ui_TaskThreadParameters;

namespace PartDesign
{
class Thread;
}

namespace Gui
{
class LinearGizmo;
class RotationalGizmo;
class GizmoContainer;
}  // namespace Gui

namespace PartDesignGui
{

class TaskThreadParameters: public TaskDressUpParameters
{
    Q_OBJECT

public:
    TaskThreadParameters(ViewProviderDressUp* DressUpView, QWidget* parent = nullptr);
    ~TaskThreadParameters() override;

    enum threadSelectionModes {
        None,
        SideFaceSel,   // Seleção da face lateral
        StartFaceSel   // Seleção da face de início da rosca
    };
    threadSelectionModes currentSelectionMode = None;

    void onRefDeleted() override;
    void setButtons(const PartDesignGui::TaskDressUpParameters::selectionModes mode) override;
    void onSelectionChanged(const Gui::SelectionChanges& change) override;
    void QLineEditSelected(const QString& text);

    void apply() override;

protected:
    void changeEvent(QEvent* e) override;
    // void referenceSele(const Gui::SelectionChanges& msg, QLineEdit* widget);
    void setThreadSelectionMode(threadSelectionModes mode);

private Q_SLOTS:
    void threadTypeChanged(int index);

private:
    std::unique_ptr<Ui_TaskThreadParameters> ui;

    void setUpUI(PartDesign::Thread* pcThread);
};

class TaskDlgThreadParameters: public TaskDlgDressUpParameters
{
    Q_OBJECT

public:
    explicit TaskDlgThreadParameters(ViewProviderThread* DressUpView);
    ~TaskDlgThreadParameters() override;

    /// is called by the framework if the dialog is accepted (Ok)
    bool accept() override;
};

}  // namespace PartDesignGui
