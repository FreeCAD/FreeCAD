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

#include <QStyledItemDelegate>
#include <QListWidget>

#include <boost_signals2.hpp>

#include <Gui/Selection.h>
#include <Gui/TaskView/TaskView.h>


namespace App {
class Property;
}

namespace SketcherGui {

class ViewProviderSketch;
class Ui_TaskSketcherElements;



class ElementItem;
class ElementView;

// Struct to identify the selection/preselection of a subelement of the item
enum class SubElementType {
    edge,
    start,
    end,
    mid,
    none
};

class ElementItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ElementItemDelegate(ElementView* parent);
    ~ElementItemDelegate() override;

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;

    ElementItem* getElementtItem(const QModelIndex& index) const;

    const int border = 1; //1px, looks good around buttons.
    const int leftMargin = 4; //4px on the left of icons, looks good.
    mutable int customIconsMargin = 4;
    const int textBottomMargin = 5; //5px center the text.

Q_SIGNALS:
    void itemHovered(QModelIndex);
    void itemChecked(QModelIndex, Qt::CheckState state);
};

class ElementView : public QListWidget
{
    Q_OBJECT

public:
    explicit ElementView(QWidget *parent = nullptr);
    ~ElementView() override;
    ElementItem* itemFromIndex(const QModelIndex& index);

protected:
    void contextMenuEvent (QContextMenuEvent* event) override;

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
    void doSelectConstraints();
    void doSelectOrigin();
    void doSelectHAxis();
    void doSelectVAxis();
    void deleteSelectedItems();

    void onIndexHovered(QModelIndex index);
    void onIndexChecked(QModelIndex, Qt::CheckState state);

Q_SIGNALS:
    void onItemHovered(QListWidgetItem *);

private:
    void changeLayer(int layer);
};

class ElementFilterList : public QListWidget
{
    Q_OBJECT

public:
    explicit ElementFilterList(QWidget* parent = nullptr);
    ~ElementFilterList() override;

protected:
    void changeEvent(QEvent* e) override;
    virtual void languageChange();

private:
    // {filter item text, filter item level}
    using filterItemRepr =  std::pair<const char *, const int>;
    inline static const std::vector<filterItemRepr> filterItems = {
        {QT_TR_NOOP("Normal"),0},
        {QT_TR_NOOP("Construction"),0},
        {QT_TR_NOOP("Internal"),0},
        {QT_TR_NOOP("External"),0},
        {QT_TR_NOOP("All types"),0},
        {QT_TR_NOOP("Point"),1},
        {QT_TR_NOOP("Line"),1},
        {QT_TR_NOOP("Circle"),1},
        {QT_TR_NOOP("Ellipse"),1},
        {QT_TR_NOOP("Arc of circle"),1},
        {QT_TR_NOOP("Arc of ellipse"),1},
        {QT_TR_NOOP("Arc of hyperbola"),1},
        {QT_TR_NOOP("Arc of parabola"),1},
        {QT_TR_NOOP("B-Spline"),1}
    };

};

class TaskSketcherElements : public Gui::TaskView::TaskBox, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    explicit TaskSketcherElements(ViewProviderSketch *sketchView);
    ~TaskSketcherElements() override;

    /// Observer message from the Selection
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;

private:
    void slotElementsChanged();
    void updateVisibility();
    void setItemVisibility(QListWidgetItem* item);
    void clearWidget();
    void createFilterButtonActions();
    void createSettingsButtonActions();
    void connectSignals();

public Q_SLOTS:
    void onListWidgetElementsItemPressed(QListWidgetItem* item);
    void onListWidgetElementsItemEntered(QListWidgetItem *item);
    void onListWidgetElementsMouseMoveOnItem(QListWidgetItem* item);
    void onSettingsExtendedInformationChanged();
    void onFilterBoxStateChanged(int val);
    void onListMultiFilterItemChanged(QListWidgetItem* item);

protected:
    void changeEvent(QEvent *e) override;
    void leaveEvent ( QEvent * event ) override;
    ViewProviderSketch *sketchView;
    using Connection = boost::signals2::connection;
    Connection connectionElementsChanged;

private:
    QWidget* proxy;
    std::unique_ptr<Ui_TaskSketcherElements> ui;
    int focusItemIndex;
    int previouslySelectedItemIndex;
    int previouslyHoveredItemIndex;
    SubElementType previouslyHoveredType;

    ElementFilterList* filterList;

    bool isNamingBoxChecked;
};

} //namespace SketcherGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
