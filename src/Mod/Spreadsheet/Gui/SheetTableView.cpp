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

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QKeyEvent>
# include <QAction>
# include <QPushButton>
# include <QMenu>
# include <QApplication>
# include <QClipboard>
# include <QMenu>
# include <QMessageBox>
# include <QMimeData>
# include <QToolTip>
#endif

#include <Base/Tools.h>
#include <App/Application.h>
#include <App/AutoTransaction.h>
#include <App/Document.h>
#include <Gui/CommandT.h>
#include <Gui/Application.h>
#include <Gui/MainWindow.h>
#include <Gui/Widgets.h>
#include <boost_bind_bind.hpp>
#include "../App/Utils.h"
#include "../App/Cell.h"
#include <App/Range.h>
#include "SpreadsheetDelegate.h"
#include "SheetTableView.h"
#include "SheetModel.h"
#include "LineEdit.h"
#include "PropertiesDialog.h"
#include "DlgBindSheet.h"
#include "DlgSheetConf.h"

using namespace SpreadsheetGui;
using namespace Spreadsheet;
using namespace App;
namespace bp = boost::placeholders;

void SheetViewHeader::mouseReleaseEvent(QMouseEvent *event)
{
    QHeaderView::mouseReleaseEvent(event);
    resizeFinished();
}

bool SheetViewHeader::viewportEvent(QEvent *e) {
    if(e->type() == QEvent::ContextMenu) {
        auto *ce = static_cast<QContextMenuEvent*>(e);
        int section = logicalIndexAt(ce->pos());
        if(section>=0) {
            if(orientation() == Qt::Horizontal) {
                if(!owner->selectionModel()->isColumnSelected(section,owner->rootIndex())) {
                    owner->clearSelection();
                    owner->selectColumn(section);
                }
            }else if(!owner->selectionModel()->isRowSelected(section,owner->rootIndex())) {
                owner->clearSelection();
                owner->selectRow(section);
            }
        }
    }
    return QHeaderView::viewportEvent(e);
}

static std::pair<int, int> selectedMinMaxRows(QModelIndexList list)
{
    int min = std::numeric_limits<int>::max();
    int max = 0;
    for (const auto & item : list) {
        int row = item.row();
        min = std::min(row, min);
        max = std::max(row, max);
    }
    return {min, max};
}

static std::pair<int, int> selectedMinMaxColumns(QModelIndexList list)
{
    int min = std::numeric_limits<int>::max();
    int max = 0;
    for (const auto & item : list) {
        int column = item.column();
        min = std::min(column, min);
        max = std::max(column, max);
    }
    return {min, max};
}

SheetTableView::SheetTableView(QWidget *parent)
    : QTableView(parent)
    , sheet(0)
    , tabCounter(0)
{

    actionShowRows = new QAction(tr("Show all rows"), this);
    actionShowRows->setCheckable(true);
    connect(actionShowRows, SIGNAL(toggled(bool)), this, SLOT(showRows()));
    actionShowColumns = new QAction(tr("Show all columns"), this);
    actionShowColumns->setCheckable(true);
    connect(actionShowColumns, SIGNAL(toggled(bool)), this, SLOT(showColumns()));
    setHorizontalHeader(new SheetViewHeader(this,Qt::Horizontal));
    setVerticalHeader(new SheetViewHeader(this,Qt::Vertical));
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    verticalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);

    actionBind = new QAction(tr("Bind..."),this);
    connect(actionBind, SIGNAL(triggered()), this, SLOT(onBind()));

    connect(verticalHeader(), &QWidget::customContextMenuRequested,
       [this](const QPoint &point){
            QMenu menu(this);
            const auto selection = selectionModel()->selectedRows();
            const auto & [min, max] = selectedMinMaxRows(selection);
            if (bool isContiguous = max - min == selection.size() - 1) {
                Q_UNUSED(isContiguous)
                /*: This is shown in the context menu for the vertical header in a spreadsheet.
                    The number refers to how many lines are selected and will be inserted. */
                auto insertBefore = menu.addAction(tr("Insert %n row(s) above", "", selection.size()));
                connect(insertBefore, SIGNAL(triggered()), this, SLOT(insertRows()));
    
                if (max < model()->rowCount() - 1) {
                    auto insertAfter = menu.addAction(tr("Insert %n row(s) below", "", selection.size()));
                    connect(insertAfter, SIGNAL(triggered()), this, SLOT(insertRowsAfter()));
                }
            } else {
                auto insert = menu.addAction(tr("Insert %n non-contiguous rows", "", selection.size()));
                connect(insert, SIGNAL(triggered()), this, SLOT(insertRows()));
            }
            auto remove = menu.addAction(tr("Remove row(s)", "", selection.size()));
            connect(remove, SIGNAL(triggered()), this, SLOT(removeRows()));

            menu.addSeparator();

            menu.addAction(tr("Toggle row visibility"), [this](bool) {toggleRows();});
            menu.addAction(actionShowRows);

            menu.addSeparator();

            menu.addAction(actionBind);

            menu.exec(verticalHeader()->mapToGlobal(point));
       });

    connect(horizontalHeader(), &QWidget::customContextMenuRequested,
       [this](const QPoint &point){
            QMenu menu(this);
            const auto selection = selectionModel()->selectedColumns();
            const auto & [min, max] = selectedMinMaxColumns(selection);
            if (bool isContiguous = max - min == selection.size() - 1) {
                Q_UNUSED(isContiguous)
                /*: This is shown in the context menu for the horizontal header in a spreadsheet.
                    The number refers to how many lines are selected and will be inserted. */
                auto insertAbove = menu.addAction(tr("Insert %n column(s) left", "", selection.size()));
                connect(insertAbove, SIGNAL(triggered()), this, SLOT(insertColumns()));

                if (max < model()->columnCount() - 1) {
                    auto insertAfter = menu.addAction(tr("Insert %n column(s) right", "", selection.size()));
                    connect(insertAfter, SIGNAL(triggered()), this, SLOT(insertColumnsAfter()));
                }
            } else {
                auto insert = menu.addAction(tr("Insert %n non-contiguous columns", "", selection.size()));
                connect(insert, SIGNAL(triggered()), this, SLOT(insertColumns()));
            }
            auto remove = menu.addAction(tr("Remove column(s)", "", selection.size()));
            connect(remove, SIGNAL(triggered()), this, SLOT(removeColumns()));

            menu.addSeparator();

            menu.addAction(tr("Toggle column visibility"), [this](bool) {toggleColumns();});
            menu.addAction(actionShowColumns);

            menu.addSeparator();

            menu.addAction(actionBind);

            menu.exec(horizontalHeader()->mapToGlobal(point));
       });
       
    contextMenu = new QMenu(this);

    QAction * cellProperties = new QAction(tr("Properties..."), this);
    contextMenu->addAction(cellProperties);
    connect(cellProperties, SIGNAL(triggered()), this, SLOT(cellProperties()));

    actionAlias = new QAction(tr("Alias..."), this);
    contextMenu->addAction(actionAlias);
    connect(actionAlias, SIGNAL(triggered()), this, SLOT(cellAlias()));

    actionRemoveAlias = new QAction(tr("Remove alias(es)"), this);
    contextMenu->addAction(actionRemoveAlias);
    connect(actionRemoveAlias, SIGNAL(triggered()), this, SLOT(removeAlias()));

    QActionGroup *editGroup = new QActionGroup(this);
    editGroup->setExclusive(true);

#define SHEET_CELL_MODE(_name, _label, _doc) \
    actionEdit##_name = new QAction(_label, this);\
    actionEdit##_name->setCheckable(true);\
    actionEdit##_name->setData(QVariant((int)Cell::Edit##_name));\
    actionEdit##_name->setToolTip(tr(_doc));\
    editGroup->addAction(actionEdit##_name);

    SHEET_CELL_MODES
#undef SHEET_CELL_MODE

    QMenu *subMenu = new QMenu(tr("Edit mode"),contextMenu);
    subMenu->setToolTipsVisible(true);

    contextMenu->addMenu(subMenu);
    subMenu->addActions(editGroup->actions());
    connect(editGroup, SIGNAL(triggered(QAction*)), this, SLOT(editMode(QAction*)));

    actionEditPersistent = new QAction(tr("Persistent"),this);
    actionEditPersistent->setCheckable(true);
    connect(actionEditPersistent, SIGNAL(toggled(bool)), this, SLOT(onEditPersistent(bool)));
    subMenu->addSeparator();
    subMenu->addAction(actionEditPersistent);

    contextMenu->addSeparator();
    QAction *recompute = new QAction(tr("Recompute cells"),this);
    connect(recompute, SIGNAL(triggered()), this, SLOT(onRecompute()));
    contextMenu->addAction(recompute);
    recompute->setToolTip(tr("Mark selected cells as touched, and recompute the entire spreadsheet"));

    QAction *recomputeOnly = new QAction(tr("Recompute cells only"),this);
    connect(recomputeOnly, SIGNAL(triggered()), this, SLOT(onRecomputeNoTouch()));
    contextMenu->addAction(recomputeOnly);
    recomputeOnly->setToolTip(tr("Recompute only the selected cells without touching other depending cells\n"
                                 "It can be used as a way out of tricky cyclic dependency problem, but may\n"
                                 "may affect cells dependency coherence. Use with care!"));

    contextMenu->addSeparator();

    contextMenu->addAction(actionBind);

    QAction *actionConf = new QAction(tr("Configuration table..."),this);
    connect(actionConf, SIGNAL(triggered()), this, SLOT(onConfSetup()));
    contextMenu->addAction(actionConf);

    contextMenu->addSeparator();
    actionMerge = contextMenu->addAction(tr("Merge cells"));
    connect(actionMerge,SIGNAL(triggered()), this, SLOT(mergeCells()));
    actionSplit = contextMenu->addAction(tr("Split cells"));
    connect(actionSplit,SIGNAL(triggered()), this, SLOT(splitCell()));
    contextMenu->addSeparator();
    actionCut = contextMenu->addAction(tr("Cut"));
    connect(actionCut,SIGNAL(triggered()), this, SLOT(cutSelection()));
    actionDel = contextMenu->addAction(tr("Delete"));
    connect(actionDel,SIGNAL(triggered()), this, SLOT(deleteSelection()));
    actionCopy = contextMenu->addAction(tr("Copy"));
    connect(actionCopy,SIGNAL(triggered()), this, SLOT(copySelection()));
    actionPaste = contextMenu->addAction(tr("Paste"));
    connect(actionPaste,SIGNAL(triggered()), this, SLOT(pasteClipboard()));

    pasteMenu = new QMenu(tr("Paste special..."));
    contextMenu->addMenu(pasteMenu);
    actionPasteValue = pasteMenu->addAction(tr("Paste value"));
    connect(actionPasteValue,SIGNAL(triggered()), this, SLOT(pasteValue()));
    actionPasteFormat = pasteMenu->addAction(tr("Paste format"));
    connect(actionPasteFormat,SIGNAL(triggered()), this, SLOT(pasteFormat()));
    actionPasteValueFormat = pasteMenu->addAction(tr("Paste value && format"));
    connect(actionPasteValueFormat,SIGNAL(triggered()), this, SLOT(pasteValueFormat()));
    actionPasteFormula = pasteMenu->addAction(tr("Paste formula"));
    connect(actionPasteFormula,SIGNAL(triggered()), this, SLOT(pasteFormula()));

    setTabKeyNavigation(false);
}

void SheetTableView::updateHiddenRows() {
    bool showAll = actionShowRows->isChecked();
    for(auto i : sheet->hiddenRows.getValues()) {
        if(!hiddenRows.erase(i))
            verticalHeader()->headerDataChanged(Qt::Vertical,i,i);
        setRowHidden(i, !showAll);
    }
    for(auto i : hiddenRows) {
        verticalHeader()->headerDataChanged(Qt::Vertical,i,i);
        setRowHidden(i,false);
    }
    hiddenRows = sheet->hiddenRows.getValues();
}

void SheetTableView::removeAlias()
{
    App::AutoTransaction committer(QT_TRANSLATE_NOOP("Command", "Remove cell alias"));
    try {
        for(auto &index : selectionModel()->selectedIndexes()) {
            CellAddress addr(index.row(), index.column());
            auto cell = sheet->getCell(addr);
            if(cell)
                Gui::cmdAppObjectArgs(sheet, "setAlias('%s', None)", addr.toString());
        }
        Gui::Command::updateActive();
    }catch(Base::Exception &e) {
        e.ReportException();
        QMessageBox::critical(Gui::getMainWindow(), QObject::tr("Failed to remove alias"),
                QString::fromLatin1(e.what()));
    }
}

void SheetTableView::updateHiddenColumns() {
    bool showAll = actionShowColumns->isChecked();
    for(auto i : sheet->hiddenColumns.getValues()) {
        if(!hiddenColumns.erase(i))
            horizontalHeader()->headerDataChanged(Qt::Horizontal,i,i);
        setColumnHidden(i, !showAll);
    }
    for(auto i : hiddenColumns) {
        horizontalHeader()->headerDataChanged(Qt::Horizontal,i,i);
        setColumnHidden(i,false);
    }
    hiddenColumns = sheet->hiddenColumns.getValues();
}

void SheetTableView::editMode(QAction *action) {
    int mode = action->data().toInt();

    App::AutoTransaction committer(QT_TRANSLATE_NOOP("Command", "Cell edit mode"));
    try {
        for(auto &index : selectionModel()->selectedIndexes()) {
            CellAddress addr(index.row(), index.column());
            auto cell = sheet->getCell(addr);
            if(cell) {
                Gui::cmdAppObject(sheet, std::ostringstream() << "setEditMode('"
                        << addr.toString() << "', '"
                        << Cell::editModeName((Cell::EditMode)mode) << "')");
            }
        }
        Gui::Command::updateActive();
    }catch(Base::Exception &e) {
        e.ReportException();
        QMessageBox::critical(Gui::getMainWindow(), QObject::tr("Failed to set edit mode"),
                QString::fromLatin1(e.what()));
    }
}

void SheetTableView::onEditPersistent(bool checked) {
    App::AutoTransaction committer(QT_TRANSLATE_NOOP("Command", "Cell persistent edit"));
    try {
        for(auto &index : selectionModel()->selectedIndexes()) {
            CellAddress addr(index.row(), index.column());
            auto cell = sheet->getCell(addr);
            if(cell) {
                Gui::cmdAppObject(sheet, std::ostringstream() << "setPersistentEdit('"
                        << addr.toString() << "', " << (checked?"True":"False") << ")");
            }
        }
        Gui::Command::updateActive();
    }catch(Base::Exception &e) {
        e.ReportException();
        QMessageBox::critical(Gui::getMainWindow(), QObject::tr("Failed to set edit mode"),
                QString::fromLatin1(e.what()));
    }
}

void SheetTableView::onRecompute() {
    App::AutoTransaction committer(QT_TRANSLATE_NOOP("Command", "Recompute cells"));
    try {
        for(auto &range : selectedRanges()) {
            Gui::cmdAppObjectArgs(sheet, "touchCells('%s', '%s')",
                    range.fromCellString(), range.toCellString());
        }
        Gui::cmdAppObjectArgs(sheet, "recompute(True)");
    } catch (Base::Exception &e) {
        e.ReportException();
        QMessageBox::critical(Gui::getMainWindow(), QObject::tr("Failed to recompute cells"),
                QString::fromLatin1(e.what()));
    }
}

void SheetTableView::onRecomputeNoTouch() {
    App::AutoTransaction committer(QT_TRANSLATE_NOOP("Command", "Recompute cells only"));
    try {
        for(auto &range : selectedRanges()) {
            Gui::cmdAppObjectArgs(sheet, "recomputeCells('%s', '%s')",
                    range.fromCellString(), range.toCellString());
        }
    } catch (Base::Exception &e) {
        e.ReportException();
        QMessageBox::critical(Gui::getMainWindow(), QObject::tr("Failed to recompute cells"),
                QString::fromLatin1(e.what()));
    }
}

void SheetTableView::onBind() {
    auto ranges = selectedRanges();
    if(ranges.size()>=1 && ranges.size()<=2) {
        DlgBindSheet dlg(sheet,ranges,Gui::getMainWindow());
        dlg.exec();
    }
}

void SheetTableView::onConfSetup() {
    auto ranges = selectedRanges();
    if(ranges.empty())
        return;
    DlgSheetConf dlg(sheet,ranges.back(),Gui::getMainWindow());
    dlg.exec();
}

void SheetTableView::cellProperties()
{
    PropertiesDialog dialog(sheet, selectedRanges(), this);

    if (dialog.exec() == QDialog::Accepted) {
        dialog.apply();
    }
}

void SheetTableView::cellAlias()
{
    auto ranges = selectedRanges();
    if(ranges.size() != 1
            || ranges[0].rowCount()!=1 
            || ranges[0].colCount()!=1)
        return;

    PropertiesDialog dialog(sheet, ranges, this);
    dialog.selectAlias();
    if (dialog.exec() == QDialog::Accepted)
        dialog.apply();
}

std::vector<Range> SheetTableView::selectedRanges() const
{
    QModelIndexList list = selectionModel()->selectedIndexes();
    std::vector<Range> result;

    // Insert selected cells into set. This variable should ideally be a hash_set
    // but that is not part of standard stl.
    std::set<std::pair<int, int> > cells;
    for (QModelIndexList::const_iterator it = list.begin(); it != list.end(); ++it)
        cells.insert(std::make_pair<int,int>((*it).row(), (*it).column()));

    // Create rectangular cells from the unordered collection of selected cells
    std::map<std::pair<int, int>, std::pair<int, int> > rectangles;
    createRectangles(cells, rectangles);

    std::map<std::pair<int, int>, std::pair<int, int> >::const_iterator i = rectangles.begin();
    for (; i != rectangles.end(); ++i) {
        std::pair<int, int> ul = (*i).first;
        std::pair<int, int> size = (*i).second;

        result.emplace_back(ul.first, ul.second,
                                                   ul.first + size.first - 1, ul.second + size.second - 1);
    }

    return result;
}

void SheetTableView::insertRows()
{
    assert(sheet != 0);

    QModelIndexList rows = selectionModel()->selectedRows();
    std::vector<int> sortedRows;

    bool updateHidden = false;
    if(hiddenRows.size() && !actionShowRows->isChecked()) {
        updateHidden = true;
        actionShowRows->setChecked(true);
        // To make sure the hidden rows are actually shown. Any better idea?
        qApp->sendPostedEvents();
    }

    /* Make sure rows are sorted in ascending order */
    for (QModelIndexList::const_iterator it = rows.begin(); it != rows.end(); ++it)
        sortedRows.push_back(it->row());
    std::sort(sortedRows.begin(), sortedRows.end());

    App::AutoTransaction committer(QT_TRANSLATE_NOOP("Command", "Insert rows"));
    try {
        /* Insert rows */
        std::vector<int>::const_reverse_iterator it = sortedRows.rbegin();
        while (it != sortedRows.rend()) {
            int prev = *it;
            int count = 1;

            /* Collect neighbouring rows into one chunk */
            ++it;
            while (it != sortedRows.rend()) {
                if (*it == prev - 1) {
                    prev = *it;
                    ++count;
                    ++it;
                }
                else
                    break;
            }

            Gui::cmdAppObjectArgs(sheet, "insertRows('%s', %d)", rowName(prev).c_str(), count);
        }
        Gui::Command::updateActive();
    } catch (Base::Exception &e) {
        e.ReportException();
        QMessageBox::critical(Gui::getMainWindow(), QObject::tr("Failed to insert rows"),
                QString::fromLatin1(e.what()));
    }

    if(updateHidden)
        actionShowRows->setChecked(false);
}

void SheetTableView::insertRowsAfter()
{
    assert(sheet != 0);
    const auto rows = selectionModel()->selectedRows();
    const auto & [min, max] = selectedMinMaxRows(rows);
    assert(max - min == rows.size() - 1);
    Q_UNUSED(min)

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Insert rows"));
    Gui::cmdAppObjectArgs(sheet, "insertRows('%s', %d)", rowName(max + 1).c_str(), rows.size());
    Gui::Command::commitCommand();
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
}

void SheetTableView::removeRows()
{
    assert(sheet != 0);

    bool updateHidden = false;
    if(hiddenRows.size() && !actionShowRows->isChecked()) {
        updateHidden = true;
        actionShowRows->setChecked(true);
        // To make sure the hidden rows are actually shown. Any better idea?
        qApp->sendPostedEvents();
    }

    QModelIndexList rows = selectionModel()->selectedRows();
    std::vector<int> sortedRows;

    /* Make sure rows are sorted in descending order */
    for (QModelIndexList::const_iterator it = rows.begin(); it != rows.end(); ++it)
        sortedRows.push_back(it->row());
    std::sort(sortedRows.begin(), sortedRows.end(), std::greater<int>());

    App::AutoTransaction committer(QT_TRANSLATE_NOOP("Command", "Remove rows"));
    try {
        /* Remove rows */
        for (std::vector<int>::const_iterator it = sortedRows.begin(); it != sortedRows.end(); ++it) {
            Gui::cmdAppObjectArgs(sheet, "removeRows('%s', %d)", rowName(*it).c_str(), 1);
        }
        Gui::Command::updateActive();
    } catch (Base::Exception &e) {
        e.ReportException();
        QMessageBox::critical(Gui::getMainWindow(), QObject::tr("Failed to remove rows"),
                QString::fromLatin1(e.what()));
    }

    if(updateHidden)
        actionShowRows->setChecked(false);
}

void SheetTableView::insertColumns()
{
    assert(sheet != 0);

    QModelIndexList cols = selectionModel()->selectedColumns();
    std::vector<int> sortedColumns;

    bool updateHidden = false;
    if(hiddenColumns.size() && !actionShowColumns->isChecked()) {
        updateHidden = true;
        actionShowColumns->setChecked(true);
        qApp->sendPostedEvents();
    }

    /* Make sure rows are sorted in ascending order */
    for (QModelIndexList::const_iterator it = cols.begin(); it != cols.end(); ++it)
        sortedColumns.push_back(it->column());
    std::sort(sortedColumns.begin(), sortedColumns.end());

    App::AutoTransaction committer(QT_TRANSLATE_NOOP("Command", "Insert columns"));
    try {
        /* Insert columns */
        std::vector<int>::const_reverse_iterator it = sortedColumns.rbegin();
        while (it != sortedColumns.rend()) {
            int prev = *it;
            int count = 1;

            /* Collect neighbouring columns into one chunk */
            ++it;
            while (it != sortedColumns.rend()) {
                if (*it == prev - 1) {
                    prev = *it;
                    ++count;
                    ++it;
                }
                else
                    break;
            }

            Gui::cmdAppObjectArgs(sheet, "insertColumns('%s', %d)",
                                        columnName(prev).c_str(), count);
        }
        Gui::Command::updateActive();
    } catch (Base::Exception &e) {
        e.ReportException();
        QMessageBox::critical(Gui::getMainWindow(), QObject::tr("Failed to insert columns"),
                QString::fromLatin1(e.what()));
    }

    if(updateHidden)
        actionShowColumns->setChecked(false);
}

void SheetTableView::insertColumnsAfter()
{
    assert(sheet != 0);
    const auto columns = selectionModel()->selectedColumns();
    const auto& [min, max] = selectedMinMaxColumns(columns);
    assert(max - min == columns.size() - 1);
    Q_UNUSED(min)

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Insert columns"));
    Gui::cmdAppObjectArgs(sheet, "insertColumns('%s', %d)", columnName(max + 1).c_str(), columns.size());
    Gui::Command::commitCommand();
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
}

void SheetTableView::removeColumns()
{
    assert(sheet != 0);

    QModelIndexList cols = selectionModel()->selectedColumns();
    std::vector<int> sortedColumns;

    bool updateHidden = false;
    if(hiddenColumns.size() && !actionShowColumns->isChecked()) {
        updateHidden = true;
        actionShowColumns->setChecked(true);
        qApp->sendPostedEvents();
    }

    /* Make sure rows are sorted in descending order */
    for (QModelIndexList::const_iterator it = cols.begin(); it != cols.end(); ++it)
        sortedColumns.push_back(it->column());
    std::sort(sortedColumns.begin(), sortedColumns.end(), std::greater<int>());

    App::AutoTransaction committer(QT_TRANSLATE_NOOP("Command", "Remove columns"));
    try {
        /* Remove columns */
        for (std::vector<int>::const_iterator it = sortedColumns.begin(); it != sortedColumns.end(); ++it)
            Gui::cmdAppObjectArgs(sheet, "removeColumns('%s', %d)",
                                        columnName(*it).c_str(), 1);
        Gui::Command::updateActive();
    } catch (Base::Exception &e) {
        e.ReportException();
        QMessageBox::critical(Gui::getMainWindow(), QObject::tr("Failed to remove columns"),
                QString::fromLatin1(e.what()));
    }

    if(updateHidden)
        actionShowColumns->setChecked(false);
}

void SheetTableView::showColumns()
{
    updateHiddenColumns();
}

void SheetTableView::toggleColumns() {
    auto hidden = sheet->hiddenColumns.getValues();
    for(auto &idx : selectionModel()->selectedColumns()) {
        auto res = hidden.insert(idx.column());
        if(!res.second)
            hidden.erase(res.first);
    }
    sheet->hiddenColumns.setValues(hidden);
}

void SheetTableView::showRows()
{
    updateHiddenRows();
}

void SheetTableView::toggleRows() {
    auto hidden = sheet->hiddenRows.getValues();
    for(auto &idx : selectionModel()->selectedRows()) {
        auto res = hidden.insert(idx.row());
        if(!res.second)
            hidden.erase(res.first);
    }
    sheet->hiddenRows.setValues(hidden);
}

SheetTableView::~SheetTableView()
{

}

void SheetTableView::updateCellSpan(CellAddress address)
{
    int rows, cols;

    sheet->getSpans(address, rows, cols);

    if (rows != rowSpan(address.row(), address.col()) || cols != columnSpan(address.row(), address.col()))
        setSpan(address.row(), address.col(), rows, cols);
}

void SheetTableView::setSheet(Sheet* _sheet)
{
    sheet = _sheet;
    cellSpanChangedConnection = sheet->cellSpanChanged.connect(bind(&SheetTableView::updateCellSpan, this, bp::_1));

    // Update row and column spans
    std::vector<std::string> usedCells = sheet->getUsedCells();

    for (std::vector<std::string>::const_iterator i = usedCells.begin(); i != usedCells.end(); ++i) {
        CellAddress address(*i);
        auto cell = sheet->getCell(address);
        if(cell && !cell->hasException() && cell->isPersistentEditMode())
            openPersistentEditor(model()->index(address.row(),address.col()));

        if (sheet->isMergedCell(address))
            updateCellSpan(address);
    }

    // Update column widths and row height
    std::map<int, int> columWidths = sheet->getColumnWidths();
    for (std::map<int, int>::const_iterator i = columWidths.begin(); i != columWidths.end(); ++i) {
        int newSize = i->second;

        if (newSize > 0 && horizontalHeader()->sectionSize(i->first) != newSize)
            setColumnWidth(i->first, newSize);
    }

    std::map<int, int> rowHeights = sheet->getRowHeights();
    for (std::map<int, int>::const_iterator i = rowHeights.begin(); i != rowHeights.end(); ++i) {
        int newSize = i->second;

        if (newSize > 0 && verticalHeader()->sectionSize(i->first) != newSize)
            setRowHeight(i->first, newSize);
    }

    updateHiddenRows();
    updateHiddenColumns();
}

void SheetTableView::commitData(QWidget* editor)
{
    QTableView::commitData(editor);
}

bool SheetTableView::edit(const QModelIndex& index, EditTrigger trigger, QEvent* event)
{
    auto delegate = qobject_cast<SpreadsheetDelegate*>(itemDelegate());
    if (delegate)
        delegate->setEditTrigger(trigger);
    return QTableView::edit(index, trigger, event);
}

bool SheetTableView::event(QEvent* event)
{
    if (event && event->type() == QEvent::KeyPress && this->hasFocus()) {
        // If this widget has focus, look for keyboard events that represent movement shortcuts
        // and handle them.
        QKeyEvent* kevent = static_cast<QKeyEvent*>(event);
        switch (kevent->key()) {
        case Qt::Key_Return: [[fallthrough]];
        case Qt::Key_Enter: [[fallthrough]];
        case Qt::Key_Home: [[fallthrough]];
        case Qt::Key_End: [[fallthrough]];
        case Qt::Key_Left: [[fallthrough]];
        case Qt::Key_Right: [[fallthrough]];
        case Qt::Key_Up: [[fallthrough]];
        case Qt::Key_Down: [[fallthrough]];
        case Qt::Key_Tab: [[fallthrough]];
        case Qt::Key_Backtab:
            finishEditWithMove(kevent->key(), kevent->modifiers(), true);
            return true;
        // Also handle the delete key here:
        case Qt::Key_Delete:
            deleteSelection();
            return true;
        default:
            break;
        }

        if (kevent->key() == Qt::Key_Escape) {
            sheet->setCopyOrCutRanges({});
            return true;
        }
        else if (kevent->matches(QKeySequence::Cut)) {
            cutSelection();
            return true;
        }
        else if (kevent->matches(QKeySequence::Copy)) {
            copySelection();
            return true;
        }
        else if (kevent->matches(QKeySequence::Paste)) {
            pasteClipboard();
            return true;
        }
    }
    else if (event && event->type() == QEvent::ShortcutOverride) {
        QKeyEvent * kevent = static_cast<QKeyEvent*>(event);
        if (kevent->modifiers() == Qt::NoModifier ||
            kevent->modifiers() == Qt::ShiftModifier ||
            kevent->modifiers() == Qt::KeypadModifier) {
            switch (kevent->key()) {
                case Qt::Key_Return: [[fallthrough]];
                case Qt::Key_Enter: [[fallthrough]];
                case Qt::Key_Delete: [[fallthrough]];
                case Qt::Key_Home: [[fallthrough]];
                case Qt::Key_End: [[fallthrough]];
                case Qt::Key_Backspace: [[fallthrough]];
                case Qt::Key_Left: [[fallthrough]];
                case Qt::Key_Right: [[fallthrough]];
                case Qt::Key_Up: [[fallthrough]];
                case Qt::Key_Down: [[fallthrough]];
                case Qt::Key_Tab:
                    kevent->accept();
                    break;
                default:
                    break;
            }

            if (kevent->key() < Qt::Key_Escape) {
                kevent->accept();
            }
        }

        if (kevent->matches(QKeySequence::Cut)) {
            kevent->accept();
        }
        else if (kevent->matches(QKeySequence::Copy)) {
            kevent->accept();
        }
        else if (kevent->matches(QKeySequence::Paste)) {
            kevent->accept();
        }
    }
    return QTableView::event(event);
}

void SheetTableView::deleteSelection()
{
    QModelIndexList selection = selectionModel()->selectedIndexes();

    if (selection.size() > 0) {
        App::AutoTransaction committer(QT_TRANSLATE_NOOP("Command", "Clear cell(s)"));
        try {
            std::vector<Range> ranges = selectedRanges();
            std::vector<Range>::const_iterator i = ranges.begin();

            for (; i != ranges.end(); ++i) {
                Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.clear('%s')", sheet->getNameInDocument(),
                                        i->rangeString().c_str());
            }
            Gui::Command::updateActive();
        } catch (Base::Exception &e) {
            e.ReportException();
            QMessageBox::critical(Gui::getMainWindow(), QObject::tr("Failed to clear cells"),
                    QString::fromLatin1(e.what()));
        }
    }
}

static const QLatin1String _SheetMime("application/x-fc-spreadsheet");

void SheetTableView::copySelection()
{
    _copySelection(selectedRanges(), true);
}

void SheetTableView::_copySelection(const std::vector<App::Range> &ranges, bool copy)
{
    int minRow = INT_MAX;
    int maxRow = 0;
    int minCol = INT_MAX;
    int maxCol = 0;
    for (auto &range : ranges) {
        minRow = std::min(minRow, range.from().row());
        maxRow = std::max(maxRow, range.to().row());
        minCol = std::min(minCol, range.from().col());
        maxCol = std::max(maxCol, range.to().col());
    }

    QString selectedText;
    for (int i=minRow; i<=maxRow; i++) {
        for (int j=minCol; j<=maxCol; j++) {
            QModelIndex index = model()->index(i,j);
            QString cell = index.data(Qt::EditRole).toString();
            if (j < maxCol)
                cell.append(QChar::fromLatin1('\t'));
            selectedText += cell;
        }
        if (i < maxRow)
            selectedText.append(QChar::fromLatin1('\n'));
    }

    Base::StringWriter writer;
    sheet->getCells()->copyCells(writer,ranges);
    QMimeData *mime = new QMimeData();
    mime->setText(selectedText);
    mime->setData(_SheetMime,QByteArray(writer.getString().c_str()));
    QApplication::clipboard()->setMimeData(mime);

    sheet->setCopyOrCutRanges(std::move(ranges), copy);
}

void SheetTableView::cutSelection()
{
    _copySelection(selectedRanges(), false);
}

void SheetTableView::pasteClipboard()
{
    _pasteClipboard("Paste cell", Cell::PasteAll);
}

void SheetTableView::pasteValue()
{
    _pasteClipboard("Paste cell value", Cell::PasteValue);
}

void SheetTableView::pasteFormat()
{
    _pasteClipboard("Paste cell format", Cell::PasteFormat);
}

void SheetTableView::pasteValueFormat()
{
    _pasteClipboard("Paste value format", Cell::PasteValue|Cell::PasteFormat);
}

void SheetTableView::pasteFormula()
{
    _pasteClipboard("Paste cell formula", Cell::PasteFormula);
}

void SheetTableView::_pasteClipboard(const char *name, int type)
{
    App::AutoTransaction committer(name);
    try {
        bool copy = true;
        auto ranges = sheet->getCopyOrCutRange(copy);
        if(ranges.empty()) {
            copy = false;
            ranges = sheet->getCopyOrCutRange(copy);
        }

        if(ranges.size())
            _copySelection(ranges, copy);

        const QMimeData* mimeData = QApplication::clipboard()->mimeData();
        if(!mimeData || !mimeData->hasText())
            return;

        if(!copy) {
            for(auto range : ranges) {
                do {
                    sheet->clear(*range);
                } while (range.next());
            }
        }

        ranges = selectedRanges();
        if(ranges.empty())
            return;

        Range range = ranges.back();
        if (!mimeData->hasFormat(_SheetMime)) {
            CellAddress current = range.from();
            QStringList cells;
            QString text = mimeData->text();
            int i=0;
            for (auto it : text.split(QLatin1Char('\n'))) {
                QStringList cols = it.split(QLatin1Char('\t'));
                int j=0;
                for (auto jt : cols) {
                    QModelIndex index = model()->index(current.row()+i, current.col()+j);
                    model()->setData(index, jt);
                    j++;
                }
                i++;
            }
        }else{
            QByteArray res = mimeData->data(_SheetMime);
            Base::ByteArrayIStreambuf buf(res);
            std::istream in(0);
            in.rdbuf(&buf);
            Base::XMLReader reader("<memory>", in);
            sheet->getCells()->pasteCells(reader,range,(Cell::PasteType)type);
        }

        GetApplication().getActiveDocument()->recompute();

    }catch(Base::Exception &e) {
        e.ReportException();
        QMessageBox::critical(Gui::getMainWindow(), QObject::tr("Copy & Paste failed"),
                QString::fromLatin1(e.what()));
        return;
    }
    clearSelection();
}

void SheetTableView::finishEditWithMove(int keyPressed, Qt::KeyboardModifiers modifiers, bool handleTabMotion)
{
    (void)handleTabMotion;

    // A utility lambda for finding the beginning and ending of data regions
    auto scanForRegionBoundary = [this](int& r, int& c, int dr, int dc) {
        auto startAddress = CellAddress(r, c);
        auto startCell = sheet->getCell(startAddress);
        bool startedAtEmptyCell = startCell ? !startCell->isUsed() : true;
        const int maxRow = this->model()->rowCount() - 1;
        const int maxCol = this->model()->columnCount() - 1;
        while (c + dc >= 0 && r + dr >= 0 && c + dc <= maxCol && r + dr <= maxRow) {
            r += dr;
            c += dc;
            auto cell = sheet->getCell(CellAddress(r, c));
            auto cellIsEmpty = cell ? !cell->isUsed() : true;
            if (cellIsEmpty && !startedAtEmptyCell) {
                // Don't stop at the empty cell, stop at the last non-empty cell
                r -= dr;
                c -= dc;
                break;
            }
            else if (!cellIsEmpty && startedAtEmptyCell) {
                break;
            }
        }
        if (r == startAddress.row() && c == startAddress.col()) {
            // Always move at least one cell:
            r += dr;
            c += dc;
        }
        r = std::max(0, std::min(r, maxRow));
        c = std::max(0, std::min(c, maxCol));
    };

    int targetRow = currentIndex().row();
    int targetColumn = currentIndex().column();
    int colSpan;
    int rowSpan;
    sheet->getSpans(CellAddress(targetRow, targetColumn), rowSpan, colSpan);
    switch (keyPressed) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (modifiers == Qt::NoModifier) {
            targetRow += rowSpan;
            targetColumn -= tabCounter;
        }
        else if (modifiers == Qt::ShiftModifier) {
            targetRow -= 1;
            targetColumn -= tabCounter;
        }
        else {
            // For an unrecognized modifier, just go down
            targetRow += rowSpan;
        }
        tabCounter = 0;
        break;

    case Qt::Key_Home:
        // Home: row 1, same column
        // Ctrl-Home: row 1, column 1 
        targetRow = 0;
        if (modifiers == Qt::ControlModifier)
            targetColumn = 0;
        tabCounter = 0;
        break;

    case Qt::Key_End:
    {
        // End should take you to the last occupied cell in the current column
        // Ctrl-End takes you to the last cell in the sheet
        auto usedCells = sheet->getCells()->getUsedCells();
        for (const auto& cell : usedCells) {
            if (modifiers == Qt::NoModifier) {
                if (cell.col() == targetColumn)
                    targetRow = std::max(targetRow, cell.row());
            }
            else if (modifiers == Qt::ControlModifier) {
                targetRow = std::max(targetRow, cell.row());
                targetColumn = std::max(targetColumn, cell.col());
            }
        }
        tabCounter = 0;
        break;
    }

    case Qt::Key_Left:
        if (targetColumn == 0)
            break; // Nothing to do, we're already in the first column
        if (modifiers == Qt::NoModifier || modifiers == Qt::ShiftModifier)
            targetColumn--;
        else if (modifiers == Qt::ControlModifier ||
                 modifiers == (Qt::ControlModifier | Qt::ShiftModifier))
            scanForRegionBoundary(targetRow, targetColumn, 0, -1);
        else
            targetColumn--; //Unrecognized modifier combination: default to just moving one cell
        tabCounter = 0;
        break;
    case Qt::Key_Right:
        if (targetColumn >= this->model()->columnCount() - 1)
            break; // Nothing to do, we're already in the last column
        if (modifiers == Qt::NoModifier || modifiers == Qt::ShiftModifier)
            targetColumn += colSpan;
        else if (modifiers == Qt::ControlModifier ||
                 modifiers == (Qt::ControlModifier | Qt::ShiftModifier))
            scanForRegionBoundary(targetRow, targetColumn, 0, 1);
        else
            targetColumn += colSpan; //Unrecognized modifier combination: default to just moving one cell
        tabCounter = 0;
        break;
    case Qt::Key_Up:
        if (targetRow == 0)
            break; // Nothing to do, we're already in the first column
        if (modifiers == Qt::NoModifier || modifiers == Qt::ShiftModifier)
            targetRow--;
        else if (modifiers == Qt::ControlModifier ||
                 modifiers == (Qt::ControlModifier | Qt::ShiftModifier))
            scanForRegionBoundary(targetRow, targetColumn, -1, 0);
        else
            targetRow--; //Unrecognized modifier combination: default to just moving one cell
        tabCounter = 0;
        break;
    case Qt::Key_Down:
        if (targetRow >= this->model()->rowCount() - 1)
            break; // Nothing to do, we're already in the last row
        if (modifiers == Qt::NoModifier || modifiers == Qt::ShiftModifier)
            targetRow += rowSpan;
        else if (modifiers == Qt::ControlModifier ||
                 modifiers == (Qt::ControlModifier | Qt::ShiftModifier))
            scanForRegionBoundary(targetRow, targetColumn, 1, 0);
        else
            targetRow += rowSpan; //Unrecognized modifier combination: default to just moving one cell
        tabCounter = 0;
        break;
    case Qt::Key_Tab:
        if (modifiers == Qt::NoModifier) {
            tabCounter++;
            // if (handleTabMotion)
                targetColumn += colSpan;
        }
        else if (modifiers == Qt::ShiftModifier) {
            tabCounter = 0;
            // if (handleTabMotion)
                targetColumn--;
        }
        break;
    case Qt::Key_Backtab:
        modifiers.setFlag(Qt::ShiftModifier, false);
        targetColumn--;
        tabCounter = 0;
        break;
    default:
        break;
    }

    if (this->sheet->isMergedCell(CellAddress(targetRow, targetColumn))) {
        auto anchor = this->sheet->getAnchor(CellAddress(targetRow, targetColumn));
        targetRow = anchor.row();
        targetColumn = anchor.col();
    }

    // Overflow/underflow protection:
    const int maxRow = this->model()->rowCount() - 1;
    const int maxCol = this->model()->columnCount() - 1;
    targetRow = std::max(0, std::min(targetRow, maxRow));
    targetColumn = std::max(0, std::min(targetColumn, maxCol));

    if (!(modifiers & Qt::ShiftModifier) || keyPressed == Qt::Key_Tab || keyPressed == Qt::Key_Enter || keyPressed == Qt::Key_Return) {
        // We have to use this method so that Ctrl-modifier combinations don't result in multiple selection
        this->selectionModel()->setCurrentIndex(model()->index(targetRow, targetColumn),
            QItemSelectionModel::ClearAndSelect);
    }
    else if (modifiers & Qt::ShiftModifier) {
        // With shift down, this motion becomes a block selection command, rather than just simple motion:
        ModifyBlockSelection(targetRow, targetColumn);
    }
        
}

void SheetTableView::ModifyBlockSelection(int targetRow, int targetCol)
{
    int startingRow = currentIndex().row();
    int startingCol = currentIndex().column();

    // Get the current block selection size:
    auto selection = this->selectionModel()->selection();
    for (const auto& range : selection) {
        if (range.contains(currentIndex())) {
            // This range contains the current cell, so it's the one we're going to modify (assuming we're at one of the corners)
            int rangeMinRow = range.top();
            int rangeMaxRow = range.bottom();
            int rangeMinCol = range.left();
            int rangeMaxCol = range.right();
            if ((startingRow == rangeMinRow || startingRow == rangeMaxRow) &&
                (startingCol == rangeMinCol || startingCol == rangeMaxCol)) {
                if (range.contains(model()->index(targetRow, targetCol))) {
                    // If the range already contains the target cell, then we're making the range smaller
                    if (startingRow == rangeMinRow)
                        rangeMinRow = targetRow;
                    if (startingRow == rangeMaxRow)
                        rangeMaxRow = targetRow;
                    if (startingCol == rangeMinCol)
                        rangeMinCol = targetCol;
                    if (startingCol == rangeMaxCol)
                        rangeMaxCol = targetCol;
                }
                else {
                    // We're making the range bigger
                    rangeMinRow = std::min(rangeMinRow, targetRow);
                    rangeMaxRow = std::max(rangeMaxRow, targetRow);
                    rangeMinCol = std::min(rangeMinCol, targetCol);
                    rangeMaxCol = std::max(rangeMaxCol, targetCol);
                }
                QItemSelection oldRange(range.topLeft(), range.bottomRight());
                this->selectionModel()->select(oldRange, QItemSelectionModel::Deselect);
                QItemSelection newRange(model()->index(rangeMinRow, rangeMinCol), model()->index(rangeMaxRow, rangeMaxCol));
                this->selectionModel()->select(newRange, QItemSelectionModel::Select);
            }
            break;
        }
    }

    this->selectionModel()->setCurrentIndex(model()->index(targetRow, targetCol), QItemSelectionModel::Current);
}

void SheetTableView::mergeCells() {
    Gui::Application::Instance->commandManager().runCommandByName("Spreadsheet_MergeCells");
}

void SheetTableView::splitCell() {
    Gui::Application::Instance->commandManager().runCommandByName("Spreadsheet_SplitCell");
}

void SheetTableView::mousePressEvent(QMouseEvent* event)
{
    tabCounter = 0;
    QTableView::mousePressEvent(event);
}

void SheetTableView::closeEditor(QWidget * editor, QAbstractItemDelegate::EndEditHint hint)
{
    if (qobject_cast<SpreadsheetGui::TextEdit*>(editor)
            || !qobject_cast<QPushButton*>(editor))
    {
        QTableView::closeEditor(editor, hint);
    }
}

void SheetTableView::edit ( const QModelIndex & index )
{
    QTableView::edit(index);
}

void SheetTableView::contextMenuEvent(QContextMenuEvent *) {
    QAction *action = 0;
    bool persistent = false;
    auto ranges = selectedRanges();
    for(auto &range : ranges) {
        do {
            auto cell = sheet->getCell(range.address());
            if(!cell) continue;
            persistent = persistent || cell->isPersistentEditMode();
            switch(cell->getEditMode()) {
#define SHEET_CELL_MODE(_name, _label, _doc) \
            case Cell::Edit##_name:\
                action = actionEdit##_name;\
                break;
            SHEET_CELL_MODES
#undef SHEET_CELL_MODE
            default:
                action = actionEditNormal;
                break;
            }
            break;
        } while (range.next());
        if(action)
            break;
    }
    if(!action)
        action = actionEditNormal;
    action->setChecked(true);

    actionEditPersistent->setChecked(persistent);

    const QMimeData* mimeData = QApplication::clipboard()->mimeData();
    if(ranges.empty()) {
        actionCut->setEnabled(false);
        actionCopy->setEnabled(false);
        actionDel->setEnabled(false);
        actionPaste->setEnabled(false);
        actionSplit->setEnabled(false);
        actionMerge->setEnabled(false);
        pasteMenu->setEnabled(false);
    }else{
        bool canPaste = mimeData && (mimeData->hasText() || mimeData->hasFormat(_SheetMime));
        actionPaste->setEnabled(canPaste);
        pasteMenu->setEnabled(canPaste && mimeData->hasFormat(_SheetMime));
        actionCut->setEnabled(true);
        actionCopy->setEnabled(true);
        actionDel->setEnabled(true);
        actionSplit->setEnabled(true);
        actionMerge->setEnabled(true);
    }

    actionBind->setEnabled(ranges.size()>=1 && ranges.size()<=2);

    actionAlias->setEnabled(ranges.size()==1
            && ranges[0].rowCount()==1 && ranges[0].colCount()==1);

    contextMenu->exec(QCursor::pos());
}

void SheetTableView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight , const QVector<int> &roles)
{
    App::Range range(topLeft.row(),topLeft.column(),bottomRight.row(),bottomRight.column());
    auto delegate = qobject_cast<SpreadsheetDelegate*>(itemDelegate());
    bool _dummy = false;
    bool &updating = delegate?delegate->updating:_dummy;
    Base::StateLocker guard(updating);
    do {
        auto address = *range;
        auto cell = sheet->getCell(address);
        closePersistentEditor(model()->index(address.row(),address.col()));
        if(cell && !cell->hasException() && cell->isPersistentEditMode())
            openPersistentEditor(model()->index(address.row(),address.col()));
    }while(range.next());

    QTableView::dataChanged(topLeft,bottomRight,roles);
}

void SheetTableView::setForegroundColor(const QColor &c)
{
    auto m = static_cast<SheetModel*>(model());
    if (m && c != m->foregroundColor()) {
        m->setForegroundColor(c);
        update();
    }
}

QColor SheetTableView::foregroundColor() const
{
    auto m = static_cast<SheetModel*>(model());
    if (m)
        return m->foregroundColor();
    return QColor();
}

void SheetTableView::setAliasForegroundColor(const QColor &c)
{
    auto m = static_cast<SheetModel*>(model());
    if (m && c != m->aliasForegroundColor()) {
        m->setAliasForegroundColor(c);
        update();
    }
}

QColor SheetTableView::aliasForegroundColor() const
{
    auto m = static_cast<SheetModel*>(model());
    if (m)
        return m->aliasForegroundColor();
    return QColor();
}

#include "moc_SheetTableView.cpp"
