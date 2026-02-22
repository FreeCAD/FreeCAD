// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

#include <Gui/DocumentObserver.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/PartDesign/App/FeatureDressUp.h>

#include "TaskFeatureParameters.h"
#include "ViewProviderDressUp.h"


class QAction;
class QListWidget;
class QListWidgetItem;

namespace Part
{
class Feature;
}

namespace PartDesignGui
{

class TaskDressUpParameters: public TaskFeatureParameters, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    TaskDressUpParameters(
        ViewProviderDressUp* DressUpView,
        bool selectEdges,
        bool selectFaces,
        QWidget* parent = nullptr
    );
    ~TaskDressUpParameters() override;

    const std::vector<std::string> getReferences() const;
    Part::Feature* getBase() const;

    void setupTransaction();

    int getTransactionID() const
    {
        return transactionID;
    }

    bool event(QEvent* event) override;

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
    void keyPressEvent(QKeyEvent* ke) override;
    void hideOnError();
    void addAllEdges(QListWidget* listWidget);
    void deleteRef(QListWidget* listWidget);
    void updateFeature(PartDesign::DressUp* pcDressUp, const std::vector<std::string>& refs);

protected:
    enum selectionModes
    {
        none,
        refSel,
        plane,
        line
    };
    void setSelectionMode(selectionModes mode);
    virtual void setButtons(const selectionModes mode) = 0;
    static void removeItemFromListWidget(QListWidget* widget, const char* itemstr);

    ViewProviderDressUp* getDressUpView() const;

private:
    void tryAddSelection(const std::string& doc, const std::string& obj, const std::string& sub);
    void setDressUpVisibility(bool visible);

protected:
    QWidget* proxy;
    QAction* deleteAction;
    QAction* addAllEdgesAction;

    bool allowFaces, allowEdges;
    selectionModes selectionMode;
    int transactionID;

    static QString stopSelectionLabel();
    static QString startSelectionLabel();

private:
    Gui::WeakPtrT<ViewProviderDressUp> DressUpView;

    Gui::ViewProvider* previouslyShownViewProvider {nullptr};
};

/// simulation dialog for the TaskView
class TaskDlgDressUpParameters: public TaskDlgFeatureParameters
{
    Q_OBJECT

public:
    explicit TaskDlgDressUpParameters(ViewProviderDressUp* DressUpView);
    ~TaskDlgDressUpParameters() override;

public:
    /// is called by the framework if the dialog is accepted (Ok)
    bool accept() override;
    bool reject() override;

protected:
    TaskDressUpParameters* parameter;
};

}  // namespace PartDesignGui
