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

#include "TaskSketchBasedParameters.h"
#include "ViewProviderHole.h"


class Ui_TaskHoleParameters;

namespace App
{
class Property;
}

namespace Gui
{
class LinearGizmo;
class GizmoContainer;
class ViewProvider;
}  // namespace Gui

namespace PartDesign
{
class Hole;
}

namespace PartDesignGui
{


class TaskHoleParameters: public TaskSketchBasedParameters
{
    Q_OBJECT

public:
    explicit TaskHoleParameters(ViewProviderHole* HoleView, QWidget* parent = nullptr);
    ~TaskHoleParameters() override;

    void apply() override;

    bool getThreaded() const;
    long getThreadType() const;
    long getThreadSize() const;
    long getThreadClass() const;
    long getThreadFit() const;
    Base::Quantity getDiameter() const;
    long getThreadDirection() const;
    long getHoleCutType() const;
    bool getHoleCutCustomValues() const;
    Base::Quantity getHoleCutDiameter() const;
    Base::Quantity getHoleCutDepth() const;
    Base::Quantity getHoleCutCountersinkAngle() const;
    long getDepthType() const;
    Base::Quantity getDepth() const;
    long getDrillPoint() const;
    Base::Quantity getDrillPointAngle() const;
    bool getDrillForDepth() const;
    bool getTapered() const;
    Base::Quantity getTaperedAngle() const;
    bool getUseCustomThreadClearance() const;
    double getCustomThreadClearance() const;
    bool getModelThread() const;
    long getThreadDepthType() const;
    double getThreadDepth() const;
    int getBaseProfileType() const;

private Q_SLOTS:
    void holeTypeChanged(int index);
    void threadTypeChanged(int index);
    void threadSizeChanged(int index);
    void threadClassChanged(int index);
    void threadFitChanged(int index);
    void threadPitchChanged(double value);
    void threadDiameterChanged(double value);
    void threadDirectionChanged();
    void holeCutTypeChanged(int index);
    void holeCutCustomValuesChanged();
    void holeCutDiameterChanged(double value);
    void holeCutDepthChanged(double value);
    void holeCutCountersinkAngleChanged(double value);
    void depthChanged(int index);
    void depthValueChanged(double value);
    void drillPointChanged();
    void drillPointAngledValueChanged(double value);
    void drillForDepthChanged();
    void taperedChanged();
    void taperedAngleChanged(double value);
    void reversedChanged();
    void useCustomThreadClearanceChanged();
    void customThreadClearanceChanged(double value);
    void updateViewChanged(bool isChecked);
    void threadDepthTypeChanged(int index);
    void threadDepthChanged(double value);
    void baseProfileTypeChanged(int index);
    void setCutDiagram();

private:
    class Observer: public App::DocumentObserver
    {
    public:
        Observer(TaskHoleParameters* _owner, PartDesign::Hole* _hole);

    private:
        void slotChangedObject(const App::DocumentObject& Obj, const App::Property& Prop) override;
        TaskHoleParameters* owner;
        PartDesign::Hole* hole;
    };
    enum HoleTypeIndex : int
    {
        Clearance = 0,
        TapDrill = 1,
        ModeledThread = 2
    };

protected:
    void changeEvent(QEvent* e) override;
    void changedObject(const App::Document&, const App::Property& Prop);

private:
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;
    void updateHoleCutLimits(PartDesign::Hole* hole);
    void updateHoleTypeCombo();

private:
    using Connection = fastsignals::scoped_connection;
    Connection connectPropChanged;

    std::unique_ptr<Observer> observer;
    QWidget* proxy;
    std::unique_ptr<Ui_TaskHoleParameters> ui;

    std::unique_ptr<Gui::GizmoContainer> gizmoContainer;
    Gui::LinearGizmo* holeDepthGizmo = nullptr;
    void setupGizmos(ViewProviderHole* vp);
    void setGizmoPositions();
};

/// simulation dialog for the TaskView
class TaskDlgHoleParameters: public TaskDlgSketchBasedParameters
{
    Q_OBJECT

public:
    explicit TaskDlgHoleParameters(ViewProviderHole* HoleView);
    ~TaskDlgHoleParameters() override;

protected:
    TaskHoleParameters* parameter;
};

}  // namespace PartDesignGui
