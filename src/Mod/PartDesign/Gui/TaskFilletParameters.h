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

#include "TaskDressUpParameters.h"
#include "ViewProviderFillet.h"

class Ui_TaskFilletParameters;

namespace PartDesignGui {

class TaskFilletParameters : public TaskDressUpParameters
{
    Q_OBJECT

public:
    explicit TaskFilletParameters(ViewProviderDressUp *DressUpView, QWidget *parent=nullptr);
    ~TaskFilletParameters() override;

    void apply() override;

private Q_SLOTS:
    void onLengthChanged(double);
    void onRefDeleted() override;
    void onAddAllEdges();
    void onCheckBoxUseAllEdgesToggled(bool checked);

protected:
    double getLength() const;
    void setButtons(const selectionModes mode) override;
    bool event(QEvent *e) override;
    void changeEvent(QEvent *e) override;
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;

private:
    std::unique_ptr<Ui_TaskFilletParameters> ui;
};

/// simulation dialog for the TaskView
class TaskDlgFilletParameters : public TaskDlgDressUpParameters
{
    Q_OBJECT

public:
    explicit TaskDlgFilletParameters(ViewProviderFillet *DressUpView);
    ~TaskDlgFilletParameters() override;

public:
    /// is called by the framework if the dialog is accepted (Ok)
    bool accept() override;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TaskFilletParameters_H
