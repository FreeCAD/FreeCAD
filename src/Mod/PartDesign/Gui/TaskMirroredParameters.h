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


#ifndef GUI_TASKVIEW_TaskMirroredParameters_H
#define GUI_TASKVIEW_TaskMirroredParameters_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <Gui/TaskView/TaskDialog.h>

#include "TaskTransformedParameters.h"
#include "ViewProviderMirrored.h"

class Ui_TaskMirroredParameters;

namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}

namespace PartDesignGui {

class TaskMultiTransformParameters;

class TaskMirroredParameters : public TaskTransformedParameters
{
    Q_OBJECT

public:
    /// Constructor for task with ViewProvider
    TaskMirroredParameters(ViewProviderTransformed *TransformedView, QWidget *parent = 0);
    /// Constructor for task with parent task (MultiTransform mode)
    TaskMirroredParameters(TaskMultiTransformParameters *parentTask, QLayout *layout);

    virtual ~TaskMirroredParameters();

    const QString getMirrorPlane(void) const;
    const std::string getStdMirrorPlane(void) const;
    const bool updateView() const;

private Q_SLOTS:
    void onButtonXY();
    void onButtonXZ();
    void onButtonYZ();
    void onButtonReference(const bool checked);
    virtual void onUpdateView(bool);

protected:
    virtual void changeEvent(QEvent *e);
    virtual void onSelectionChanged(const Gui::SelectionChanges& msg);

    bool referenceSelectionMode;

private:
    void onStdMirrorPlane(const std::string& plane);
    void setupUI();
    void updateUI();
    void exitSelectionMode();

private:
    Ui_TaskMirroredParameters* ui;
};


/// simulation dialog for the TaskView
class TaskDlgMirroredParameters : public TaskDlgTransformedParameters
{
    Q_OBJECT

public:
    TaskDlgMirroredParameters(ViewProviderMirrored *MirroredView);
    virtual ~TaskDlgMirroredParameters() {}

public:
    /// is called by the framework if the dialog is accepted (Ok)
    virtual bool accept();
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
