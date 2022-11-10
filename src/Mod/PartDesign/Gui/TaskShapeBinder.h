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


#ifndef GUI_TASKVIEW_TaskDatumShapeBinder_H
#define GUI_TASKVIEW_TaskDatumShapeBinder_H

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Gui/DocumentObserver.h>

#include "ViewProviderShapeBinder.h"

class Ui_TaskShapeBinder;
class QListWidget;

namespace App {
class Property;
}

namespace Gui {
class ButtonGroup;
class ViewProvider;
}

namespace PartDesignGui {



class TaskShapeBinder : public Gui::TaskView::TaskBox, Gui::SelectionObserver
{
    Q_OBJECT

public:
    explicit TaskShapeBinder(ViewProviderShapeBinder *view,bool newObj=false,QWidget *parent = nullptr);
    ~TaskShapeBinder() override;

    void accept();

protected:
    enum selectionModes { none, refAdd, refRemove, refObjAdd };
    void changeEvent(QEvent *e) override;
    selectionModes selectionMode = none;

    void removeFromListWidget(QListWidget *w, QString name);
    bool referenceSelected(const Gui::SelectionChanges& msg) const;

private:
    void setupButtonGroup();
    void setupContextMenu();
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;
    void onButtonToggled(QAbstractButton *button, bool checked);
    void updateUI();
    void supportChanged(const QString&);
    void clearButtons();
    void deleteItem();
    void exitSelectionMode();

    bool supportShow = false;

private:
    QWidget* proxy;
    std::unique_ptr<Ui_TaskShapeBinder> ui;
    Gui::ButtonGroup *buttonGroup;
    Gui::WeakPtrT<ViewProviderShapeBinder> vp;
};


/// simulation dialog for the TaskView
class TaskDlgShapeBinder : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskDlgShapeBinder(ViewProviderShapeBinder *view,bool newObj=false);
    ~TaskDlgShapeBinder() override;

public:
    /// is called by the framework if the dialog is accepted (Ok)
    bool accept() override;
    /// is called by the framework if the dialog is rejected (Cancel)
    bool reject() override;

protected:
    TaskShapeBinder  *parameter;
    Gui::WeakPtrT<ViewProviderShapeBinder> vp;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
