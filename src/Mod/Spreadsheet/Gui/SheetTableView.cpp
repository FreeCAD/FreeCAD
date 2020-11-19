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

SheetTableView::SheetTableView(QWidget *parent)
    : QTableView(parent)
    , sheet(0)
{
    QAction * insertRows = new QAction(tr("Insert rows"), this);
    QAction * removeRows = new QAction(tr("Remove rows"), this);
    QAction * hideRows = new QAction(tr("Toggle rows"), this);
    actionShowRows = new QAction(tr("Show all rows"), this);
    actionShowRows->setCheckable(true);
    QAction * insertColumns = new QAction(tr("Insert columns"), this);
    QAction * removeColumns = new QAction(tr("Remove columns"), this);
    QAction * hideColumns = new QAction(tr("Toggle columns"), this);
    actionShowColumns = new QAction(tr("Show all columns"), this);
    actionShowColumns->setCheckable(true);

    setHorizontalHeader(new SheetViewHeader(this,Qt::Horizontal));
    setVerticalHeader(new SheetViewHeader(this,Qt::Vertical));
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    horizontalHeader()->addAction(insertColumns);
    horizontalHeader()->addAction(removeColumns);
    horizontalHeader()->addAction(hideColumns);
    horizontalHeader()->addAction(actionShowColumns);
    horizontalHeader()->setContextMenuPolicy(Qt::ActionsContextMenu);

    verticalHeader()->addAction(insertRows);
    verticalHeader()->addAction(removeRows);
    verticalHeader()->addAction(hideRows);
    verticalHeader()->addAction(actionShowRows);
    verticalHeader()->setContextMenuPolicy(Qt::ActionsContextMenu);

    connect(insertRows, SIGNAL(triggered()), this, SLOT(insertRows()));
    connect(insertColumns, SIGNAL(triggered()), this, SLOT(insertColumns()));
    connect(hideRows, SIGNAL(triggered()), this, SLOT(hideRows()));
    connect(actionShowRows, SIGNAL(toggled(bool)), this, SLOT(showRows()));
    connect(removeRows, SIGNAL(triggered()), this, SLOT(removeRows()));
    connect(removeColumns, SIGNAL(triggered()), this, SLOT(removeColumns()));
    connect(hideColumns, SIGNAL(triggered()), this, SLOT(hideColumns()));
    connect(actionShowColumns, SIGNAL(toggled(bool)), this, SLOT(showColumns()));

    contextMenu = new QMenu(this);

    QAction * cellProperties = new QAction(tr("Properties..."), this);
    contextMenu->addAction(cellProperties);
    connect(cellProperties, SIGNAL(triggered()), this, SLOT(cellProperties()));

    actionAlias = new QAction(tr("Alias..."), this);
    contextMenu->addAction(actionAlias);
    connect(actionAlias, SIGNAL(triggered()), this, SLOT(cellAlias()));

    QActionGroup *editGroup = new QActionGroup(this);
    editGroup->setExclusive(true);

#define SHEET_CELL_MODE(_name, _doc) \
    actionEdit##_name = new QAction(tr(#_name), this);\
    actionEdit##_name->setCheckable(true);\
    actionEdit##_name->setData(QVariant((int)Cell::Edit##_name));\
    actionEdit##_name->setToolTip(tr(_doc));\
    editGroup->addAction(actionEdit##_name);

    SHEET_CELL_MODES
#undef SHEET_CELL_MODE

    QMenu *subMenu = new QMenu(tr("Edit mode"),contextMenu);
#if QT_VERSION >= 0x050100
    subMenu->setToolTipsVisible(true);
#else
    subMenu->installEventFilter(this);
#endif
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
                                 "may affect cells depedency coherence. Use with care!"));

    contextMenu->addSeparator();

    actionBind = new QAction(tr("Bind..."),this);
    connect(actionBind, SIGNAL(triggered()), this, SLOT(onBind()));
    contextMenu->addAction(actionBind);

    QAction *actionConf = new QAction(tr("Configuration table..."),this);
    connect(actionConf, SIGNAL(triggered()), this, SLOT(onConfSetup()));
    contextMenu->addAction(actionConf);

    horizontalHeader()->addAction(actionBind);
    verticalHeader()->addAction(actionBind);

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

    Gui::Command::openCommand("Cell edit mode");
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
        Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
    }catch(Base::Exception &e) {
        e.ReportException();
        Gui::Command::abortCommand();
    }
    Gui::Command::commitCommand();
}

void SheetTableView::onEditPersistent(bool checked) {
    Gui::Command::openCommand("Cell persistent edit");
    try {
        for(auto &index : selectionModel()->selectedIndexes()) {
            CellAddress addr(index.row(), index.column());
            auto cell = sheet->getCell(addr);
            if(cell) {
                Gui::cmdAppObject(sheet, std::ostringstream() << "setPersistentEdit('"
                        << addr.toString() << "', " << (checked?"True":"False") << ")");
            }
        }
        Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
    }catch(Base::Exception &e) {
        e.ReportException();
        Gui::Command::abortCommand();
    }
    Gui::Command::commitCommand();
}

void SheetTableView::onRecompute() {
    Gui::Command::openCommand("Recompute cells");
    for(auto &range : selectedRanges()) {
        Gui::cmdAppObjectArgs(sheet, "touchCells('%s', '%s')",
                range.fromCellString(), range.toCellString());
    }
    Gui::cmdAppObjectArgs(sheet, "recompute(True)");
    Gui::Command::commitCommand();
}

void SheetTableView::onRecomputeNoTouch() {
    Gui::Command::openCommand("Recompute cells only");
    for(auto &range : selectedRanges()) {
        Gui::cmdAppObjectArgs(sheet, "recomputeCells('%s', '%s')",
                range.fromCellString(), range.toCellString());
    }
    Gui::Command::commitCommand();
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

    /* Insert rows */
    Gui::Command::openCommand("Insert rows");
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
    Gui::Command::commitCommand();
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");

    if(updateHidden)
        actionShowRows->setChecked(false);
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

    /* Remove rows */
    Gui::Command::openCommand("Remove rows");
    for (std::vector<int>::const_iterator it = sortedRows.begin(); it != sortedRows.end(); ++it) {
        Gui::cmdAppObjectArgs(sheet, "removeRows('%s', %d)", rowName(*it).c_str(), 1);
    }
    Gui::Command::commitCommand();
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");

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

    /* Insert columns */
    Gui::Command::openCommand("Insert columns");
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
    Gui::Command::commitCommand();
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");

    if(updateHidden)
        actionShowColumns->setChecked(false);
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

    /* Remove columns */
    Gui::Command::openCommand("Remove rows");
    for (std::vector<int>::const_iterator it = sortedColumns.begin(); it != sortedColumns.end(); ++it)
        Gui::cmdAppObjectArgs(sheet, "removeColumns('%s', %d)",
                                     columnName(*it).c_str(), 1);
    Gui::Command::commitCommand();
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");

    if(updateHidden)
        actionShowColumns->setChecked(false);
}

void SheetTableView::showColumns()
{
    updateHiddenColumns();
}

void SheetTableView::hideColumns() {
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

void SheetTableView::hideRows() {
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

void SheetTableView::setSheet(Sheet * _sheet)
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

void SheetTableView::commitData ( QWidget * editor )
{
    QTableView::commitData(editor);
}

bool SheetTableView::edit ( const QModelIndex & index, EditTrigger trigger, QEvent * event )
{
    if (trigger & (QAbstractItemView::DoubleClicked | QAbstractItemView::AnyKeyPressed | QAbstractItemView::EditKeyPressed) )
        currentEditIndex = index;
    return QTableView::edit(index, trigger, event);
}

bool SheetTableView::event(QEvent *event)
{
    /* Catch key presses for navigating the table; Enter/Return (+Shift), and Tab (+Shift) */
    if (event && event->type() == QEvent::KeyPress) {
        QKeyEvent * kevent = static_cast<QKeyEvent*>(event);

        if (kevent->key() == Qt::Key_Tab) {
            QModelIndex c = currentIndex();

            if (kevent->modifiers() == 0) {
                setCurrentIndex(model()->index(c.row(), qMin(c.column() + 1, model()->columnCount() -1)));
                return true;
            }
        }
        else if (kevent->key() == Qt::Key_Backtab) {
            QModelIndex c = currentIndex();

            if (kevent->modifiers() == Qt::ShiftModifier) {
                setCurrentIndex(model()->index(c.row(), qMax(c.column() - 1, 0)));
                return true;
            }
        }
        else if (kevent->key() == Qt::Key_Enter || kevent->key() == Qt::Key_Return) {
            QModelIndex c = currentIndex();

            if (kevent->modifiers() == 0) {
                setCurrentIndex(model()->index(qMin(c.row() + 1, model()->rowCount() - 1), c.column()));
                return true;
            }
            else if (kevent->modifiers() == Qt::ShiftModifier) {
                setCurrentIndex(model()->index(qMax(c.row() - 1, 0), c.column()));
                return true;
            }
        }
        else if (kevent->key() == Qt::Key_Delete) {
            deleteSelection();
            return true;
        }
        else if (kevent->key() == Qt::Key_Escape) {
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
                case Qt::Key_Return:
                case Qt::Key_Enter:
                case Qt::Key_Delete:
                case Qt::Key_Home:
                case Qt::Key_End:
                case Qt::Key_Backspace:
                case Qt::Key_Left:
                case Qt::Key_Right:
                case Qt::Key_Up:
                case Qt::Key_Down:
                case Qt::Key_Tab:
                    kevent->accept();
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
        Gui::Command::openCommand("Clear cell(s)");
        std::vector<Range> ranges = selectedRanges();
        std::vector<Range>::const_iterator i = ranges.begin();

        for (; i != ranges.end(); ++i) {
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.clear('%s')", sheet->getNameInDocument(),
                                    i->rangeString().c_str());
        }
        Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
        Gui::Command::commitCommand();
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

void SheetTableView::mergeCells() {
    Gui::Application::Instance->commandManager().runCommandByName("Spreadsheet_MergeCells");
}

void SheetTableView::splitCell() {
    Gui::Application::Instance->commandManager().runCommandByName("Spreadsheet_SplitCell");
}

void SheetTableView::closeEditor(QWidget * editor, QAbstractItemDelegate::EndEditHint hint)
{
    SpreadsheetGui::TextEdit * le = qobject_cast<SpreadsheetGui::TextEdit*>(editor);
    if(le) {
        currentEditIndex = QModelIndex();
        QTableView::closeEditor(editor, hint);
        setCurrentIndex(le->next());
        return;
    }
    QPushButton *button = qobject_cast<QPushButton*>(editor);
    if(!button)
        QTableView::closeEditor(editor, hint);
}

void SheetTableView::edit ( const QModelIndex & index )
{
    currentEditIndex = index;
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
#define SHEET_CELL_MODE(_name, _doc) \
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

bool SheetTableView::eventFilter(QObject *o, QEvent *ev) {
    (void)o;
    switch (ev->type()) {
#if QT_VERSION < 0x050100
    case QEvent::ToolTip: {
        auto menu = qobject_cast<QMenu*>(o);
        if(!menu)
            break;
        QHelpEvent* he = static_cast<QHelpEvent*>(ev);
        QAction* act = menu->actionAt(he->pos());
        if (act) {
            QString tooltip = act->toolTip();
            if (tooltip.size()) {
                Gui::ToolTip::showText(he->globalPos(), act->toolTip(), menu);
                return false;
            }
        }
        Gui::ToolTip::hideText();
        break;
    }
#endif
    default:
        break;
    }
    return false;
}

void SheetTableView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight
#if QT_VERSION >= 0x050000
        , const QVector<int> &roles
#endif
        )
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

#if QT_VERSION >= 0x050000
    QTableView::dataChanged(topLeft,bottomRight,roles);
#else
    QTableView::dataChanged(topLeft,bottomRight);
#endif
}

#include "moc_SheetTableView.cpp"
