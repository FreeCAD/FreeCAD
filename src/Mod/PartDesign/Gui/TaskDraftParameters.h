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


#ifndef GUI_TASKVIEW_TaskDraftParameters_H
#define GUI_TASKVIEW_TaskDraftParameters_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <Gui/TaskView/TaskDialog.h>

#include "ViewProviderDraft.h"

class Ui_TaskDraftParameters;

namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}


namespace PartDesignGui {

class TaskDraftParameters : public Gui::TaskView::TaskBox, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    TaskDraftParameters(ViewProviderDraft *DraftView, QWidget *parent=0);
    ~TaskDraftParameters();

    const double getAngle(void) const;
    const bool getReversed(void) const;
    const std::vector<std::string> getFaces(void) const;
    const std::string getPlane(void) const;
    const std::string getLine(void) const;
    App::DocumentObject *getBase(void) const;

    void hideObject();
    void showObject();

private Q_SLOTS:
    void onAngleChanged(double angle);
    void onReversedChanged(bool reversed);
    void onButtonFaceAdd(const bool checked);
    void onButtonFaceRemove(const bool checked);
    void onButtonPlane(const bool checked);
    void onButtonLine(const bool checked);
    void onFaceDeleted(void);

protected:
    void exitSelectionMode();

protected:
    void changeEvent(QEvent *e);
    virtual void onSelectionChanged(const Gui::SelectionChanges& msg);

private:
    QWidget* proxy;
    Ui_TaskDraftParameters* ui;
    ViewProviderDraft *DraftView;

    enum selectionModes { none, faceAdd, faceRemove, plane, line };
    selectionModes selectionMode;
};

/// simulation dialog for the TaskView
class TaskDlgDraftParameters : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgDraftParameters(ViewProviderDraft *DraftView);
    ~TaskDlgDraftParameters();

    ViewProviderDraft* getDraftView() const
    { return DraftView; }


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
    ViewProviderDraft   *DraftView;

    TaskDraftParameters  *parameter;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
