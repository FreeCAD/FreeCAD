// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2026 Caio Venâncio <caio.venancio784@gmail.com>          *
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

    enum threadSelectionModes
    {
        None,
        SideFaceSel,
        StartFaceSel,
        UpToGeometrySel
    };
    threadSelectionModes currentSelectionMode = None;

    void onRefDeleted() override;
    void setButtons(const PartDesignGui::TaskDressUpParameters::selectionModes mode) override;
    void onSelectionChanged(const Gui::SelectionChanges& change) override;
    void QLineEditSelected(const QString& text);

    void apply() override;

protected:
    void changeEvent(QEvent* e) override;
    void setThreadSelectionMode(threadSelectionModes mode);
    void changedObject(const App::Document&, const App::Property& Prop);
    void setThreadSelectionGate();
    void setLinkSubText(QLineEdit* edit, const App::PropertyLinkSub& prop);

private Q_SLOTS:
    void threadTypeChanged(int index);
    void threadSizeChanged(int index);
    void threadSizePitchChanged(int index);
    void depthChanged(double value);
    void depthTypeChanged(int index);
    void threadClassChanged(int index);
    void threadDirectionChanged(int index);
    void CustomClearanceCheckValuesChanged();
    void threadModelChanged();
    void customThreadClearanceChanged(double value);

private:
    class Observer: public App::DocumentObserver
    {
    public:
        Observer(TaskThreadParameters* _owner, PartDesign::Thread* _thread);

    private:
        void slotChangedObject(const App::DocumentObject& Obj, const App::Property& Prop) override;
        TaskThreadParameters* owner;
        PartDesign::Thread* thread;
    };

    std::unique_ptr<Ui_TaskThreadParameters> ui;

    using Connection = fastsignals::scoped_connection;
    Connection connectPropChanged;
    std::unique_ptr<Observer> observer;
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
