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
#include <Mod/PartDesign/App/FeatureDressUp.h>

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
    TaskDressUpParameters(ViewProviderDressUp *DressUpView, bool selectEdges, bool selectFaces, QWidget* parent = nullptr);
    ~TaskDressUpParameters() override;

    const std::vector<std::string> getReferences() const;
    Part::Feature *getBase() const;

    void hideObject();
    void showObject();
    void setupTransaction();

    /// Apply the changes made to the object to it
    virtual void apply() {}

    int getTransactionID() const {
        return transactionID;
    }

protected Q_SLOTS:
    void onButtonRefSel(const bool checked);
    void doubleClicked(QListWidgetItem* item);
    void setSelection(QListWidgetItem* current);
    void itemClickedTimeout();
    virtual void onRefDeleted() = 0;
    void createDeleteAction(QListWidget* parentList);
    void createAddAllEdgesAction(QListWidget* parentList);

protected:
    void referenceSelected(const Gui::SelectionChanges& msg, QListWidget* widget);
    bool wasDoubleClicked = false;
    bool KeyEvent(QEvent *e);
    void hideOnError();
    void addAllEdges(QListWidget* listWidget);
    void deleteRef(QListWidget* listWidget);
    void updateFeature(PartDesign::DressUp* pcDressUp, const std::vector<std::string>& refs);

protected:
    enum selectionModes { none, refSel, plane, line };
    void setSelectionMode(selectionModes mode);
    virtual void setButtons(const selectionModes mode) = 0;
    static void removeItemFromListWidget(QListWidget* widget, const char* itemstr);

    ViewProviderDressUp* getDressUpView() const
    { return DressUpView; }

protected:
    QWidget* proxy;
    ViewProviderDressUp *DressUpView;
    QAction* deleteAction;
    QAction* addAllEdgesAction;

    bool allowFaces, allowEdges;
    selectionModes selectionMode;
    int transactionID;

    static const QString btnPreviewStr();
    static const QString btnSelectStr();
};

/// simulation dialog for the TaskView
class TaskDlgDressUpParameters : public TaskDlgFeatureParameters
{
    Q_OBJECT

public:
    explicit TaskDlgDressUpParameters(ViewProviderDressUp *DressUpView);
    ~TaskDlgDressUpParameters() override;

    ViewProviderDressUp* getDressUpView() const
    { return static_cast<ViewProviderDressUp*>(vp); }

public:
    /// is called by the framework if the dialog is accepted (Ok)
    bool accept() override;
    bool reject() override;

protected:
    TaskDressUpParameters  *parameter;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TaskDressUpParameters_H
