// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *   Copyright (c) 2026 Boyer Pierre-Louis <pierrelouis.boyer@gmail.com>    *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#pragma once

#include <memory>
#include <vector>

#include <Gui/Selection/Selection.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Gui/ComboLinks.h>

#include "TaskPatternParameters.h"

namespace Part
{
class LinkArray;
class LinkArrayCircular;
class LinkArrayLinear;
class LinkArrayPath;
class LinkArrayPoint;
class LinkArrayPolar;
}  // namespace Part

class Ui_TaskLinkArrayParameters;

namespace Gui
{
class View3DInventorViewer;
}

namespace PartGui
{

class PatternInstanceControls;

class PartGuiExport TaskLinkArrayParameters: public Gui::TaskView::TaskBox,
                                             public Gui::SelectionObserver,
                                             protected PartGui::TaskPatternParameters
{
public:
    explicit TaskLinkArrayParameters(Part::LinkArray* array, QWidget* parent = nullptr);
    ~TaskLinkArrayParameters() override;

    bool accept();
    bool reject();
    void exitLinkedObjectSelectionMode();
    void exitReferenceSelectionMode();

protected:
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;

private:
    App::DocumentObject* getPatternObject() const override;
    void fillDirectionCombo(Gui::ComboLinks& combo, Part::LinearPatternDirection direction) override;
    void onReferenceSelectionRequested() override;
    void onPatternParametersChanged() override;
    void setupPatternTransaction() override;
    void recomputePatternFeature() override;
    Base::Vector3d getPatternStartPoint() const override;
    Base::Vector3d getLinearPatternFallbackDirection(
        Part::LinearPatternDirection direction
    ) const override;
    Base::Vector3d transformLinearPatternDirection(const Base::Vector3d& direction) const override;
    void transformPolarPatternAxis(gp_Ax2& axis) const override;

    void setupLinkedObjectButton();
    void updateLinkedObjectButton();
    void enterLinkedObjectSelectionMode();
    void applyLinkedObjectSelection(App::DocumentObject* linked);
    void applyInitialSelection();
    bool isUsefulLinkedObject(App::DocumentObject* obj) const;
    App::DocumentObject* getSelectedLinkedObject() const;
    void enterReferenceSelectionMode();
    void setupInstanceControls(Gui::View3DInventorViewer* viewer);
    void updateInstanceControls();
    void setInstanceSuppressed(int index, bool suppress);

    std::unique_ptr<Ui_TaskLinkArrayParameters> ui;
    std::unique_ptr<PatternInstanceControls> instanceControls;
    QWidget* proxy = nullptr;
    Gui::View3DInventorViewer* instanceControlsViewer = nullptr;
    Part::LinkArray* array = nullptr;
    bool blockUpdate = false;
    bool linkedObjectSelectionMode = false;
    bool referenceSelectionMode = false;
    std::vector<Base::Vector3d> instanceControlCenters;
    std::vector<bool> instanceControlCentersValid;
};

class PartGuiExport TaskDlgLinkArrayParameters: public Gui::TaskView::TaskDialog
{
public:
    explicit TaskDlgLinkArrayParameters(Part::LinkArray* array);

    bool accept() override;
    bool reject() override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    }

private:
    TaskLinkArrayParameters* parameter = nullptr;
};

}  // namespace PartGui
