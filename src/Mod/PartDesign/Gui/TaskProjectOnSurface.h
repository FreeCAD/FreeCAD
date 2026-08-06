// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <memory>

#include <Gui/DocumentObserver.h>
#include <Gui/Selection/Selection.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>

#include "ViewProviderProjectOnSurface.h"

namespace PartDesignGui
{

class Ui_TaskProjectOnSurface;

class TaskProjectOnSurface: public Gui::TaskView::TaskBox, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    explicit TaskProjectOnSurface(ViewProviderProjectOnSurface* view, QWidget* parent = nullptr);
    ~TaskProjectOnSurface() override;

private:
    enum class SelectionMode
    {
        None,
        Projection,
        Support
    };

    void onSelectionChanged(const Gui::SelectionChanges& msg) override;
    void setSelectionMode(SelectionMode mode, bool enabled);
    void removeSelected(bool support);
    void updateUI();
    void updateFeature();

    std::unique_ptr<Ui_TaskProjectOnSurface> ui;
    Gui::WeakPtrT<ViewProviderProjectOnSurface> vp;
    SelectionMode selectionMode {SelectionMode::None};
};

class TaskDlgProjectOnSurface: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskDlgProjectOnSurface(ViewProviderProjectOnSurface* view);

    bool accept() override;
    bool reject() override;

private:
    TaskProjectOnSurface* parameter;
    Gui::WeakPtrT<ViewProviderProjectOnSurface> vp;
};

}  // namespace PartDesignGui
