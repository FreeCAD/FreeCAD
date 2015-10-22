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


#ifndef GUI_TASKVIEW_TaskHoleParameters_H
#define GUI_TASKVIEW_TaskHoleParameters_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <Gui/TaskView/TaskDialog.h>

#include "ViewProviderHole.h"

class Ui_TaskHoleParameters;

namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}

namespace PartDesignGui { 



class TaskHoleParameters : public Gui::TaskView::TaskBox, public Gui::SelectionSingleton::ObserverType
{
    Q_OBJECT

public:
    TaskHoleParameters(QWidget *parent = 0);
    ~TaskHoleParameters();
    /// Observer message from the Selection
    void OnChange(Gui::SelectionSingleton::SubjectType &rCaller,
                  Gui::SelectionSingleton::MessageType Reason);

private Q_SLOTS:

protected:
    void changeEvent(QEvent *e);

private:

private:
    QWidget* proxy;
    Ui_TaskHoleParameters* ui;
};

/// simulation dialog for the TaskView
class TaskDlgHoleParameters : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgHoleParameters(ViewProviderHole *HoleView);
    ~TaskDlgHoleParameters();

    ViewProviderHole* getHoleView() const
    { return HoleView; }

public:
    /// is called the TaskView when the dialog is opened
    virtual void open();
    /// is called by the framework if an button is clicked which has no accept or reject role
    virtual void clicked(int);
    /// is called by the framework if the dialog is accepted (Ok)
    virtual bool accept();
    /// is called by the framework if the dialog is rejected (Cancel)
    virtual bool reject();
    /// is called by the framework if the user presses the help button 
    virtual void helpRequested();
    virtual bool isAllowedAlterDocument(void) const
    { return false; }

    /// returns for Close and Help button 
    virtual QDialogButtonBox::StandardButtons getStandardButtons(void) const
    { return QDialogButtonBox::Close|QDialogButtonBox::Help; }

protected:
    ViewProviderHole   *HoleView;

    TaskHoleParameters  *parameter;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
