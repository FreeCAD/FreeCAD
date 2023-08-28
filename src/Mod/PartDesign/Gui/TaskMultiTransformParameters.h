/******************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/

#ifndef GUI_TASKVIEW_TaskMultiTransformParameters_H
#define GUI_TASKVIEW_TaskMultiTransformParameters_H

#include "TaskTransformedParameters.h"
#include "ViewProviderMultiTransform.h"


class Ui_TaskMultiTransformParameters;
class QModelIndex;

namespace PartDesign {
class Transformed;
}

namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}

namespace PartDesignGui {



class TaskMultiTransformParameters : public TaskTransformedParameters
{
    Q_OBJECT

public:
    explicit TaskMultiTransformParameters(ViewProviderTransformed *TransformedView,QWidget *parent = nullptr);
    ~TaskMultiTransformParameters() override;

    const std::vector<App::DocumentObject*> getTransformFeatures() const;

    /// Return the currently active subFeature
    PartDesign::Transformed* getSubFeature() {
        return subFeature;
    }

    void apply() override;

public Q_SLOTS:
    /// User finished editing a subFeature
    void onSubTaskButtonOK() override;

private Q_SLOTS:
    void onTransformDelete();
    void onTransformEdit();
    void onTransformActivated(const QModelIndex& index);
    void onTransformAddMirrored();
    void onTransformAddLinearPattern();
    void onTransformAddPolarPattern();
    void onTransformAddScaled();
    void onMoveUp();
    void onMoveDown();
    // Note: There is no Cancel button because I couldn't work out how to save the state of
    // a subFeature so as to revert the changes of an edit operation
    void onUpdateView(bool) override;
    void onFeatureDeleted() override;
    /** Notifies when the object is about to be removed. */
    void slotDeletedObject(const Gui::ViewProviderDocumentObject& Obj) override;

protected:
    void addObject(App::DocumentObject*) override;
    void removeObject(App::DocumentObject*) override;
    void changeEvent(QEvent *e) override;
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;
    void clearButtons() override;

private:
    void updateUI();
    void closeSubTask();
    void moveTransformFeature(const int increment);
    void finishAdd(std::string &newFeatName);

private:
    std::unique_ptr<Ui_TaskMultiTransformParameters> ui;
    /// The subTask and subFeature currently active in the UI
    TaskTransformedParameters* subTask;
    PartDesign::Transformed* subFeature;
    bool editHint;
};


/// simulation dialog for the TaskView
class TaskDlgMultiTransformParameters : public TaskDlgTransformedParameters
{
    Q_OBJECT

public:
    explicit TaskDlgMultiTransformParameters(ViewProviderMultiTransform *MultiTransformView);
    ~TaskDlgMultiTransformParameters() override = default;

public:
    /// is called by the framework if the dialog is accepted (Ok)
    bool accept() override;
    /// is called by the framework if the dialog is rejected (Cancel)
    // virtual bool reject();
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
