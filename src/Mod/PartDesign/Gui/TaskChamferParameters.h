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


#ifndef GUI_TASKVIEW_TaskChamferParameters_H
#define GUI_TASKVIEW_TaskChamferParameters_H

#include "TaskDressUpParameters.h"
#include "ViewProviderChamfer.h"

class Ui_TaskChamferParameters;
namespace PartDesign {
class Chamfer;
}

namespace PartDesignGui {

class TaskChamferParameters : public TaskDressUpParameters
{
    Q_OBJECT

public:
    TaskChamferParameters(ViewProviderDressUp *DressUpView, QWidget *parent=nullptr);
    ~TaskChamferParameters();

    virtual void apply();

private Q_SLOTS:
    void onTypeChanged(int);
    void onSizeChanged(double);
    void onSize2Changed(double);
    void onAngleChanged(double);
    void onFlipDirection(bool);
    void onRefDeleted(void);
    void onAddAllEdges(void);
    void onCheckBoxUseAllEdgesToggled(bool checked);

protected:
    virtual void clearButtons(const selectionModes notThis);
    bool event(QEvent *e);
    void changeEvent(QEvent *e);
    virtual void onSelectionChanged(const Gui::SelectionChanges& msg);

    int getType(void) const;
    double getSize(void) const;
    double getSize2(void) const;
    double getAngle(void) const;
    bool getFlipDirection(void) const;

private:
    void setUpUI(PartDesign::Chamfer* pcChamfer);

    std::unique_ptr<Ui_TaskChamferParameters> ui;
};

/// simulation dialog for the TaskView
class TaskDlgChamferParameters : public TaskDlgDressUpParameters
{
    Q_OBJECT

public:
    TaskDlgChamferParameters(ViewProviderChamfer *DressUpView);
    ~TaskDlgChamferParameters();

public:
    /// is called by the framework if the dialog is accepted (Ok)
    virtual bool accept();
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TaskChamferParameters_H
