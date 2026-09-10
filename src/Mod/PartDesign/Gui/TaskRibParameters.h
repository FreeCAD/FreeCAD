// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "TaskSketchBasedParameters.h"

namespace PartDesignGui
{

class ViewProviderRib;
class Ui_TaskRibParameters;

class TaskRibParameters: public TaskFeatureParameters
{
    Q_OBJECT

public:
    explicit TaskRibParameters(ViewProviderRib* view);
    ~TaskRibParameters() override;

protected:
    void changeEvent(QEvent* event) override;

private:
    std::unique_ptr<Ui_TaskRibParameters> ui;
};

class TaskDlgRibParameters: public TaskDlgSketchBasedParameters
{
public:
    explicit TaskDlgRibParameters(ViewProviderRib* view);
};

}  // namespace PartDesignGui
