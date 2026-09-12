// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "TaskSketchBasedParameters.h"

#include <string>

namespace PartDesignGui
{

class ViewProviderRib;
class Ui_TaskRibParameters;
class Ui_TaskRibAdvancedParameters;

class TaskRibParameters: public TaskSketchBasedParameters
{
    Q_OBJECT

public:
    explicit TaskRibParameters(ViewProviderRib* view);
    ~TaskRibParameters() override;
    void apply() override;
    void finishSelection();
    QWidget* advancedPanel() const;

protected:
    void changeEvent(QEvent* event) override;
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;

private:
    void selectProfile(bool enabled);
    void refreshProfile();
    void refreshEnums();
    void updateVisibility();
    void updateDirection();
    void updatePullDirection();

    std::unique_ptr<Ui_TaskRibParameters> ui;
    std::unique_ptr<Ui_TaskRibAdvancedParameters> advancedUi;
    Gui::TaskView::TaskBox* advanced;
    bool pickingProfile = false;
    std::string shownProfile;
    bool profileWasVisible = false;
    std::string selectedSource;
    std::string resolvedSource;
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
