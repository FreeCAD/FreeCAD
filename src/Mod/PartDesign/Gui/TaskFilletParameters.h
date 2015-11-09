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


#ifndef GUI_TASKVIEW_TaskFilletParameters_H
#define GUI_TASKVIEW_TaskFilletParameters_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <Gui/TaskView/TaskDialog.h>

#include "ViewProviderFillet.h"

class Ui_TaskFilletParameters;

namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}


namespace PartDesignGui {

class TaskFilletParameters : public Gui::TaskView::TaskBox
{
    Q_OBJECT

public:
    TaskFilletParameters(ViewProviderFillet *FilletView, QWidget *parent=0);
    ~TaskFilletParameters();

    void apply();

private Q_SLOTS:
    void onLengthChanged(double);

protected:
    double getLength(void) const;
    void changeEvent(QEvent *e);

private:

private:
    QWidget* proxy;
    Ui_TaskFilletParameters* ui;
    ViewProviderFillet *FilletView;
};

/// simulation dialog for the TaskView
class TaskDlgFilletParameters : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgFilletParameters(ViewProviderFillet *FilletView);
    ~TaskDlgFilletParameters();

    ViewProviderFillet* getFilletView() const
    { return FilletView; }


public:
    /// is called the TaskView when the dialog is opened
    virtual void open();
    /// is called by the framework if an button is clicked which has no accept or reject role
    virtual void clicked(int);
    /// is called by the framework if the dialog is accepted (Ok)
    virtual bool accept();
    /// is called by the framework if the dialog is rejected (Cancel)
    virtual bool reject();
    virtual bool isAllowedAlterDocument(void) const
    { return false; }

    /// returns for Close and Help button
    virtual QDialogButtonBox::StandardButtons getStandardButtons(void) const
    { return QDialogButtonBox::Ok|QDialogButtonBox::Cancel; }

protected:
    ViewProviderFillet   *FilletView;

    TaskFilletParameters  *parameter;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
