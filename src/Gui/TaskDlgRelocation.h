/***************************************************************************
 *   Copyright (c) 2010 Jürgen Riegel <juergen.riegel@web.de>              *
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


#ifndef GUI_TASKDLGRELOCATE_H
#define GUI_TASKDLGRELOCATE_H

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>


class Ui_DlgLocationPos;
class ui_DlgLocationAngle;

namespace Gui {

class TaskBoxPosition : public Gui::TaskView::TaskBox
{
    Q_OBJECT

public:
    TaskBoxPosition(QWidget *parent = nullptr);
    ~TaskBoxPosition();


private Q_SLOTS:

protected:

private:
    QWidget* proxy;
    Ui_Position* ui;
};

class TaskBoxAngle : public Gui::TaskView::TaskBox
{
    Q_OBJECT

public:
    TaskBoxAngle(QWidget *parent = nullptr);
    ~TaskBoxAngle();


private Q_SLOTS:

protected:

private:
    QWidget* proxy;
    Ui_Angle* ui;
};


///
class GuiExport TaskDlgRelocation : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgRelocation();
    ~TaskDlgRelocation();

public:
    /// is called the TaskView when the dialog is opened
    virtual void open();
    /// is called by the framework if an button is clicked which has no accept or rject role
    virtual void clicked(int);
    /// is called by the framework if the dialog is accepted (Ok)
    virtual bool accept();
    /// is called by the framework if the dialog is rejected (Cancel)
    virtual bool reject();
    /// is called by the framework if the user press the help button
    virtual void helpRequested();

    /// returns for Close and Help button
    virtual QDialogButtonBox::StandardButtons getStandardButtons(void) const
	{ return QDialogButtonBox::Ok|QDialogButtonBox::Cancel; }

protected:

};



} //namespace Gui

#endif // ROBOTGUI_TASKDLGSIMULATE_H
