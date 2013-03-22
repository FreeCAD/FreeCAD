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

namespace FemGui {

class TaskFemConstraint : public Gui::TaskView::TaskBox, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    TaskFemConstraint(ViewProviderFemConstraint *ConstraintView,QWidget *parent = 0,const char* pixmapname = "");
    virtual ~TaskFemConstraint() {}

    virtual const std::string getReferences(void) const {return std::string();}
    const std::string getReferences(const std::vector<std::string>& items) const;

protected Q_SLOTS:
    void onReferenceDeleted(const int row);
    void onButtonReference(const bool pressed = true);
    // Shaft Wizard integration
    void onButtonWizOk();
    void onButtonWizCancel();

protected:
    virtual void changeEvent(QEvent *e) { TaskBox::changeEvent(e); }
    const QString makeRefText(const App::DocumentObject* obj, const std::string& subName) const;
    virtual void keyPressEvent(QKeyEvent * ke);

private:
    virtual void onSelectionChanged(const Gui::SelectionChanges&) {}

protected:
    QWidget* proxy;
    ViewProviderFemConstraint *ConstraintView;
    enum {seldir, selref, selloc, selnone} selectionMode;

private:
    // This seems to be the only way to access the widgets again in order to remove them from the dialog
    QDialogButtonBox* buttonBox;
    QPushButton* okButton;
    QPushButton* cancelButton;
};

/// simulation dialog for the TaskView
class TaskDlgFemConstraint : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    /*
    /// is called the TaskView when the dialog is opened
    virtual void open() {}
    /// is called by the framework if an button is clicked which has no accept or reject role
    virtual void clicked(int) {}
    /// is called by the framework if the dialog is accepted (Ok)
    */
    virtual bool accept();
    /// is called by the framework if the dialog is rejected (Cancel)
    virtual bool reject();
    /// is called by the framework if the user presses the help button
    virtual bool isAllowedAlterDocument(void) const
    { return false; }

    /// returns for Close and Help button
    virtual QDialogButtonBox::StandardButtons getStandardButtons(void) const
    { return QDialogButtonBox::Ok|QDialogButtonBox::Cancel; }

    ViewProviderFemConstraint* getConstraintView() const
    { return ConstraintView; }

protected:
    ViewProviderFemConstraint *ConstraintView;
    TaskFemConstraint  *parameter;
};

} //namespace FemGui

#endif // GUI_TASKVIEW_TaskFemConstraint_H
