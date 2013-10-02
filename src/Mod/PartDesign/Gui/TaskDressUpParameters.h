/***************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net>        *
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


#ifndef GUI_TASKVIEW_TaskDressUpParameters_H
#define GUI_TASKVIEW_TaskDressUpParameters_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <Gui/TaskView/TaskDialog.h>

#include "ViewProviderDressUp.h"

class QListWidget;

namespace PartDesignGui {

class TaskDressUpParameters : public Gui::TaskView::TaskBox, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    TaskDressUpParameters(ViewProviderDressUp *DressUpView, QWidget *parent=0);
    virtual ~TaskDressUpParameters();

    const std::vector<std::string> getReferences(void) const;
    App::DocumentObject *getBase(void) const;

    void hideObject();
    void showObject();

    /// Apply the changes made to the object to it
    virtual void apply() {};

protected Q_SLOTS:
    void onButtonRefAdd(const bool checked);
    void onButtonRefRemove(const bool checked);
    virtual void onRefDeleted(void)=0;

protected:
    void exitSelectionMode();
    const bool referenceSelected(const Gui::SelectionChanges& msg);

protected:
    enum selectionModes { none, refAdd, refRemove, plane, line };
    virtual void clearButtons(const selectionModes notThis) = 0;
    virtual void changeEvent(QEvent *e) = 0;
    static void removeItemFromListWidget(QListWidget* widget, const char* itemstr);

    ViewProviderDressUp* getDressUpView() const
    { return DressUpView; }

protected:
    QWidget* proxy;
    ViewProviderDressUp *DressUpView;

    selectionModes selectionMode;    
};

/// simulation dialog for the TaskView
class TaskDlgDressUpParameters : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgDressUpParameters(ViewProviderDressUp *DressUpView);
    virtual ~TaskDlgDressUpParameters();

    ViewProviderDressUp* getDressUpView() const
    { return DressUpView; }


public:
    /// is called the TaskView when the dialog is opened
    virtual void open() {}
    /// is called by the framework if an button is clicked which has no accept or reject role
    virtual void clicked(int) {}
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
    ViewProviderDressUp   *DressUpView;

    TaskDressUpParameters  *parameter;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TaskDressUpParameters_H
