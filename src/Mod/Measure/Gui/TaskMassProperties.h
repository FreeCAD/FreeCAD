// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Morten Vajhøj
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#pragma once

#include <QDialogButtonBox>
#include <QEvent>

#include <string>
#include <vector>

#include <Gui/Action.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection/Selection.h>

#include <Base/Placement.h>

#include "Mod/Measure/App/MassPropertiesResult.h"

namespace MassPropertiesGui
{

class TaskMassPropertiesWidget;

class TaskMassProperties: public Gui::TaskView::TaskDialog, public Gui::SelectionObserver
{
public:
    TaskMassProperties();
    ~TaskMassProperties() override;

    void modifyStandardButtons(QDialogButtonBox* box) override;
    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Apply | QDialogButtonBox::Abort | QDialogButtonBox::Reset;
    }

    void invoke();
    bool accept() override;
    bool reject() override;

    void onSelectionChanged(const Gui::SelectionChanges& msg);
    void update(const Gui::SelectionChanges& msg);
    void tryUpdate();

    void createDatum(const Base::Vector3d& position, const std::string& name, bool removeExisting = true);
    void createLCS(std::string name, bool removeExisting = true);

    void onCogDatumButtonPressed();
    void onCovDatumButtonPressed();
    void onLcsButtonPressed();
    void onCoordinateSystemChanged(MassPropertiesMode coordSystemMode);
    void onSelectCustomCoordinateSystem();
    void updateInertiaVisibility();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void escape();
    void removeTemporaryObjects();
    void clearUiFields();
    void saveResult();

    TaskMassPropertiesWidget* panel = nullptr;

    bool selectingCustomCoordSystem = false;
    bool isUpdating = false;
    int unitsSchemaIndex = -1;

    Base::Placement currentDatumPlacement;
    bool hasCurrentDatumPlacement = false;

    Gui::Action* deleteAction = nullptr;
    bool deleteActivated = false;

    std::vector<MassPropertiesInput> objectsToMeasure;
};

}  // namespace MassPropertiesGui
