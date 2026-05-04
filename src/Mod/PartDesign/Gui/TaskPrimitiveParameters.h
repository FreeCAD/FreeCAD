// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefeantroeger@gmx.net>             *
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

#include <memory>
#include <unordered_map>

#include <Gui/DocumentObserver.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>

#include "ViewProviderPrimitive.h"
#include "TaskDatumParameters.h"
#include "TaskFeatureParameters.h"

namespace App
{
class Property;
}

namespace Gui
{
class ViewProvider;
class GizmoContainer;
class LinearGizmo;
}  // namespace Gui

namespace PartDesignGui
{
class Ui_DlgPrimitives;
class TaskBoxPrimitives: public Gui::TaskView::TaskBox, public Gui::DocumentObserver
{
    Q_OBJECT

public:
    explicit TaskBoxPrimitives(ViewProviderPrimitive* vp, QWidget* parent = nullptr);
    ~TaskBoxPrimitives() override;

    bool setPrimitive(App::DocumentObject*);

public Q_SLOTS:
    void onBoxLengthChanged(double);
    void onBoxWidthChanged(double);
    void onBoxHeightChanged(double);
    void onCylinderRadiusChanged(double);
    void onCylinderHeightChanged(double);
    void onCylinderXSkewChanged(double);
    void onCylinderYSkewChanged(double);
    void onCylinderAngleChanged(double);
    void onSphereRadiusChanged(double);
    void onSphereAngle1Changed(double);
    void onSphereAngle2Changed(double);
    void onSphereAngle3Changed(double);
    void onConeRadius1Changed(double);
    void onConeRadius2Changed(double);
    void onConeAngleChanged(double);
    void onConeHeightChanged(double);
    void onEllipsoidRadius1Changed(double);
    void onEllipsoidRadius2Changed(double);
    void onEllipsoidRadius3Changed(double);
    void onEllipsoidAngle1Changed(double);
    void onEllipsoidAngle2Changed(double);
    void onEllipsoidAngle3Changed(double);
    void onTorusRadius1Changed(double);
    void onTorusRadius2Changed(double);
    void onTorusAngle1Changed(double);
    void onTorusAngle2Changed(double);
    void onTorusAngle3Changed(double);
    void onPrismCircumradiusChanged(double);
    void onPrismHeightChanged(double);
    void onPrismXSkewChanged(double);
    void onPrismYSkewChanged(double);
    void onPrismPolygonChanged(int);
    void onWedgeXmaxChanged(double);
    void onWedgeXminChanged(double);
    void onWedgeYmaxChanged(double);
    void onWedgeYminChanged(double);
    void onWedgeZmaxChanged(double);
    void onWedgeZminChanged(double);
    void onWedgeX2maxChanged(double);
    void onWedgeX2minChanged(double);
    void onWedgeZ2maxChanged(double);
    void onWedgeZ2minChanged(double);

    void onPlacementChanged();

private:
    /** Notifies when the object is about to be removed. */
    void slotDeletedObject(const Gui::ViewProviderDocumentObject& Obj) override;

    template<typename T = App::DocumentObject>
    T* getObject() const
    {
        static_assert(std::is_base_of<App::DocumentObject, T>::value, "Wrong template argument");
        if (vp) {
            return vp->getObject<T>();
        }

        return nullptr;
    }

private:
    QWidget* proxy;
    std::unique_ptr<Ui_DlgPrimitives> ui;
    ViewProviderPrimitive* vp;

    std::unique_ptr<Gui::GizmoContainer> gizmoContainer;
    Gui::LinearGizmo* lengthGizmo = nullptr;
    Gui::LinearGizmo* heightGizmo = nullptr;
    Gui::LinearGizmo* widthGizmo = nullptr;
    Gui::LinearGizmo* radiusGizmo = nullptr;
    void setupGizmos();
    void setGizmoPositions();
};

class TaskDlgPrimitiveParameters: public TaskDlgFeatureParameters
{
    Q_OBJECT

public:
    explicit TaskDlgPrimitiveParameters(ViewProviderPrimitive* PrimitiveView);
    ~TaskDlgPrimitiveParameters() override;

protected:
    QDialogButtonBox::StandardButtons getStandardButtons() const override;

    bool accept() override;
    bool reject() override;

private:
    TaskBoxPrimitives* primitive;
    PartGui::TaskAttacher* parameter;
    ViewProviderPrimitive* vp_prm;
};

}  // namespace PartDesignGui
