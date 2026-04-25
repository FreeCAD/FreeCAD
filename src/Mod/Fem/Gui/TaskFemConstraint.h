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

#pragma once

#include <Gui/DocumentObserver.h>
#include <Gui/Selection/Selection.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/Fem/FemGlobal.h>

#include "ViewProviderFemConstraint.h"


class QAction;
class QListWidget;
class QListWidgetItem;

namespace FemGui
{

class TaskFemConstraint: public Gui::TaskView::TaskBox, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    explicit TaskFemConstraint(
        ViewProviderFemConstraint* ConstraintView,
        QWidget* parent = nullptr,
        const char* pixmapname = ""
    );
    ~TaskFemConstraint() override = default;

    virtual const std::string getReferences() const
    {
        return std::string();
    }
    const std::string getReferences(const std::vector<std::string>& items) const;
    const std::string getScale() const;

protected Q_SLOTS:
    void onReferenceDeleted(const int row);
    void onButtonReference(const bool pressed = true);
    void onReferenceClearList();
    void setSelection(QListWidgetItem* item);

    bool event(QEvent* event) override;

protected:
    void changeEvent(QEvent* e) override
    {
        TaskBox::changeEvent(e);
    }
    const QString makeRefText(const std::string& objName, const std::string& subName) const;
    const QString makeRefText(const App::DocumentObject* obj, const std::string& subName) const;
    void keyPressEvent(QKeyEvent* ke) override;
    void createActions(QListWidget* parentList);
    void createClearListAction(QListWidget* parentList);
    void createDeleteAction(QListWidget* parentList);
    void onSelectionChanged(const Gui::SelectionChanges&) override
    {}

protected:
    QWidget* proxy;
    QListWidget* actionList;
    QAction* clearListAction;
    QAction* deleteAction;
    Gui::WeakPtrT<ViewProviderFemConstraint> ConstraintView;
    enum
    {
        seldir,
        selref,
        selloc,
        selnone
    } selectionMode;
};

/// simulation dialog for the TaskView
class TaskDlgFemConstraint: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    /// is called the TaskView when the dialog is opened
    void open() override;
    /*
    /// is called by the framework if an button is clicked which has no accept or reject role
    virtual void clicked(int) {}
    /// is called by the framework if the dialog is accepted (Ok)
    */
    bool accept() override;
    /// is called by the framework if the dialog is rejected (Cancel)
    bool reject() override;
    bool isAllowedAlterDocument() const override
    {
        return false;
    }

    /// returns for Close and Help button
    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    }

    ViewProviderFemConstraint* getConstraintView() const
    {
        return ConstraintView;
    }

protected:
    ViewProviderFemConstraint* ConstraintView;
    TaskFemConstraint* parameter;
};

}  // namespace FemGui
