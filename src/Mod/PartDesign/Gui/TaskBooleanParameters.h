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
    explicit TaskBooleanParameters(ViewProviderBoolean *BooleanView, QWidget *parent=nullptr);
    ~TaskBooleanParameters() override;

    const std::vector<std::string> getBodies() const;
    int getType() const;

private Q_SLOTS:
    void onButtonBodyAdd(const bool checked);
    void onButtonBodyRemove(const bool checked);
    void onBodyDeleted();
    void onTypeChanged(int index);

protected:
    void exitSelectionMode();

protected:
    void changeEvent(QEvent *e) override;
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;

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
    explicit TaskDlgBooleanParameters(ViewProviderBoolean *BooleanView);
    ~TaskDlgBooleanParameters() override;

    ViewProviderBoolean* getBooleanView() const
    { return BooleanView; }


public:
    /// is called the TaskView when the dialog is opened
    void open() override;
    /// is called by the framework if an button is clicked which has no accept or reject role
    void clicked(int) override;
    /// is called by the framework if the dialog is accepted (Ok)
    bool accept() override;
    /// is called by the framework if the dialog is rejected (Cancel)
    bool reject() override;
    /// is called by the framework if the user presses the help button
    bool isAllowedAlterDocument() const override
    { return false; }

    /// returns for Close and Help button
    QDialogButtonBox::StandardButtons getStandardButtons() const override
    { return QDialogButtonBox::Ok|QDialogButtonBox::Cancel; }

protected:
    ViewProviderBoolean   *BooleanView;

    TaskBooleanParameters  *parameter;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
