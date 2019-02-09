/******************************************************************************
 *   Copyright (c)2012 Jan Rheinlaender <jrheinlaender@users.sourceforge.net> *
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

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <Gui/TaskView/TaskDialog.h>

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
    TaskMultiTransformParameters(ViewProviderTransformed *TransformedView,QWidget *parent = 0);
    virtual ~TaskMultiTransformParameters();

    const std::vector<App::DocumentObject*> getTransformFeatures(void) const;

    /// Return the currently active subFeature
    PartDesign::Transformed* getSubFeature(void) {
        return subFeature;
    }

    virtual void apply();

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
    /// User finished editing a subFeature
    virtual void onSubTaskButtonOK();
    // Note: There is no Cancel button because I couldn't work out how to save the state of
    // a subFeature so as to revert the changes of an edit operation
    virtual void onUpdateView(bool);
    /** Notifies when the object is about to be removed. */
    virtual void slotDeletedObject(const Gui::ViewProviderDocumentObject& Obj);

protected:
    virtual void changeEvent(QEvent *e);
    virtual void onSelectionChanged(const Gui::SelectionChanges& msg);
    virtual void clearButtons();

private:
    void updateUI();
    void closeSubTask();
    void moveTransformFeature(const int increment);
    void finishAdd(std::string &newFeatName);

private:
    Ui_TaskMultiTransformParameters* ui;
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
    TaskDlgMultiTransformParameters(ViewProviderMultiTransform *MultiTransformView);
    virtual ~TaskDlgMultiTransformParameters() {}

public:
    /// is called by the framework if the dialog is accepted (Ok)
    virtual bool accept();
    /// is called by the framework if the dialog is rejected (Cancel)
    // virtual bool reject();
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
