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


#ifndef GUI_TASKVIEW_TaskThicknessParameters_H
#define GUI_TASKVIEW_TaskThicknessParameters_H

#include "TaskDressUpParameters.h"
#include "ViewProviderThickness.h"

class Ui_TaskThicknessParameters;

namespace PartDesignGui {

class TaskThicknessParameters : public TaskDressUpParameters
{
    Q_OBJECT

public:
    TaskThicknessParameters(ViewProviderDressUp *DressUpView, QWidget *parent=nullptr);
    ~TaskThicknessParameters();

    double getValue(void) const;
    bool getReversed(void) const;
    bool getIntersection(void) const;
    int  getMode(void) const;
    int  getJoinType(void) const;

private Q_SLOTS:
    void onValueChanged(double angle);
    void onModeChanged(int mode);
    void onJoinTypeChanged(int join);
    void onReversedChanged(bool reversed);
    void onIntersectionChanged(bool intersection);
    void onRefDeleted(void);

protected:
    virtual void clearButtons(const selectionModes notThis);
    bool event(QEvent *e);
    void changeEvent(QEvent *e);
    virtual void onSelectionChanged(const Gui::SelectionChanges& msg);

private:
    std::unique_ptr<Ui_TaskThicknessParameters> ui;
};

/// simulation dialog for the TaskView
class TaskDlgThicknessParameters : public TaskDlgDressUpParameters
{
    Q_OBJECT

public:
    TaskDlgThicknessParameters(ViewProviderThickness *ThicknessView);
    ~TaskDlgThicknessParameters();

public:
    /// is called by the framework if the dialog is accepted (Ok)
    virtual bool accept();
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
