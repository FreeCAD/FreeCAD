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


#ifndef GUI_TASKVIEW_TaskPadParameters_H
#define GUI_TASKVIEW_TaskPadParameters_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <Gui/TaskView/TaskDialog.h>

#include "ViewProviderPad.h"

class Ui_TaskPadParameters;

namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}

namespace PartDesignGui { 



class TaskPadParameters : public Gui::TaskView::TaskBox, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    TaskPadParameters(ViewProviderPad *PadView,QWidget *parent = 0);
    ~TaskPadParameters();

    int getMode(void) const;
    double getLength(void) const;
    double getLength2(void) const;
    bool   getReversed(void) const;
    bool   getMidplane(void) const;
    QByteArray getFaceName(void) const;
    const bool updateView() const;

private Q_SLOTS:
    void onLengthChanged(double);
    void onMidplane(bool);
    void onReversed(bool);
    void onLength2Changed(double);
    void onModeChanged(int);
    void onButtonFace(const bool pressed = true);
    void onFaceName(const QString& text);
    void onUpdateView(bool);

protected:
    void changeEvent(QEvent *e);

private:
    void onSelectionChanged(const Gui::SelectionChanges& msg);
    void updateUI(int index);

private:
    QWidget* proxy;
    Ui_TaskPadParameters* ui;
    ViewProviderPad *PadView;
};

/// simulation dialog for the TaskView
class TaskDlgPadParameters : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgPadParameters(ViewProviderPad *PadView);
    ~TaskDlgPadParameters();

    ViewProviderPad* getPadView() const
    { return PadView; }


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
    ViewProviderPad   *PadView;

    TaskPadParameters  *parameter;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
