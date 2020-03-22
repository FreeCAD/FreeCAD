/***************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer                                    *
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


#ifndef GUI_TASKVIEW_TaskDressUpParameters_H
#define GUI_TASKVIEW_TaskDressUpParameters_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>

#include "TaskFeatureParameters.h"
#include "ViewProviderDressUp.h"

class QAction;
class QListWidget;
class QListWidgetItem;

namespace Part {
    class Feature;
}

namespace PartDesignGui {

class TaskDressUpParameters : public Gui::TaskView::TaskBox, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    TaskDressUpParameters(ViewProviderDressUp *DressUpView, bool selectEdges, bool selectFaces, QWidget* parent = 0);
    virtual ~TaskDressUpParameters();

    const std::vector<std::string> getReferences(void) const;
    Part::Feature *getBase(void) const;

    void hideObject();
    void showObject();
    void setupTransaction();

    /// Apply the changes made to the object to it
    virtual void apply() {}

    int getTransactionID() const {
        return transactionID;
    }

protected Q_SLOTS:
    void onButtonRefAdd(const bool checked);
    void onButtonRefRemove(const bool checked);
    void doubleClicked(QListWidgetItem* item);
    void setSelection(QListWidgetItem* current);
    void itemClickedTimeout();
    virtual void onRefDeleted(void) = 0;
    void createDeleteAction(QListWidget* parentList, QWidget* parentButton);

protected:
    void exitSelectionMode();
    bool referenceSelected(const Gui::SelectionChanges& msg);
    bool wasDoubleClicked = false;
    bool KeyEvent(QEvent *e);

protected:
    enum selectionModes { none, refAdd, refRemove, plane, line };
    virtual void clearButtons(const selectionModes notThis) = 0;
    static void removeItemFromListWidget(QListWidget* widget, const char* itemstr);

    ViewProviderDressUp* getDressUpView() const
    { return DressUpView; }

protected:
    QWidget* proxy;
    ViewProviderDressUp *DressUpView;
    QAction* deleteAction;

    bool allowFaces, allowEdges;
    selectionModes selectionMode;    
    int transactionID;
};

/// simulation dialog for the TaskView
class TaskDlgDressUpParameters : public TaskDlgFeatureParameters
{
    Q_OBJECT

public:
    TaskDlgDressUpParameters(ViewProviderDressUp *DressUpView);
    virtual ~TaskDlgDressUpParameters();

    ViewProviderDressUp* getDressUpView() const
    { return static_cast<ViewProviderDressUp*>(vp); }

public:
    /// is called by the framework if the dialog is accepted (Ok)
    virtual bool accept();
    virtual bool reject();

protected:
    TaskDressUpParameters  *parameter;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TaskDressUpParameters_H
