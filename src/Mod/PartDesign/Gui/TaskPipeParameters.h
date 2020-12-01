/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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


#ifndef GUI_TASKVIEW_TaskPipeParameters_H
#define GUI_TASKVIEW_TaskPipeParameters_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <Gui/TaskView/TaskDialog.h>

#include "TaskSketchBasedParameters.h"
#include "ViewProviderPipe.h"
#include "TaskDressUpParameters.h"

class Ui_TaskPipeParameters;
class Ui_TaskPipeOrientation;
class Ui_TaskPipeScaling;


namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}

namespace PartDesignGui { 



class TaskPipeParameters : public TaskSketchBasedParameters
{
    Q_OBJECT

public:
    TaskPipeParameters(ViewProviderPipe *PipeView,bool newObj=false,QWidget *parent = 0);
    ~TaskPipeParameters();

 
private Q_SLOTS:
    void onTangentChanged(bool checked);
    void onTransitionChanged(int);
    void onButtonRefAdd(bool checked);
    void onButtonRefRemove(bool checked);
    void onBaseButton(bool checked);
    void onProfileButton(bool checked);
    void onDeleteEdge();
  
protected:
    enum selectionModes { none, refAdd, refRemove, refObjAdd, refProfile };
    selectionModes selectionMode = none;
    
    void removeFromListWidget(QListWidget*w, QString name);
    bool referenceSelected(const Gui::SelectionChanges& msg) const;

private:
    void onSelectionChanged(const Gui::SelectionChanges& msg);
    void updateUI();
    void clearButtons();
    void exitSelectionMode();

    bool spineShow = false;
    
private:
    QWidget* proxy;
    Ui_TaskPipeParameters* ui;
};

class TaskPipeOrientation : public TaskSketchBasedParameters
{
    Q_OBJECT

public:
    TaskPipeOrientation(ViewProviderPipe *PipeView,bool newObj=false,QWidget *parent = 0);
    virtual ~TaskPipeOrientation();

 
private Q_SLOTS:
    void onOrientationChanged(int);
    void onButtonRefAdd(bool checked);
    void onButtonRefRemove(bool checked);
    void updateUI(int idx);
    void onBaseButton(bool checked);
    void onClearButton();
    void onCurvelinearChanged(bool checked);
    void onBinormalChanged(double);
    void onDeleteItem();
  
protected:
    enum selectionModes { none, refAdd, refRemove, refObjAdd };
    selectionModes selectionMode = none;
    
    void removeFromListWidget(QListWidget*w, QString name);
    bool referenceSelected(const Gui::SelectionChanges& msg) const;

private:
    void onSelectionChanged(const Gui::SelectionChanges& msg);
    void clearButtons();
    void exitSelectionMode();
    
    bool auxSpineShow = false;

private:
    QWidget* proxy;
    Ui_TaskPipeOrientation* ui;
};


class TaskPipeScaling : public TaskSketchBasedParameters
{
    Q_OBJECT

public:
    TaskPipeScaling(ViewProviderPipe *PipeView,bool newObj=false,QWidget *parent = 0);
    virtual ~TaskPipeScaling();

 
private Q_SLOTS:
    void onScalingChanged(int);
    void onButtonRefAdd(bool checked);
    void onButtonRefRemove(bool checked);
    void updateUI(int idx);
    void onDeleteSection();
  
protected:
    enum selectionModes { none, refAdd, refRemove };
    selectionModes selectionMode = none;
    
    void removeFromListWidget(QListWidget*w, QString name);
    bool referenceSelected(const Gui::SelectionChanges& msg) const;

private:
    void onSelectionChanged(const Gui::SelectionChanges& msg);
    void clearButtons();
    void exitSelectionMode();

private:
    QWidget* proxy;
    Ui_TaskPipeScaling* ui;
};


/// simulation dialog for the TaskView
class TaskDlgPipeParameters : public TaskDlgSketchBasedParameters
{
    Q_OBJECT

public:
    TaskDlgPipeParameters(ViewProviderPipe *PipeView,bool newObj=false);
    ~TaskDlgPipeParameters();

    ViewProviderPipe* getPipeView() const
    { return static_cast<ViewProviderPipe*>(vp); }


public:
    /// is called by the framework if the dialog is accepted (Ok)
    virtual bool accept();
    /// is called by the framework if the dialog is rejected (Cancel)

protected:
    TaskPipeParameters  *parameter;
    TaskPipeOrientation *orientation;
    TaskPipeScaling     *scaling;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
