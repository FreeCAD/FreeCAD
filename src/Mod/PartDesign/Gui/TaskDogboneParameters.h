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


#ifndef GUI_TASKVIEW_TaskDogboneParameters_H
#define GUI_TASKVIEW_TaskDogboneParameters_H

#include "TaskDressUpParameters.h"
#include "ViewProviderDogbone.h"

class Ui_TaskDogboneParameters;

namespace PartDesignGui {

class TaskDogboneParameters : public TaskDressUpParameters
{
    Q_OBJECT

public:
    TaskDogboneParameters(ViewProviderDressUp *DressUpView, QWidget *parent=0);
    ~TaskDogboneParameters();

    virtual void apply();

private Q_SLOTS:
    void onLengthChanged(double);
    void onRefDeleted(void);

protected:
    double getRadius(void) const;
    virtual void clearButtons(const selectionModes notThis);
    void changeEvent(QEvent *e);
    virtual void onSelectionChanged(const Gui::SelectionChanges& msg);

private:
    Ui_TaskDogboneParameters* ui;
};

/// simulation dialog for the TaskView
class TaskDlgDogboneParameters : public TaskDlgDressUpParameters
{
    Q_OBJECT

public:
    TaskDlgDogboneParameters(ViewProviderDogbone *DressUpView);
    ~TaskDlgDogboneParameters();

public:
    /// is called by the framework if the dialog is accepted (Ok)
    virtual bool accept();
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TaskDogboneParameters_H
