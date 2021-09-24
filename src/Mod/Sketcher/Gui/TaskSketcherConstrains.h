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


#ifndef GUI_TASKVIEW_TaskSketcherConstrains_H
#define GUI_TASKVIEW_TaskSketcherConstrains_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <boost_signals2.hpp>
#include <QListWidget>

namespace App {
class Property;
}

namespace SketcherGui {

class ViewProviderSketch;
class Ui_TaskSketcherConstrains;

class ConstraintView : public QListWidget
{
    Q_OBJECT

public:
    explicit ConstraintView(QWidget *parent = 0);
    ~ConstraintView();

protected:
    void contextMenuEvent (QContextMenuEvent* event);

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

class TaskSketcherConstrains : public Gui::TaskView::TaskBox, public Gui::SelectionObserver
{
    Q_OBJECT

    enum FilterValue {
        All = 0,
        Geometric = 1,
        Datums = 2,
        Named = 3,
        NonDriving = 4,
        Horizontal = 5,
        Vertical = 6,
        Coincident = 7,
        PointOnObject = 8,
        Parallel = 9,
        Perpendicular = 10,
        Tangent = 11,
        Equality = 12,
        Symmetric = 13,
        Block = 14,
        Distance = 15,
        HorizontalDistance = 16,
        VerticalDistance = 17,
        Radius = 18,
        Weight = 19,
        Diameter = 20,
        Angle = 21,
        SnellsLaw = 22,
        InternalAlignment = 23
    };

    enum class ActionTarget {
        All,
        Selected
    };

public:
    TaskSketcherConstrains(ViewProviderSketch *sketchView);
    ~TaskSketcherConstrains();

    /// Observer message from the Selection
    void onSelectionChanged(const Gui::SelectionChanges& msg);

private:
    void slotConstraintsChanged(void);
    bool isConstraintFiltered(QListWidgetItem * item);
    void change3DViewVisibilityToTrackFilter();
    void changeFilteredVisibility(bool show, ActionTarget target = ActionTarget::All);

public Q_SLOTS:
    void on_comboBoxFilter_currentIndexChanged(int);
    void on_listWidgetConstraints_itemSelectionChanged(void);
    void on_listWidgetConstraints_itemActivated(QListWidgetItem *item);
    void on_listWidgetConstraints_itemChanged(QListWidgetItem * item);
    void on_listWidgetConstraints_updateDrivingStatus(QListWidgetItem *item, bool status);
    void on_listWidgetConstraints_updateActiveStatus(QListWidgetItem *item, bool status);
    void on_listWidgetConstraints_emitCenterSelectedItems(void);
    void on_filterInternalAlignment_stateChanged(int state);
    void on_extendedInformation_stateChanged(int state);
    void on_visualisationTrackingFilter_stateChanged(int state);
    void on_showAllButton_clicked(bool);
    void on_hideAllButton_clicked(bool);
    void on_listWidgetConstraints_emitShowSelection3DVisibility();
    void on_listWidgetConstraints_emitHideSelection3DVisibility();

protected:
    void changeEvent(QEvent *e);
    ViewProviderSketch *sketchView;
    typedef boost::signals2::connection Connection;
    Connection connectionConstraintsChanged;

private:
    QWidget* proxy;
    bool inEditMode;
    std::unique_ptr<Ui_TaskSketcherConstrains> ui;
};

} //namespace SketcherGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
