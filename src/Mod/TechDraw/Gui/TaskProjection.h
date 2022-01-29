/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
//this file originally part of TechDraw workbench
//migrated to TechDraw workbench 2022-01-26 by Wandererfan


#ifndef TECHDRAWGUI_TASKDIALOG
#define TECHDRAWGUI_TASKDIALOG

#include <QWidget>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>

class QCheckBox;

namespace TechDrawGui
{

/**
 * Embed the panel into a task dialog.
 */
class Ui_TaskProjection;
class TaskProjection : public QWidget
{
    Q_OBJECT

public:
    TaskProjection();
    ~TaskProjection();

public:
    bool accept();
    bool reject();

    virtual bool isAllowedAlterDocument(void) const
    { return true; }

private:
    std::unique_ptr<Ui_TaskProjection> ui;

};


class TaskDlgProjection : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgProjection();
    ~TaskDlgProjection();

public:
    /// is called the TaskView when the dialog is opened
    virtual void open();
    /// is called by the framework if a button is clicked which has no accept or reject role
    virtual void clicked(int);
    /// is called by the framework if the dialog is accepted (Ok)
    virtual bool accept();
    /// is called by the framework if the dialog is rejected (Cancel)
    virtual bool reject();
    /// is called by the framework if the user presses the help button
    virtual void helpRequested() { return;}

    void update();

protected:

private:
    TaskProjection* widget;
    Gui::TaskView::TaskBox* taskbox;
};

} //namespace TechDrawGui



#endif // TECHDRAWGUI_TASKDIALOG
