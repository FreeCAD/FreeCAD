// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   © 2026 Morten Vajhøj                                                     *
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

#ifndef MASSPROPERTIES_TASKMASSPROPERTIES_H
#define MASSPROPERTIES_TASKMASSPROPERTIES_H

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QString>
#include <QWidget>

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection/Selection.h>

#include <Base/Placement.h>

namespace MassPropertiesGui {

class TaskMassProperties : public Gui::TaskView::TaskDialog, public Gui::SelectionObserver
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
    void tryupdate();

    void createDatum(double x, double y, double z, const std::string& name, bool removeExisting = true);
    void createLCS(std::string name, bool removeExisting = true);

    void onCogDatumButtonPressed();
    void onCovDatumButtonPressed();
    void onLcsButtonPressed();
    void onCoordinateSystemChanged(std::string coordSystem);
    void onSelectCustomCoordinateSystem();
    void updateInertiaVisibility();

private:
    void quit();
    void removeTemporaryObjects();
    void clearUiFields();
    void saveResult();

    QListWidget* listWidget = nullptr;
    QLineEdit* customEdit = nullptr;
    QComboBox* unitsComboBox = nullptr;

    QLineEdit* volumeEdit = nullptr;
    QLineEdit* massEdit = nullptr;
    QLineEdit* densityEdit = nullptr;
    QLineEdit* surfaceAreaEdit = nullptr;

    QLineEdit* cogXText = nullptr;
    QLineEdit* cogYText = nullptr;
    QLineEdit* cogZText = nullptr;

    QLineEdit* covXText = nullptr;
    QLineEdit* covYText = nullptr;
    QLineEdit* covZText = nullptr;

    QLineEdit* inertiaJoxText = nullptr;
    QLineEdit* inertiaJxyText = nullptr;
    QLineEdit* inertiaJzxText = nullptr;
    QLineEdit* inertiaJoyText = nullptr;
    QLineEdit* inertiaJzyText = nullptr;
    QLineEdit* inertiaJozText = nullptr;

    QLineEdit* inertiaJxText = nullptr;
    QLineEdit* inertiaJyText = nullptr;
    QLineEdit* inertiaJzText = nullptr;

    QLineEdit* axisInertiaText = nullptr;

    QWidget* inertiaMatrixWidget = nullptr;
    QWidget* inertiaDiagWidget = nullptr;
    QWidget* inertiaLcsWidget = nullptr;
    QWidget* axisInertiaWidget = nullptr;

    bool selectingCustomCoordSystem = false;
    bool isUpdating = false;
    int unitsSchemaIndex = -1;

    Base::Placement currentDatumPlacement;
    bool hasCurrentDatumPlacement = false;

};

} // namespace MassPropertiesGui

#endif // MASSPROPERTIES_TASKMASSPROPERTIES_H
