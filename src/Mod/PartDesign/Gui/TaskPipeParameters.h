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

#include "TaskSketchBasedParameters.h"
#include "ViewProviderPipe.h"
#include "TaskDressUpParameters.h"


namespace App {
class Property;
}

namespace Gui {
class ButtonGroup;
class ViewProvider;
}

namespace PartDesignGui {

class Ui_TaskPipeParameters;
class Ui_TaskPipeOrientation;
class Ui_TaskPipeScaling;

/// Convenience class to maintain states between the various task boxes for pipe
class StateHandlerTaskPipe
{
public:
    enum SelectionModes {
        none = 0,
        refProfile,
        refSpine,
        refSpineEdgeAdd,
        refSpineEdgeRemove,
        refAuxSpine,
        refAuxSpineEdgeAdd,
        refAuxSpineEdgeRemove,
        refSectionAdd,
        refSectionRemove
    };

public:
    StateHandlerTaskPipe() {selectionMode = SelectionModes::none;}
    ~StateHandlerTaskPipe() {}

    // only keeping getter because task boxes shouldn't need to change this
    // and task dialog is already friend
    enum SelectionModes getSelectionMode() {return selectionMode;}

private:
    enum SelectionModes selectionMode;
    friend class TaskDlgPipeParameters;
};


class TaskPipeParameters : public TaskSketchBasedParameters
{
    Q_OBJECT

public:
    TaskPipeParameters(ViewProviderPipe *PipeView, bool newObj=false, QWidget *parent = nullptr);
    ~TaskPipeParameters();

    bool accept();

private Q_SLOTS:
    void onTangentChanged(bool checked);
    void onTransitionChanged(int);
    void onProfileButton(bool checked);
    void onDeleteEdge();

protected:
    void removeFromListWidget(QListWidget*w, QString name);
    bool referenceSelected(const Gui::SelectionChanges& msg) const;

private:
    void onSelectionChanged(const Gui::SelectionChanges& msg);
    void updateUI();
    void clearButtons();
    void exitSelectionMode();
    void setVisibilityOfSpineAndProfile();

    ViewProviderPipe* getPipeView() const
    { return static_cast<ViewProviderPipe*>(vp); }

    bool spineShow = false;
    bool profileShow = false;
    bool auxSpineShow = false;

private:
    QWidget* proxy;
    std::unique_ptr<Ui_TaskPipeParameters> ui;
    StateHandlerTaskPipe *stateHandler;
    friend class TaskDlgPipeParameters;
};

class TaskPipeOrientation : public TaskSketchBasedParameters
{
    Q_OBJECT

public:
    TaskPipeOrientation(ViewProviderPipe *PipeView, bool newObj=false, QWidget *parent = nullptr);
    virtual ~TaskPipeOrientation();


private Q_SLOTS:
    void onOrientationChanged(int);
    void updateUI(int idx);
    void onClearButton();
    void onCurvelinearChanged(bool checked);
    void onBinormalChanged(double);
    void onDeleteItem();

protected:
    void removeFromListWidget(QListWidget*w, QString name);
    bool referenceSelected(const Gui::SelectionChanges& msg) const;

private:
    void onSelectionChanged(const Gui::SelectionChanges& msg);
    void clearButtons();
    void exitSelectionMode();

private:
    QWidget* proxy;
    std::unique_ptr<Ui_TaskPipeOrientation> ui;
    StateHandlerTaskPipe *stateHandler;
    friend class TaskDlgPipeParameters;
};


class TaskPipeScaling : public TaskSketchBasedParameters
{
    Q_OBJECT

public:
    TaskPipeScaling(ViewProviderPipe *PipeView,bool newObj=false,QWidget *parent = nullptr);
    virtual ~TaskPipeScaling();

private Q_SLOTS:
    void onScalingChanged(int);
    void updateUI(int idx);
    void onDeleteSection();
    void indexesMoved();

protected:
    void removeFromListWidget(QListWidget*w, QString name);
    bool referenceSelected(const Gui::SelectionChanges& msg) const;

private:
    void onSelectionChanged(const Gui::SelectionChanges& msg);
    void clearButtons();
    void exitSelectionMode();

private:
    QWidget* proxy;
    std::unique_ptr<Ui_TaskPipeScaling> ui;
    StateHandlerTaskPipe *stateHandler;
    friend class TaskDlgPipeParameters;
};

/// simulation dialog for the TaskView
class TaskDlgPipeParameters : public TaskDlgSketchBasedParameters
{
    Q_OBJECT

public:
    TaskDlgPipeParameters(ViewProviderPipe *PipeView,bool newObj=false);
    ~TaskDlgPipeParameters();

public:
    /// is called by the framework if the dialog is accepted (Ok)
    virtual bool accept();
    /// is called by the framework if the dialog is rejected (Cancel)

protected Q_SLOTS:
    void onButtonToggled(QAbstractButton *button, bool checked);

protected:
    TaskPipeParameters  *parameter;
    TaskPipeOrientation *orientation;
    TaskPipeScaling     *scaling;

    Gui::ButtonGroup *buttonGroup;
    StateHandlerTaskPipe *stateHandler;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
