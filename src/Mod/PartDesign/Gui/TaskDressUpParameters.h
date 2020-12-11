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

#include <boost/signals2.hpp>

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>

#include "TaskFeatureParameters.h"
#include "ViewProviderDressUp.h"

class QListWidget;
class QListWidgetItem;

namespace Part {
    class Feature;
}

class Ui_TaskDressUpParameters;

namespace PartDesignGui {

class TaskDressUpParameters : public Gui::TaskView::TaskBox, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    TaskDressUpParameters(ViewProviderDressUp *DressUpView, bool selectEdges, bool selectFaces, QWidget* parent = 0);
    virtual ~TaskDressUpParameters();

    std::vector<std::string> getReferences(void) const;
    Part::Feature *getBase(void) const;

    void setupTransaction();

    void setup(QLabel *msg, QListWidget *widget, QPushButton *btnAdd, bool touched=false);

    /// Apply the changes made to the object to it
    virtual void apply() {}

    int getTransactionID() const {
        return transactionID;
    }

protected Q_SLOTS:
    void onButtonRefAdd(const bool checked);
    void onItemEntered(QListWidgetItem* current);
    void onItemSelectionChanged();
    virtual void onTimer();
    virtual void onRefDeleted(void);

protected:
    void exitSelectionMode();
    App::DocumentObject *getInEdit(std::string &subname, App::DocumentObject *base=nullptr);
    App::SubObjectT getInEdit(App::DocumentObject *base=nullptr, const char *sub=nullptr);
    bool showOnTop(bool enable, std::vector<App::SubObjectT> &&objs = {});
    bool syncItems(const std::vector<App::SubObjectT> &sels = {}, bool select=false);
    void recompute();
    bool populate(bool refresh=false);
    virtual void refresh();
    void showMessage(const char *msg=nullptr);

protected:
    enum selectionModes { none, refToggle, plane, line };
    virtual void clearButtons(const selectionModes notThis);
    virtual void onSelectionChanged(const Gui::SelectionChanges& msg);
    virtual void changeEvent(QEvent *e) = 0;
    bool event(QEvent *e);

    ViewProviderDressUp* getDressUpView() const
    { return DressUpView; }

    bool eventFilter(QObject *o, QEvent *e);

protected:
    QWidget* proxy;
    ViewProviderDressUp *DressUpView;

    bool allowFaces, allowEdges;
    selectionModes selectionMode;    
    int transactionID = 0;

    QAction* deleteAction = nullptr;
    QListWidget *listWidget = nullptr;
    QPushButton *btnAdd = nullptr;
    QLabel *messageLabel = nullptr;

    bool onTopEnabled;

    std::vector<App::SubObjectT> onTopObjs;

    boost::signals2::scoped_connection connUndo;
    boost::signals2::scoped_connection connRedo;
    boost::signals2::scoped_connection connDelete;

    QTimer *timer = nullptr;
    QObject *enteredObject = nullptr;
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
