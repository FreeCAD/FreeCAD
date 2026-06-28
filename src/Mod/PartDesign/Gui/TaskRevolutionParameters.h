// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <Mod/PartDesign/App/FeatureRevolution.h>
#include <Mod/PartDesign/App/FeatureGroove.h>
#include "TaskSketchBasedParameters.h"


class Ui_TaskRevolutionParameters;

namespace App
{
class Property;
}

namespace Gui
{
class RadialGizmo;
class Gizmo;
class ViewProvider;
class ViewProviderCoordinateSystem;
}  // namespace Gui

namespace PartDesignGui
{
class ViewProviderRevolution;
class ViewProviderGroove;

class TaskRevolutionParameters: public TaskSketchBasedParameters
{
    Q_OBJECT

public:
    TaskRevolutionParameters(
        ViewProvider* RevolutionView,
        const char* pixname,
        const QString& title,
        QWidget* parent = nullptr
    );
    ~TaskRevolutionParameters() override;

    void apply() override;

    /**
     * @brief fillAxisCombo fills the combo and selects the item according to
     * current value of revolution object's axis reference.
     * @param forceRefill if true, the combo box will be completely refilled. If
     * false, the current value of revolution object's axis will be added to the
     * list (if necessary), and selected. If the list is empty, it will be refilled anyway.
     */
    void fillAxisCombo(bool forceRefill = false);
    void addAxisToCombo(
        App::DocumentObject* linkObj,
        const std::string& linkSubname,
        const QString& itemText
    );

private Q_SLOTS:
    void onAngleChanged(double);
    void onAngle2Changed(double);
    void onAxisChanged(int);
    void onMidplane(bool);
    void onReversed(bool);
    void onModeChanged(int);
    void onButtonFace(bool pressed = true);
    void onFaceName(const QString& text);

protected:
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;
    void changeEvent(QEvent* event) override;
    void getReferenceAxis(App::DocumentObject*& obj, std::vector<std::string>& sub) const;
    bool getMidplane() const;
    bool getReversed() const;
    QString getFaceName() const;
    void setupDialog();
    void setCheckboxes(PartDesign::Revolution::RevolMethod mode);

private:
    // mirrors of revolution's or groove's properties
    // should have been done by inheriting revolution and groove from common class...
    App::PropertyAngle* propAngle;
    App::PropertyAngle* propAngle2;
    App::PropertyBool* propReversed;
    App::PropertyBool* propMidPlane;
    App::PropertyLinkSub* propReferenceAxis;
    App::PropertyLinkSub* propUpToFace;

private:
    void connectSignals();
    void updateUI(int index);
    void translateModeList(int index);
    // TODO: This is common with extrude. Maybe send to superclass.
    void translateFaceName();
    void clearFaceName();
    Gui::ViewProviderCoordinateSystem* getOriginView() const;

private:
    std::unique_ptr<Ui_TaskRevolutionParameters> ui;
    QWidget* proxy;
    bool selectionFace;
    bool isGroove;
    double defaultGizmoMultFactor;

    /**
     * @brief axesInList is the list of links corresponding to axis combo; must
     * be kept in sync with the combo. A special value of zero-pointer link is
     * for "Select axis" item.
     *
     * It is a list of pointers, because properties prohibit assignment. Use new
     * when adding stuff, and delete when removing stuff.
     */
    std::vector<std::unique_ptr<App::PropertyLinkSub>> axesInList;

    std::unique_ptr<Gui::GizmoContainer> gizmoContainer;
    Gui::RadialGizmo* rotationGizmo = nullptr;
    Gui::RadialGizmo* rotationGizmo2 = nullptr;
    void setupGizmos(ViewProvider* vp);
    void setGizmoPositions();
};

class TaskDlgRevolutionParameters: public TaskDlgSketchBasedParameters
{
    Q_OBJECT

public:
    explicit TaskDlgRevolutionParameters(PartDesignGui::ViewProviderRevolution* RevolutionView);
};

class TaskDlgGrooveParameters: public TaskDlgSketchBasedParameters
{
    Q_OBJECT

public:
    explicit TaskDlgGrooveParameters(PartDesignGui::ViewProviderGroove* GrooveView);
};

}  // namespace PartDesignGui
