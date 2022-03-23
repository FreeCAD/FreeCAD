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

#ifndef GUI_TASKVIEW_TaskFemConstraintForce_H
#define GUI_TASKVIEW_TaskFemConstraintForce_H

#include <QObject>

#include "TaskFemConstraintOnBoundary.h"
#include "ViewProviderFemConstraintForce.h"


class Ui_TaskFemConstraintForce;

namespace App {
class Property;
}

namespace Gui {
class SelectionObject;
class ViewProvider;
}

namespace FemGui {

class TaskFemConstraintForce : public TaskFemConstraintOnBoundary
{
    Q_OBJECT

public:
    TaskFemConstraintForce(ViewProviderFemConstraintForce *ConstraintView,QWidget *parent = nullptr);
    virtual ~TaskFemConstraintForce();
    double getForce(void) const;
    const std::string getReferences() const;
    const std::string getDirectionName(void) const;
    const std::string getDirectionObject(void) const;
    bool getReverse(void) const;

private Q_SLOTS:
    void onReferenceDeleted(void);
    void onForceChanged(double);
    void onButtonDirection(const bool pressed = false);
    void onCheckReverse(bool);
    void addToSelection();
    void removeFromSelection();

protected:
    bool event(QEvent *e);
    virtual void changeEvent(QEvent *e);
    void clearButtons(const SelectionChangeModes notThis) override;

private:
    std::pair<App::DocumentObject*, std::string> getDirection(const std::vector<Gui::SelectionObject>&) const;
    void updateUI();

private:
    Ui_TaskFemConstraintForce* ui;
};

/// simulation dialog for the TaskView
class TaskDlgFemConstraintForce : public TaskDlgFemConstraint
{
    Q_OBJECT

public:
    TaskDlgFemConstraintForce(ViewProviderFemConstraintForce *ConstraintView);

    /// is called by the framework if the dialog is accepted (Ok)
    virtual void open();
    virtual bool accept();
    virtual bool reject();

};

} //namespace FemGui

#endif // GUI_TASKVIEW_TaskFemConstraintForce_H
