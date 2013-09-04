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


#ifndef GUI_TASKVIEW_TaskTransformedParameters_H
#define GUI_TASKVIEW_TaskTransformedParameters_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <Gui/TaskView/TaskDialog.h>

#include "TaskTransformedMessages.h"
#include "ViewProviderTransformed.h"

namespace PartDesign {
class Transformed;
}

namespace PartDesignGui {

class TaskMultiTransformParameters;

/**
  The transformed subclasses will be used in two different modes:
  1. As a stand-alone feature
  2. As a container that stores transformation info for a MultiTransform feature. In this case
     the flag insideMultiTransform is set to true.
  Because in the second case there is no ViewProvider, some special methods are required to
  access the underlying FeatureTransformed object in two different ways.
  **/
class TaskTransformedParameters : public Gui::TaskView::TaskBox, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    /// Constructor for task with ViewProvider
    TaskTransformedParameters(ViewProviderTransformed *TransformedView, QWidget *parent = 0);
    /// Constructor for task with parent task (MultiTransform mode)
    TaskTransformedParameters(TaskMultiTransformParameters *parentTask);
    virtual ~TaskTransformedParameters();

    const std::vector<App::DocumentObject*> getOriginals(void) const;
    /// Get the support object either of the object associated with this feature or with the parent feature (MultiTransform mode)
    App::DocumentObject* getSupportObject() const;
    /// Get the sketch object of the first original either of the object associated with this feature or with the parent feature (MultiTransform mode)
    App::DocumentObject* getSketchObject() const;

    void exitSelectionMode();

protected Q_SLOTS:
    /// Connect the subTask OK button to the MultiTransform task
    virtual void onSubTaskButtonOK() {}

protected:
    const bool originalSelected(const Gui::SelectionChanges& msg);

    /// Get the TransformedFeature object associated with this task
    // Either through the ViewProvider or the currently active subFeature of the parentTask
    PartDesign::Transformed *getObject() const;
    /// Recompute either this feature or the parent feature (MultiTransform mode)
    void recomputeFeature();

    void hideObject();
    void showObject();
    void hideOriginals();
    void showOriginals();

    void addReferenceSelectionGate(bool edge, bool face);
    bool isViewUpdated() const;
    int getUpdateViewTimeout() const;

protected:
    virtual void changeEvent(QEvent *e) = 0;
    virtual void onSelectionChanged(const Gui::SelectionChanges& msg) = 0;

protected:
    QWidget* proxy;
    ViewProviderTransformed *TransformedView;

    bool originalSelectionMode;
    bool referenceSelectionMode;

    /// The MultiTransform parent task of this task
    TaskMultiTransformParameters* parentTask;
    /// Flag indicating whether this object is a container for MultiTransform
    bool insideMultiTransform;
    /// Lock updateUI(), applying changes to the underlying feature and calling recomputeFeature()
    bool blockUpdate;
};

/// simulation dialog for the TaskView
class TaskDlgTransformedParameters : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgTransformedParameters(ViewProviderTransformed *TransformedView);
    virtual ~TaskDlgTransformedParameters() {}

    ViewProviderTransformed* getTransformedView() const
    { return TransformedView; }

public:
    /// is called the TaskView when the dialog is opened
    virtual void open()
        {}
    /// is called by the framework if an button is clicked which has no accept or reject role
    virtual void clicked(int)
        {}
    /// is called by the framework if the dialog is accepted (Ok)
    virtual bool accept();
    /// is called by the framework if the dialog is rejected (Cancel)
    virtual bool reject();
    /// is called by the framework if the user presses the help button
    virtual bool isAllowedAlterDocument(void) const
        { return false; }

    /// returns for Close and Help button
    virtual QDialogButtonBox::StandardButtons getStandardButtons(void) const
        { return QDialogButtonBox::Ok|QDialogButtonBox::Cancel; }

protected:
    ViewProviderTransformed   *TransformedView;

    TaskTransformedParameters  *parameter;
    TaskTransformedMessages  *message;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
