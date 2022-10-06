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
#include <QStyledItemDelegate>

namespace App {
class Property;
}

namespace SketcherGui {

class ViewProviderSketch;
class Ui_TaskSketcherElements;

enum class ElementType { edge, start, end, mid, none }; //This is to identify the type of element selected

// helper class to store additional information about the listWidget entry.
class ElementItem : public QListWidgetItem
{
    public:

    enum class GeometryState {
        Normal,
        Construction,
        InternalAlignment,
        External
    };

    ElementItem(int elementnr, int startingVertex, int midVertex, int endVertex,
        Base::Type geometryType, GeometryState state, const QString & lab) :
        ElementNbr(elementnr)
        , StartingVertex(startingVertex)
        , MidVertex(midVertex)
        , EndVertex(endVertex)
        , GeometryType(geometryType)
        , State(state)
        , isLineSelected(false)
        , isStartingPointSelected(false)
        , isEndPointSelected(false)
        , isMidPointSelected(false)
        , clickedOn(ElementType::none)
        , hovered(ElementType::none)
        , rightClicked(false)
        , label(lab)
    {}

    ~ElementItem() override {
    }

    int ElementNbr;
    int StartingVertex;
    int MidVertex;
    int EndVertex;

    Base::Type GeometryType;
    GeometryState State;

    bool isLineSelected;
    bool isStartingPointSelected;
    bool isEndPointSelected;
    bool isMidPointSelected;


    ElementType clickedOn;
    ElementType hovered;
    bool rightClicked;

    QString label;
};

class ElementView;

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
    const int textBottomMargin = 5; //5px center the text.

Q_SIGNALS:
    void itemHovered(QModelIndex);
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

Q_SIGNALS:
    void onItemHovered(QListWidgetItem *);
};

class TaskSketcherElements : public Gui::TaskView::TaskBox, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    explicit TaskSketcherElements(ViewProviderSketch *sketchView);
    ~TaskSketcherElements() override;

    /// Observer message from the Selection
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;

    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void slotElementsChanged();
    void updateVisibility();
    void setItemVisibility(QListWidgetItem* item);
    void clearWidget();
    void createSettingsButtonActions();

public Q_SLOTS:
    void on_listWidgetElements_itemPressed(QListWidgetItem* item);
    void on_listWidgetElements_itemEntered(QListWidgetItem *item);
    void on_listWidgetElements_mouseMoveOnItem(QListWidgetItem* item);
    void on_settings_extendedInformation_changed();
    void on_settingsButton_clicked(bool);
    void on_filterBox_stateChanged(int val);
    void on_listMultiFilter_itemChanged(QListWidgetItem* item);

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
    ElementType previouslyHoveredType;

    bool isNamingBoxChecked;
};

} //namespace SketcherGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
