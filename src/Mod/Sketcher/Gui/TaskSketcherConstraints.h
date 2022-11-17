/***************************************************************************
 *   Copyright (c) 2009 Jürgen Riegel <juergen.riegel@web.de>              *
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


#ifndef GUI_TASKVIEW_TaskSketcherConstraints_H
#define GUI_TASKVIEW_TaskSketcherConstraints_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <boost_signals2.hpp>
#include <QListWidget>

#include <Mod/Sketcher/App/Constraint.h>

#include "ConstraintFilters.h"

namespace App {
class Property;
}

namespace SketcherGui {

using namespace ConstraintFilter;

class ViewProviderSketch;
class Ui_TaskSketcherConstraints;

class ConstraintView : public QListWidget
{
    Q_OBJECT

public:
    explicit ConstraintView(QWidget *parent = nullptr);
    ~ConstraintView() override;

protected:
    void contextMenuEvent (QContextMenuEvent* event) override;

Q_SIGNALS:
    void onUpdateDrivingStatus(QListWidgetItem *item, bool status);
    void onUpdateActiveStatus(QListWidgetItem *item, bool status);
    void emitCenterSelectedItems();
    void emitHideSelection3DVisibility();
    void emitShowSelection3DVisibility();

protected Q_SLOTS:
    void modifyCurrentItem();
    void renameCurrentItem();
    void centerSelectedItems();
    void deleteSelectedItems();
    void doSelectConstraints();
    void updateDrivingStatus();
    void updateActiveStatus();
    void swapNamedOfSelectedItems();
    void showConstraints();
    void hideConstraints();
};

class ConstraintFilterList : public QListWidget
{
    Q_OBJECT

public:
    explicit ConstraintFilterList(QWidget* parent = nullptr);
    ~ConstraintFilterList() override;

    FilterValueBitset getMultiFilter();

    int normalFilterCount; //All filters but selected and associated
    int selectedFilterIndex;
    int associatedFilterIndex;
};

class TaskSketcherConstraints : public Gui::TaskView::TaskBox, public Gui::SelectionObserver
{
    Q_OBJECT

    enum class ActionTarget {
        All,
        Selected
    };

    enum class SpecialFilterType {
        None,
        Associated,
        Selected
    };

public:
    explicit TaskSketcherConstraints(ViewProviderSketch *sketchView);
    ~TaskSketcherConstraints() override;

    /// Observer message from the Selection
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;

    SpecialFilterType specialFilterMode;

private:
    void slotConstraintsChanged();
    bool isConstraintFiltered(QListWidgetItem * item);
    void change3DViewVisibilityToTrackFilter();
    void changeFilteredVisibility(bool show, ActionTarget target = ActionTarget::All);
    void updateSelectionFilter();
    void updateAssociatedConstraintsFilter();
    void updateList();
    void createFilterButtonActions();
    void createSettingButtonActions();
    void connectSignals();

    void getSelectionGeoId(QString expr, int & geoid, Sketcher::PointPos & pos);

public Q_SLOTS:
    void on_listWidgetConstraints_itemSelectionChanged();
    void on_listWidgetConstraints_itemActivated(QListWidgetItem *item);
    void on_listWidgetConstraints_itemChanged(QListWidgetItem * item);
    void on_listWidgetConstraints_updateDrivingStatus(QListWidgetItem *item, bool status);
    void on_listWidgetConstraints_updateActiveStatus(QListWidgetItem *item, bool status);
    void on_listWidgetConstraints_emitCenterSelectedItems();
    void on_listWidgetConstraints_emitShowSelection3DVisibility();
    void on_listWidgetConstraints_emitHideSelection3DVisibility();
    void on_filterBox_stateChanged(int val);
    void on_showHideButton_clicked(bool);
    void on_settings_restrictVisibility_changed();
    void on_settings_extendedInformation_changed();
    void on_settings_hideInternalAligment_changed();
    void on_filterList_itemChanged(QListWidgetItem* item);

protected:
    void changeEvent(QEvent *e) override;
    ViewProviderSketch *sketchView;
    using Connection = boost::signals2::connection;
    Connection connectionConstraintsChanged;

private:
    QWidget* proxy;
    bool inEditMode;
    std::unique_ptr<Ui_TaskSketcherConstraints> ui;
    ConstraintFilter::FilterValueBitset multiFilterStatus; // Stores the filters to be aggregated to form the multifilter.
    std::vector<unsigned int> selectionFilter; // holds the constraint ids of the selected constraints
    std::vector<unsigned int> associatedConstraintsFilter; // holds the constraint ids of the constraints associated with the selected geometry
    ConstraintFilterList* filterList;
};

} //namespace SketcherGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
