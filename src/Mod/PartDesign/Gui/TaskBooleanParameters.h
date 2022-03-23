/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer                                    *
 *                                   <jrheinlaender@users.sourceforge.net> *
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

#ifndef GUI_TASKVIEW_TaskBooleanParameters_H
#define GUI_TASKVIEW_TaskBooleanParameters_H

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>

#include "ViewProviderBoolean.h"


class Ui_TaskBooleanParameters;

namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}


namespace PartDesignGui {

class TaskBooleanParameters : public Gui::TaskView::TaskBox, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    TaskBooleanParameters(ViewProviderBoolean *BooleanView, QWidget *parent=nullptr);
    ~TaskBooleanParameters();

    const std::vector<std::string> getBodies(void) const;
    int getType(void) const;

private Q_SLOTS:
    void onButtonBodyAdd(const bool checked);
    void onButtonBodyRemove(const bool checked);
    void onBodyDeleted(void);
    void onTypeChanged(int index);

protected:
    void exitSelectionMode();

protected:
    void changeEvent(QEvent *e);
    virtual void onSelectionChanged(const Gui::SelectionChanges& msg);

private:
    QWidget* proxy;
    std::unique_ptr<Ui_TaskBooleanParameters> ui;
    ViewProviderBoolean *BooleanView;

    enum selectionModes { none, bodyAdd, bodyRemove };
    selectionModes selectionMode;

};

/// simulation dialog for the TaskView
class TaskDlgBooleanParameters : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgBooleanParameters(ViewProviderBoolean *BooleanView);
    ~TaskDlgBooleanParameters();

    ViewProviderBoolean* getBooleanView() const
    { return BooleanView; }


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
    virtual bool isAllowedAlterDocument(void) const
    { return false; }

    /// returns for Close and Help button
    virtual QDialogButtonBox::StandardButtons getStandardButtons(void) const
    { return QDialogButtonBox::Ok|QDialogButtonBox::Cancel; }

protected:
    ViewProviderBoolean   *BooleanView;

    TaskBooleanParameters  *parameter;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
