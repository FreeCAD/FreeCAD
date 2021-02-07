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


#ifndef GUI_TASKVIEW_TaskLoftParameters_H
#define GUI_TASKVIEW_TaskLoftParameters_H

#include <boost_signals2.hpp>

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <Gui/TaskView/TaskDialog.h>

#include "TaskSketchBasedParameters.h"
#include "ViewProviderLoft.h"

class Ui_TaskLoftParameters;
class QListWidget;
class QListWidgetItem;

namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}

namespace PartDesignGui {


class TaskLoftParameters : public TaskSketchBasedParameters
{
    Q_OBJECT

public:
    TaskLoftParameters(ViewProviderLoft *LoftView,bool newObj=false,QWidget *parent = 0);
    ~TaskLoftParameters();

    void exitSelectionMode();

private Q_SLOTS:
    void onProfileButton(bool);
    void onRefButtonAdd(bool);
    void onClosed(bool);
    void onRuled(bool);
    void onDeleteSection();
    void indexesMoved();
    void onItemEntered(QListWidgetItem *);
    void onItemSelectionChanged();
    void updateUI();

protected:
    void changeEvent(QEvent *e);
    bool eventFilter(QObject *o, QEvent *e);
    void refresh();

private:
    void onSelectionChanged(const Gui::SelectionChanges& msg);
    void clearButtons();
    void addItem(App::DocumentObject *obj, bool select=false);

private:
    QWidget* proxy;
    std::unique_ptr<Ui_TaskLoftParameters> ui;

    enum selectionModes { none, refAdd, refProfile };
    selectionModes selectionMode = none;

    App::SubObjectT lastProfile;
    boost::signals2::scoped_connection connProfile;
    std::vector<App::SubObjectT> lastSections;
    boost::signals2::scoped_connection connSections;
};

/// simulation dialog for the TaskView
class TaskDlgLoftParameters : public TaskDlgSketchBasedParameters
{
    Q_OBJECT

public:
    TaskDlgLoftParameters(ViewProviderLoft *LoftView,bool newObj=false);
    ~TaskDlgLoftParameters();

    ViewProviderLoft* getLoftView() const
    { return static_cast<ViewProviderLoft*>(vp); }

    /// is called by the framework if the dialog is accepted (Ok)
    virtual bool accept();

protected:
    TaskLoftParameters  *parameter;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
