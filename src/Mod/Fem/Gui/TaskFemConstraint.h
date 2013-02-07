/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer <jrheinlaender@users.sourceforge.net>        *
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


#ifndef GUI_TASKVIEW_TaskFemConstraint_H
#define GUI_TASKVIEW_TaskFemConstraint_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <Gui/TaskView/TaskDialog.h>

#include "ViewProviderFemConstraint.h"

class Ui_TaskFemConstraint;

namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}

namespace FemGui {

class TaskFemConstraint : public Gui::TaskView::TaskBox, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    TaskFemConstraint(ViewProviderFemConstraint *ConstraintView,QWidget *parent = 0);
    ~TaskFemConstraint();

    int getType(void) const;
    double getForce(void) const;
    const std::string getReferences(void) const;
    const std::string getDirectionName(void) const;
    const std::string getDirectionObject(void) const;
    const std::string getLocationName(void) const;
    const std::string getLocationObject(void) const;
    double getDistance(void) const;
    bool getReverse(void) const;
    double getDiameter(void) const;
    double getOtherDiameter(void) const;
    double getCenterDistance(void) const;

private Q_SLOTS:
    void onTypeChanged(int);
    void onReferenceDeleted();
    void onForceChanged(double);
    void onButtonReference(const bool pressed = true);
    void onButtonDirection(const bool pressed = true);
    void onButtonLocation(const bool pressed = true);
    void onDistanceChanged(double);
    void onCheckReverse(bool);
    void onDiameterChanged(double);
    void onOtherDiameterChanged(double);
    void onCenterDistanceChanged(double);

protected:
    void changeEvent(QEvent *e);

private:
    void onSelectionChanged(const Gui::SelectionChanges& msg);
    void updateUI();

private:
    QWidget* proxy;
    Ui_TaskFemConstraint* ui;
    ViewProviderFemConstraint *ConstraintView;
    enum {seldir, selref, selloc, selnone} selectionMode;
};

/// simulation dialog for the TaskView
class TaskDlgFemConstraint : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgFemConstraint(ViewProviderFemConstraint *ConstraintView);
    ~TaskDlgFemConstraint();

    ViewProviderFemConstraint* getConstraintView() const
    { return ConstraintView; }


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
    ViewProviderFemConstraint   *ConstraintView;

    TaskFemConstraint  *parameter;
};

} //namespace FemGui

#endif // GUI_TASKVIEW_TaskFemConstraint_H
