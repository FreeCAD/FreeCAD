/***************************************************************************
 *   Copyright (c) 2014 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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


#ifndef GUI_TASKVIEW_TaskSketcherElements_H
#define GUI_TASKVIEW_TaskSketcherElements_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <boost_signals2.hpp>
#include <QListWidget>
#include <QIcon>

namespace App {
class Property;
}

namespace SketcherGui {

class ViewProviderSketch;
class Ui_TaskSketcherElements;

class ElementView : public QListWidget
{
    Q_OBJECT

public:
    explicit ElementView(QWidget *parent = nullptr);
    ~ElementView();


Q_SIGNALS:
    void onFilterShortcutPressed();
    void signalCloseShape();

protected:
    void contextMenuEvent (QContextMenuEvent* event);
    void keyPressEvent(QKeyEvent * event);

protected Q_SLOTS:
    // Constraints
    void doPointCoincidence();
    void doPointOnObjectConstraint();
    void doVerticalDistance();
    void doHorizontalDistance();
    void doParallelConstraint();
    void doPerpendicularConstraint();
    void doTangentConstraint();
    void doEqualConstraint();
    void doSymmetricConstraint();
    void doBlockConstraint();

    void doLockConstraint();
    void doHorizontalConstraint();
    void doVerticalConstraint();
    void doLengthConstraint();
    void doRadiusConstraint();
    void doDiameterConstraint();
    void doRadiamConstraint();
    void doAngleConstraint();

    // Other Commands
    void doToggleConstruction();

    // Acelerators
    void doCloseShape();
    void doConnect();
    void doSelectConstraints();
    void doSelectOrigin();
    void doSelectHAxis();
    void doSelectVAxis();
    void deleteSelectedItems();
};

class TaskSketcherElements : public Gui::TaskView::TaskBox, public Gui::SelectionObserver
{
    Q_OBJECT

    class MultIcon {
        
    public:
        MultIcon(const char*);
        
        QIcon Normal;
        QIcon Construction;
        QIcon External;
        
        QIcon getIcon(bool construction, bool external) const;
    };
    
public:
    TaskSketcherElements(ViewProviderSketch *sketchView);
    ~TaskSketcherElements();

    /// Observer message from the Selection
    void onSelectionChanged(const Gui::SelectionChanges& msg);

private:
    void slotElementsChanged(void);
    void updateIcons(int element);
    void updatePreselection();
    void updateVisibility(int filterindex);
    void setItemVisibility(int elementindex,int filterindex);
    void clearWidget();

public Q_SLOTS:
    void on_listWidgetElements_itemSelectionChanged(void);
    void on_listWidgetElements_itemEntered(QListWidgetItem *item);
    void on_listWidgetElements_filterShortcutPressed();
    void on_listWidgetElements_currentFilterChanged ( int index );
    void on_listWidgetElements_currentModeFilterChanged ( int index );
    void on_namingBox_stateChanged(int state);
    void on_autoSwitchBox_stateChanged(int state);

protected:
    void changeEvent(QEvent *e);
    void leaveEvent ( QEvent * event );
    ViewProviderSketch *sketchView;
    typedef boost::signals2::connection Connection;
    Connection connectionElementsChanged;

private:
    QWidget* proxy;
    std::unique_ptr<Ui_TaskSketcherElements> ui;
    int focusItemIndex;
    int previouslySelectedItemIndex;

    bool isNamingBoxChecked;
    bool isautoSwitchBoxChecked;

    bool inhibitSelectionUpdate;
};

} //namespace SketcherGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
