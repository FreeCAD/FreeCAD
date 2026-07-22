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

    enum threadSelectionModes
    {
        None,
        SideFaceSel,  // Seleção da face lateral
        StartFaceSel  // Seleção da face de início da rosca
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
    void changedObject(const App::Document&, const App::Property& Prop);

private Q_SLOTS:
    void threadTypeChanged(int index);
    void threadSizeChanged(int index);
    void threadSizePitchChanged(int index);
    void depthChanged(int index);
    void threadClassChanged(int index);
    void threadDirectionChanged(int index);
    void CustomClearanceCheckValuesChanged();
    void threadModelChanged();

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

    void setUpUI(PartDesign::Thread* pcThread);

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
