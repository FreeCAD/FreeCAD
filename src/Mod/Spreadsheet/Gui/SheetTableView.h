/***************************************************************************
 *   Copyright (c) Eivind Kvedalen (eivind@kvedalen.name) 2015             *
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

#ifndef SHEETTABLEVIEW_H
#define SHEETTABLEVIEW_H

#include <QTableView>
#include <QHeaderView>
#include <QKeyEvent>
#include <Mod/Spreadsheet/App/Sheet.h>
#include <Mod/Spreadsheet/App/Utils.h>

namespace SpreadsheetGui {

class SheetViewHeader : public QHeaderView {
    Q_OBJECT
public:
    SheetViewHeader(QTableView *owner, Qt::Orientation o) 
        : QHeaderView(o),owner(owner) 
    {
        setSectionsClickable(true);
    }
Q_SIGNALS:
    void resizeFinished();
protected:
    void mouseReleaseEvent(QMouseEvent * event);
    bool viewportEvent(QEvent *e);
private:
    QTableView *owner;
};

class SheetTableView : public QTableView
{
    Q_OBJECT
public:
    explicit SheetTableView(QWidget *parent = nullptr);
    ~SheetTableView();
    
    void edit(const QModelIndex &index);
    void setSheet(Spreadsheet::Sheet *_sheet);
    std::vector<App::Range> selectedRanges() const;
    QModelIndexList selectedIndexesRaw() const;
    QString toHtml() const;

public Q_SLOTS:
    void mergeCells();
    void splitCell();
    void deleteSelection();
    void copySelection();
    void cutSelection();
    void pasteClipboard();
    void finishEditWithMove(int keyPressed, Qt::KeyboardModifiers modifiers, bool handleTabMotion = false);
    void ModifyBlockSelection(int targetRow, int targetColumn);

protected Q_SLOTS:
    void commitData(QWidget *editor);
    void updateCellSpan(App::CellAddress address);
    void insertRows();
    void insertRowsAfter();
    void removeRows();
    void insertColumns();
    void insertColumnsAfter();
    void removeColumns();
    void cellProperties();
    void onRecompute();
    void onBind();
    void onConfSetup();

protected:
    bool edit(const QModelIndex &index, EditTrigger trigger, QEvent *event);
    bool event(QEvent *event);
    void closeEditor(QWidget *editor, QAbstractItemDelegate::EndEditHint hint);
    void mousePressEvent(QMouseEvent* event);
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

    void contextMenuEvent (QContextMenuEvent * e);

    void _copySelection(const std::vector<App::Range> &ranges, bool copy);

    QModelIndex currentEditIndex;
    Spreadsheet::Sheet * sheet;
    int tabCounter;

    QMenu *contextMenu;

    QAction *actionMerge;
    QAction *actionSplit;
    QAction *actionCopy;
    QAction *actionPaste;
    QAction *actionCut;
    QAction *actionDel;
    QAction *actionBind;

    boost::signals2::scoped_connection cellSpanChangedConnection;
};

}

#endif // SHEETTABLEVIEW_H
