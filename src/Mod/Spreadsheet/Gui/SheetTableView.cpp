/***************************************************************************
 *   Copyright (c) 2015 Eivind Kvedalen <eivind@kvedalen.name>             *
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
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#endif
#include <QTextTableCell>

#include <App/Application.h>
#include <App/AutoTransaction.h>
#include <App/Document.h>
#include <App/Range.h>
#include <Base/Reader.h>
#include <Base/Stream.h>
#include <Base/Writer.h>
#include <Gui/Application.h>
#include <Gui/CommandT.h>
#include <Gui/MainWindow.h>
#include <Mod/Spreadsheet/App/Cell.h>

#include "DlgBindSheet.h"
#include "DlgSheetConf.h"
#include "LineEdit.h"
#include "PropertiesDialog.h"
#include "SheetTableView.h"


using namespace SpreadsheetGui;
using namespace Spreadsheet;
using namespace App;
namespace sp = std::placeholders;

void SheetViewHeader::mouseReleaseEvent(QMouseEvent* event)
{
    QHeaderView::mouseReleaseEvent(event);
    Q_EMIT resizeFinished();
}

bool SheetViewHeader::viewportEvent(QEvent* e)
{
    if (e->type() == QEvent::ContextMenu) {
        auto* ce = static_cast<QContextMenuEvent*>(e);
        int section = logicalIndexAt(ce->pos());
        if (section >= 0) {
            if (orientation() == Qt::Horizontal) {
                if (!owner->selectionModel()->isColumnSelected(section, owner->rootIndex())) {
                    owner->clearSelection();
                    owner->selectColumn(section);
                }
            }
            else if (!owner->selectionModel()->isRowSelected(section, owner->rootIndex())) {
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
    for (const auto& item : list) {
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
    for (const auto& item : list) {
        int column = item.column();
        min = std::min(column, min);
        max = std::max(column, max);
    }
    return {min, max};
}

SheetTableView::SheetTableView(QWidget* parent)
    : QTableView(parent)
    , sheet(nullptr)
    , tabCounter(0)
{
    setHorizontalHeader(new SheetViewHeader(this, Qt::Horizontal));
    setVerticalHeader(new SheetViewHeader(this, Qt::Vertical));
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    connect(verticalHeader(), &QWidget::customContextMenuRequested, [this](const QPoint& point) {
        QMenu menu(this);
        const auto selection = selectionModel()->selectedRows();
        const auto& [min, max] = selectedMinMaxRows(selection);
        if (bool isContiguous = max - min == selection.size() - 1) {
            Q_UNUSED(isContiguous)
            /*: This is shown in the context menu for the vertical header in a spreadsheet.
                The number refers to how many lines are selected and will be inserted. */
            auto insertBefore = menu.addAction(tr("Insert %n row(s) above", "", selection.size()));
            connect(insertBefore, &QAction::triggered, this, &SheetTableView::insertRows);

            if (max < model()->rowCount() - 1) {
                auto insertAfter =
                    menu.addAction(tr("Insert %n row(s) below", "", selection.size()));
                connect(insertAfter, &QAction::triggered, this, &SheetTableView::insertRowsAfter);
            }
        }
        else {
            auto insert = menu.addAction(tr("Insert %n non-contiguous rows", "", selection.size()));
            connect(insert, &QAction::triggered, this, &SheetTableView::insertRows);
        }
        auto remove = menu.addAction(tr("Remove row(s)", "", selection.size()));
        connect(remove, &QAction::triggered, this, &SheetTableView::removeRows);
        menu.exec(verticalHeader()->mapToGlobal(point));
    });

    connect(horizontalHeader(), &QWidget::customContextMenuRequested, [this](const QPoint& point) {
        QMenu menu(this);
        const auto selection = selectionModel()->selectedColumns();
        const auto& [min, max] = selectedMinMaxColumns(selection);
        if (bool isContiguous = max - min == selection.size() - 1) {
            Q_UNUSED(isContiguous)
            /*: This is shown in the context menu for the horizontal header in a spreadsheet.
                The number refers to how many lines are selected and will be inserted. */
            auto insertAbove = menu.addAction(tr("Insert %n column(s) left", "", selection.size()));
            connect(insertAbove, &QAction::triggered, this, &SheetTableView::insertColumns);

            if (max < model()->columnCount() - 1) {
                auto insertAfter =
                    menu.addAction(tr("Insert %n column(s) right", "", selection.size()));
                connect(insertAfter,
                        &QAction::triggered,
                        this,
                        &SheetTableView::insertColumnsAfter);
            }
        }
        else {
            auto insert =
                menu.addAction(tr("Insert %n non-contiguous columns", "", selection.size()));
            connect(insert, &QAction::triggered, this, &SheetTableView::insertColumns);
        }
        auto remove = menu.addAction(tr("Remove column(s)", "", selection.size()));
        connect(remove, &QAction::triggered, this, &SheetTableView::removeColumns);
        menu.exec(horizontalHeader()->mapToGlobal(point));
    });

    actionProperties = new QAction(tr("Properties..."), this);
    addAction(actionProperties);

    horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    verticalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);

    contextMenu = new QMenu(this);

    contextMenu->addAction(actionProperties);
    connect(actionProperties, &QAction::triggered, this, &SheetTableView::cellProperties);

    contextMenu->addSeparator();
    actionRecompute = new QAction(tr("Recompute"), this);
    connect(actionRecompute, &QAction::triggered, this, &SheetTableView::onRecompute);
    contextMenu->addAction(actionRecompute);

    actionBind = new QAction(tr("Bind..."), this);
    connect(actionBind, &QAction::triggered, this, &SheetTableView::onBind);
    contextMenu->addAction(actionBind);

    actionConf = new QAction(tr("Configuration table..."), this);
    connect(actionConf, &QAction::triggered, this, &SheetTableView::onConfSetup);
    contextMenu->addAction(actionConf);

    horizontalHeader()->addAction(actionBind);
    verticalHeader()->addAction(actionBind);

    contextMenu->addSeparator();
    actionMerge = contextMenu->addAction(tr("Merge cells"));
    connect(actionMerge, &QAction::triggered, this, &SheetTableView::mergeCells);
    actionSplit = contextMenu->addAction(tr("Split cells"));
    connect(actionSplit, &QAction::triggered, this, &SheetTableView::splitCell);

    contextMenu->addSeparator();
    actionCut = contextMenu->addAction(tr("Cut"));
    connect(actionCut, &QAction::triggered, this, &SheetTableView::cutSelection);
    actionCopy = contextMenu->addAction(tr("Copy"));
    connect(actionCopy, &QAction::triggered, this, &SheetTableView::copySelection);
    actionPaste = contextMenu->addAction(tr("Paste"));
    connect(actionPaste, &QAction::triggered, this, &SheetTableView::pasteClipboard);
    actionDel = contextMenu->addAction(tr("Delete"));
    connect(actionDel, &QAction::triggered, this, &SheetTableView::deleteSelection);

    setTabKeyNavigation(false);

    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, [this]() {
        updateCellSpan();
    });
}

void SheetTableView::onRecompute()
{
    Gui::Command::openCommand("Recompute cells");
    for (auto& range : selectedRanges()) {
        Gui::cmdAppObjectArgs(sheet,
                              "recomputeCells('%s', '%s')",
                              range.fromCellString(),
                              range.toCellString());
    }
    Gui::Command::commitCommand();
}

void SheetTableView::onBind()
{
    auto ranges = selectedRanges();
    if (!ranges.empty() && ranges.size() <= 2) {
        DlgBindSheet dlg(sheet, ranges, this);
        dlg.exec();
    }
}

void SheetTableView::onConfSetup()
{
    auto ranges = selectedRanges();
    if (ranges.empty()) {
        return;
    }
    DlgSheetConf dlg(sheet, ranges.back(), this);
    dlg.exec();
}

void SheetTableView::cellProperties()
{
    std::unique_ptr<PropertiesDialog> dialog(new PropertiesDialog(sheet, selectedRanges(), this));

    if (dialog->exec() == QDialog::Accepted) {
        dialog->apply();
    }
}

std::vector<Range> SheetTableView::selectedRanges() const
{
    std::vector<Range> result;

    if (!sheet->getCells()->hasSpan()) {
        for (const auto& sel : selectionModel()->selection()) {
            result.emplace_back(sel.top(), sel.left(), sel.bottom(), sel.right());
        }
    }
    else {
        // If there is spanning cell, QItemSelection returned by
        // QTableView::selection() does not merge selected indices into ranges.
        // So we have to do it by ourselves. Qt records selection in the order
        // of column first and then row.
        //
        // Note that there will always be ambiguous cases with the available
        // information, where multiple user selected ranges are merged
        // together. For example, consecutive single column selections that
        // form a rectangle will be merged together, but single row selections
        // will not be merged.
        for (const auto& sel : selectionModel()->selection()) {
            if (!result.empty() && sel.bottom() == sel.top() && sel.right() == sel.left()) {
                auto& last = result.back();
                if (last.colCount() == 1 && last.from().col() == sel.left()
                    && sel.top() == last.to().row() + 1) {
                    // This is the case of rectangle selection. We keep
                    // accumulating the last column, and try to merge the
                    // column to previous range whenever possible.
                    last = Range(last.from(), CellAddress(sel.top(), sel.left()));
                    if (result.size() > 1) {
                        auto& secondLast = result[result.size() - 2];
                        if (secondLast.to().col() + 1 == last.to().col()
                            && secondLast.from().row() == last.from().row()
                            && secondLast.rowCount() == last.rowCount()) {
                            secondLast = Range(secondLast.from(), last.to());
                            result.pop_back();
                        }
                    }
                    continue;
                }
                else if (last.rowCount() == 1 && last.from().row() == sel.top()
                         && last.to().col() + 1 == sel.left()) {
                    // This is the case of single row selection
                    last = Range(last.from(), CellAddress(sel.top(), sel.left()));
                    continue;
                }
            }
            result.emplace_back(sel.top(), sel.left(), sel.bottom(), sel.right());
        }
    }
    return result;
}

QModelIndexList SheetTableView::selectedIndexesRaw() const
{
    return selectedIndexes();
}

void SheetTableView::insertRows()
{
    assert(sheet);

    QModelIndexList rows = selectionModel()->selectedRows();
    std::vector<int> sortedRows;

    /* Make sure rows are sorted in ascending order */
    for (const auto& it : rows) {
        sortedRows.push_back(it.row());
    }
    std::sort(sortedRows.begin(), sortedRows.end());

    /* Insert rows */
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Insert rows"));
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
            else {
                break;
            }
        }

        Gui::cmdAppObjectArgs(sheet, "insertRows('%s', %d)", rowName(prev).c_str(), count);
    }
    Gui::Command::commitCommand();
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
}

void SheetTableView::insertRowsAfter()
{
    assert(sheet);
    const auto rows = selectionModel()->selectedRows();
    const auto& [min, max] = selectedMinMaxRows(rows);
    assert(max - min == rows.size() - 1);
    Q_UNUSED(min)

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Insert rows"));
    Gui::cmdAppObjectArgs(sheet, "insertRows('%s', %d)", rowName(max + 1).c_str(), rows.size());
    Gui::Command::commitCommand();
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
}

void SheetTableView::removeRows()
{
    assert(sheet);

    QModelIndexList rows = selectionModel()->selectedRows();
    std::vector<int> sortedRows;

    /* Make sure rows are sorted in descending order */
    for (const auto& it : rows) {
        sortedRows.push_back(it.row());
    }
    std::sort(sortedRows.begin(), sortedRows.end(), std::greater<>());

    /* Remove rows */
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Remove rows"));
    for (const auto& it : sortedRows) {
        Gui::cmdAppObjectArgs(sheet, "removeRows('%s', %d)", rowName(it).c_str(), 1);
    }
    Gui::Command::commitCommand();
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
}

void SheetTableView::insertColumns()
{
    assert(sheet);

    QModelIndexList cols = selectionModel()->selectedColumns();
    std::vector<int> sortedColumns;

    /* Make sure rows are sorted in ascending order */
    for (const auto& it : cols) {
        sortedColumns.push_back(it.column());
    }
    std::sort(sortedColumns.begin(), sortedColumns.end());

    /* Insert columns */
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Insert columns"));
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
            else {
                break;
            }
        }

        Gui::cmdAppObjectArgs(sheet, "insertColumns('%s', %d)", columnName(prev).c_str(), count);
    }
    Gui::Command::commitCommand();
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
}

void SheetTableView::insertColumnsAfter()
{
    assert(sheet);
    const auto columns = selectionModel()->selectedColumns();
    const auto& [min, max] = selectedMinMaxColumns(columns);
    assert(max - min == columns.size() - 1);
    Q_UNUSED(min)

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Insert columns"));
    Gui::cmdAppObjectArgs(sheet,
                          "insertColumns('%s', %d)",
                          columnName(max + 1).c_str(),
                          columns.size());
    Gui::Command::commitCommand();
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
}

void SheetTableView::removeColumns()
{
    assert(sheet);

    QModelIndexList cols = selectionModel()->selectedColumns();
    std::vector<int> sortedColumns;

    /* Make sure rows are sorted in descending order */
    for (const auto& it : cols) {
        sortedColumns.push_back(it.column());
    }
    std::sort(sortedColumns.begin(), sortedColumns.end(), std::greater<>());

    /* Remove columns */
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Remove rows"));
    for (const auto& it : sortedColumns) {
        Gui::cmdAppObjectArgs(sheet, "removeColumns('%s', %d)", columnName(it).c_str(), 1);
    }
    Gui::Command::commitCommand();
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
}

SheetTableView::~SheetTableView() = default;

void SheetTableView::updateCellSpan()
{
    int rows, cols;

    // Unspan first to avoid overlap
    for (const auto& addr : spanChanges) {
        if (rowSpan(addr.row(), addr.col()) > 1 || columnSpan(addr.row(), addr.col()) > 1) {
            setSpan(addr.row(), addr.col(), 1, 1);
        }
    }

    for (const auto& addr : spanChanges) {
        sheet->getSpans(addr, rows, cols);
        if (rows > 1 || cols > 1) {
            setSpan(addr.row(), addr.col(), rows, cols);
        }
    }
    spanChanges.clear();
}

void SheetTableView::setSheet(Sheet* _sheet)
{
    sheet = _sheet;
    cellSpanChangedConnection = sheet->cellSpanChanged.connect([&](const CellAddress& addr) {
        spanChanges.insert(addr);
        timer.start(10);
    });

    // Update row and column spans
    std::vector<std::string> usedCells = sheet->getUsedCells();

    for (const auto& i : usedCells) {
        CellAddress address(i);

        if (sheet->isMergedCell(address)) {
            int rows, cols;
            sheet->getSpans(address, rows, cols);
            setSpan(address.row(), address.col(), rows, cols);
        }
    }

    // Update column widths and row height
    std::map<int, int> columWidths = sheet->getColumnWidths();
    for (std::map<int, int>::const_iterator i = columWidths.begin(); i != columWidths.end(); ++i) {
        int newSize = i->second;

        if (newSize > 0 && horizontalHeader()->sectionSize(i->first) != newSize) {
            setColumnWidth(i->first, newSize);
        }
    }

    std::map<int, int> rowHeights = sheet->getRowHeights();
    for (std::map<int, int>::const_iterator i = rowHeights.begin(); i != rowHeights.end(); ++i) {
        int newSize = i->second;

        if (newSize > 0 && verticalHeader()->sectionSize(i->first) != newSize) {
            setRowHeight(i->first, newSize);
        }
    }
}

void SheetTableView::commitData(QWidget* editor)
{
    QTableView::commitData(editor);
}

bool SheetTableView::edit(const QModelIndex& index, EditTrigger trigger, QEvent* event)
{
    if (trigger
        & (QAbstractItemView::DoubleClicked | QAbstractItemView::AnyKeyPressed
           | QAbstractItemView::EditKeyPressed)) {
        currentEditIndex = index;
    }
    return QTableView::edit(index, trigger, event);
}

bool SheetTableView::event(QEvent* event)
{
    if (event && event->type() == QEvent::KeyPress && this->hasFocus()) {
        // If this widget has focus, look for keyboard events that represent movement shortcuts
        // and handle them.
        QKeyEvent* kevent = static_cast<QKeyEvent*>(event);
        switch (kevent->key()) {
            case Qt::Key_Return:
                [[fallthrough]];
            case Qt::Key_Enter:
                [[fallthrough]];
            case Qt::Key_Home:
                [[fallthrough]];
            case Qt::Key_End:
                [[fallthrough]];
            case Qt::Key_Left:
                [[fallthrough]];
            case Qt::Key_Right:
                [[fallthrough]];
            case Qt::Key_Up:
                [[fallthrough]];
            case Qt::Key_Down:
                [[fallthrough]];
            case Qt::Key_Tab:
                [[fallthrough]];
            case Qt::Key_Backtab:
                finishEditWithMove(kevent->key(), kevent->modifiers(), true);
                return true;
            // Also handle the delete key here:
            case Qt::Key_Delete:
                deleteSelection();
                return true;
            case Qt::Key_Escape:
                sheet->setCopyOrCutRanges({});
                return true;
            default:
                break;
        }
        if (kevent->matches(QKeySequence::Cut)) {
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
        QKeyEvent* kevent = static_cast<QKeyEvent*>(event);
        if (kevent->modifiers() == Qt::NoModifier || kevent->modifiers() == Qt::ShiftModifier
            || kevent->modifiers() == Qt::KeypadModifier) {
            switch (kevent->key()) {
                case Qt::Key_Return:
                    [[fallthrough]];
                case Qt::Key_Enter:
                    [[fallthrough]];
                case Qt::Key_Delete:
                    [[fallthrough]];
                case Qt::Key_Home:
                    [[fallthrough]];
                case Qt::Key_End:
                    [[fallthrough]];
                case Qt::Key_Backspace:
                    [[fallthrough]];
                case Qt::Key_Left:
                    [[fallthrough]];
                case Qt::Key_Right:
                    [[fallthrough]];
                case Qt::Key_Up:
                    [[fallthrough]];
                case Qt::Key_Down:
                    [[fallthrough]];
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
    else if (event && event->type() == QEvent::LanguageChange) {
        actionProperties->setText(tr("Properties..."));
        actionRecompute->setText(tr("Recompute"));
        actionConf->setText(tr("Configuration table..."));
        actionMerge->setText(tr("Merge cells"));
        actionSplit->setText(tr("Split cells"));
        actionCopy->setText(tr("Copy"));
        actionPaste->setText(tr("Paste"));
        actionCut->setText(tr("Cut"));
        actionDel->setText(tr("Delete"));
        actionBind->setText(tr("Bind..."));
    }
    return QTableView::event(event);
}

void SheetTableView::deleteSelection()
{
    QModelIndexList selection = selectionModel()->selectedIndexes();

    if (!selection.empty()) {
        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Clear cell(s)"));
        std::vector<Range> ranges = selectedRanges();
        std::vector<Range>::const_iterator i = ranges.begin();

        for (; i != ranges.end(); ++i) {
            Gui::Command::doCommand(Gui::Command::Doc,
                                    "App.ActiveDocument.%s.clear('%s')",
                                    sheet->getNameInDocument(),
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

void SheetTableView::_copySelection(const std::vector<App::Range>& ranges, bool copy)
{
    int minRow = INT_MAX;
    int maxRow = 0;
    int minCol = INT_MAX;
    int maxCol = 0;
    for (auto& range : ranges) {
        minRow = std::min(minRow, range.from().row());
        maxRow = std::max(maxRow, range.to().row());
        minCol = std::min(minCol, range.from().col());
        maxCol = std::max(maxCol, range.to().col());
    }

    QString selectedText;
    for (int i = minRow; i <= maxRow; i++) {
        for (int j = minCol; j <= maxCol; j++) {
            QModelIndex index = model()->index(i, j);
            QString cell = index.data(Qt::EditRole).toString();
            if (j < maxCol) {
                cell.append(QChar::fromLatin1('\t'));
            }
            selectedText += cell;
        }
        if (i < maxRow) {
            selectedText.append(QChar::fromLatin1('\n'));
        }
    }

    Base::StringWriter writer;
    sheet->getCells()->copyCells(writer, ranges);
    QMimeData* mime = new QMimeData();
    mime->setText(selectedText);
    mime->setData(_SheetMime, QByteArray(writer.getString().c_str()));
    QApplication::clipboard()->setMimeData(mime);

    sheet->setCopyOrCutRanges(std::move(ranges), copy);
}

void SheetTableView::cutSelection()
{
    _copySelection(selectedRanges(), false);
}

void SheetTableView::pasteClipboard()
{
    App::AutoTransaction committer("Paste cell");
    try {
        bool copy = true;
        auto ranges = sheet->getCopyOrCutRange(copy);
        if (ranges.empty()) {
            copy = false;
            ranges = sheet->getCopyOrCutRange(copy);
        }

        if (!ranges.empty()) {
            _copySelection(ranges, copy);
        }

        const QMimeData* mimeData = QApplication::clipboard()->mimeData();
        if (!mimeData || !mimeData->hasText()) {
            return;
        }

        if (!copy) {
            for (auto& range : ranges) {
                do {
                    sheet->clear(*range);
                } while (range.next());
            }
        }

        ranges = selectedRanges();
        if (ranges.empty()) {
            return;
        }

        Range range = ranges.back();
        if (!mimeData->hasFormat(_SheetMime)) {
            CellAddress current = range.from();
            QString text = mimeData->text();
            QStringList cells = text.split(QLatin1Char('\n'));
            int i = 0;
            for (const auto& it : cells) {
                QStringList cols = it.split(QLatin1Char('\t'));
                int j = 0;
                for (const auto& jt : cols) {
                    QModelIndex index = model()->index(current.row() + i, current.col() + j);
                    model()->setData(index, jt);
                    j++;
                }
                i++;
            }
        }
        else {
            QByteArray res = mimeData->data(_SheetMime);
            Base::ByteArrayIStreambuf buf(res);
            std::istream in(nullptr);
            in.rdbuf(&buf);
            Base::XMLReader reader("<memory>", in);
            sheet->getCells()->pasteCells(reader, range);
        }

        GetApplication().getActiveDocument()->recompute();
    }
    catch (Base::Exception& e) {
        e.ReportException();
        QMessageBox::critical(Gui::getMainWindow(),
                              QObject::tr("Copy & Paste failed"),
                              QString::fromLatin1(e.what()));
        return;
    }
    clearSelection();
}

void SheetTableView::finishEditWithMove(int keyPressed,
                                        Qt::KeyboardModifiers modifiers,
                                        bool handleTabMotion)
{
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
            if (modifiers == Qt::ControlModifier) {
                targetColumn = 0;
            }
            tabCounter = 0;
            break;

        case Qt::Key_End: {
            // End should take you to the last occupied cell in the current column
            // Ctrl-End takes you to the last cell in the sheet
            auto usedCells = sheet->getCells()->getNonEmptyCells();
            for (const auto& cell : usedCells) {
                if (modifiers == Qt::NoModifier) {
                    if (cell.col() == targetColumn) {
                        targetRow = std::max(targetRow, cell.row());
                    }
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
            if (targetColumn == 0) {
                break;  // Nothing to do, we're already in the first column
            }
            if (modifiers == Qt::NoModifier || modifiers == Qt::ShiftModifier) {
                targetColumn--;
            }
            else if (modifiers == Qt::ControlModifier
                     || modifiers == (Qt::ControlModifier | Qt::ShiftModifier)) {
                scanForRegionBoundary(targetRow, targetColumn, 0, -1);
            }
            else {
                targetColumn--;  // Unrecognized modifier combination: default to just moving one
                                 // cell
            }
            tabCounter = 0;
            break;
        case Qt::Key_Right:
            if (targetColumn >= this->model()->columnCount() - 1) {
                break;  // Nothing to do, we're already in the last column
            }
            if (modifiers == Qt::NoModifier || modifiers == Qt::ShiftModifier) {
                targetColumn += colSpan;
            }
            else if (modifiers == Qt::ControlModifier
                     || modifiers == (Qt::ControlModifier | Qt::ShiftModifier)) {
                scanForRegionBoundary(targetRow, targetColumn, 0, 1);
            }
            else {
                targetColumn +=
                    colSpan;  // Unrecognized modifier combination: default to just moving one cell
            }
            tabCounter = 0;
            break;
        case Qt::Key_Up:
            if (targetRow == 0) {
                break;  // Nothing to do, we're already in the first column
            }
            if (modifiers == Qt::NoModifier || modifiers == Qt::ShiftModifier) {
                targetRow--;
            }
            else if (modifiers == Qt::ControlModifier
                     || modifiers == (Qt::ControlModifier | Qt::ShiftModifier)) {
                scanForRegionBoundary(targetRow, targetColumn, -1, 0);
            }
            else {
                targetRow--;  // Unrecognized modifier combination: default to just moving one cell
            }
            tabCounter = 0;
            break;
        case Qt::Key_Down:
            if (targetRow >= this->model()->rowCount() - 1) {
                break;  // Nothing to do, we're already in the last row
            }
            if (modifiers == Qt::NoModifier || modifiers == Qt::ShiftModifier) {
                targetRow += rowSpan;
            }
            else if (modifiers == Qt::ControlModifier
                     || modifiers == (Qt::ControlModifier | Qt::ShiftModifier)) {
                scanForRegionBoundary(targetRow, targetColumn, 1, 0);
            }
            else {
                targetRow +=
                    rowSpan;  // Unrecognized modifier combination: default to just moving one cell
            }
            tabCounter = 0;
            break;
        case Qt::Key_Tab:
            if (modifiers == Qt::NoModifier) {
                tabCounter++;
                if (handleTabMotion) {
                    targetColumn += colSpan;
                }
            }
            else if (modifiers == Qt::ShiftModifier) {
                tabCounter = 0;
                if (handleTabMotion) {
                    targetColumn--;
                }
            }
            break;
        case Qt::Key_Backtab:
            if (modifiers == Qt::NoModifier) {
                targetColumn--;
            }
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

    if (!(modifiers & Qt::ShiftModifier) || keyPressed == Qt::Key_Tab || keyPressed == Qt::Key_Enter
        || keyPressed == Qt::Key_Return) {
        // We have to use this method so that Ctrl-modifier combinations don't result in multiple
        // selection
        this->selectionModel()->setCurrentIndex(model()->index(targetRow, targetColumn),
                                                QItemSelectionModel::ClearAndSelect);
    }
    else if (modifiers & Qt::ShiftModifier) {
        // With shift down, this motion becomes a block selection command, rather than just simple
        // motion:
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
            // This range contains the current cell, so it's the one we're going to modify (assuming
            // we're at one of the corners)
            int rangeMinRow = range.top();
            int rangeMaxRow = range.bottom();
            int rangeMinCol = range.left();
            int rangeMaxCol = range.right();
            if ((startingRow == rangeMinRow || startingRow == rangeMaxRow)
                && (startingCol == rangeMinCol || startingCol == rangeMaxCol)) {
                if (range.contains(model()->index(targetRow, targetCol))) {
                    // If the range already contains the target cell, then we're making the range
                    // smaller
                    if (startingRow == rangeMinRow) {
                        rangeMinRow = targetRow;
                    }
                    if (startingRow == rangeMaxRow) {
                        rangeMaxRow = targetRow;
                    }
                    if (startingCol == rangeMinCol) {
                        rangeMinCol = targetCol;
                    }
                    if (startingCol == rangeMaxCol) {
                        rangeMaxCol = targetCol;
                    }
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
                QItemSelection newRange(model()->index(rangeMinRow, rangeMinCol),
                                        model()->index(rangeMaxRow, rangeMaxCol));
                this->selectionModel()->select(newRange, QItemSelectionModel::Select);
            }
            break;
        }
    }

    this->selectionModel()->setCurrentIndex(model()->index(targetRow, targetCol),
                                            QItemSelectionModel::Current);
}

void SheetTableView::mergeCells()
{
    Gui::Application::Instance->commandManager().runCommandByName("Spreadsheet_MergeCells");
}

void SheetTableView::splitCell()
{
    Gui::Application::Instance->commandManager().runCommandByName("Spreadsheet_SplitCell");
}

void SheetTableView::closeEditor(QWidget* editor, QAbstractItemDelegate::EndEditHint hint)
{
    QTableView::closeEditor(editor, hint);
}

void SheetTableView::mousePressEvent(QMouseEvent* event)
{
    tabCounter = 0;
    QTableView::mousePressEvent(event);
}

void SheetTableView::selectionChanged(const QItemSelection& selected,
                                      const QItemSelection& deselected)
{
    Gui::getMainWindow()->updateActions();
    QTableView::selectionChanged(selected, deselected);
}

void SheetTableView::edit(const QModelIndex& index)
{
    currentEditIndex = index;
    QTableView::edit(index);
}

void SheetTableView::contextMenuEvent(QContextMenuEvent*)
{
    const QMimeData* mimeData = QApplication::clipboard()->mimeData();
    if (!selectionModel()->hasSelection()) {
        actionCut->setEnabled(false);
        actionCopy->setEnabled(false);
        actionDel->setEnabled(false);
        actionPaste->setEnabled(false);
        actionSplit->setEnabled(false);
        actionMerge->setEnabled(false);
    }
    else {
        actionPaste->setEnabled(mimeData && (mimeData->hasText() || mimeData->hasText()));
        actionCut->setEnabled(true);
        actionCopy->setEnabled(true);
        actionDel->setEnabled(true);
        actionSplit->setEnabled(
            selectedIndexesRaw().size() == 1
            && sheet->isMergedCell(CellAddress(currentIndex().row(), currentIndex().column())));
        actionMerge->setEnabled(selectedIndexesRaw().size() > 1);
    }

    auto ranges = selectedRanges();
    actionBind->setEnabled(!ranges.empty() && ranges.size() <= 2);

    contextMenu->exec(QCursor::pos());
}

QString SheetTableView::toHtml() const
{
    auto cells = sheet->getCells()->getNonEmptyCells();
    int rowCount = 0;
    int colCount = 0;
    for (const auto& it : cells) {
        rowCount = std::max(rowCount, it.row());
        colCount = std::max(colCount, it.col());
    }

    std::unique_ptr<QTextDocument> doc(new QTextDocument);
    doc->setDocumentMargin(10);
    QTextCursor cursor(doc.get());

    cursor.movePosition(QTextCursor::Start);

    QTextTableFormat tableFormat;
    tableFormat.setCellSpacing(0.0);
    tableFormat.setCellPadding(2.0);
    QVector<QTextLength> constraints;
    for (int col = 0; col < colCount + 1; col++) {
        constraints.append(QTextLength(QTextLength::FixedLength, sheet->getColumnWidth(col)));
    }
    constraints.prepend(QTextLength(QTextLength::FixedLength, 30.0));
    tableFormat.setColumnWidthConstraints(constraints);

    QTextCharFormat boldFormat;
    QFont boldFont = boldFormat.font();
    boldFont.setBold(true);
    boldFormat.setFont(boldFont);

    QColor bgColor;
    bgColor.setNamedColor(QLatin1String("#f0f0f0"));
    QTextCharFormat bgFormat;
    bgFormat.setBackground(QBrush(bgColor));

    QTextTable* table = cursor.insertTable(rowCount + 2, colCount + 2, tableFormat);

    // The header cells of the rows
    for (int row = 0; row < rowCount + 1; row++) {
        QTextTableCell headerCell = table->cellAt(row + 1, 0);
        headerCell.setFormat(bgFormat);
        QTextCursor headerCellCursor = headerCell.firstCursorPosition();
        QString data = model()->headerData(row, Qt::Vertical).toString();
        headerCellCursor.insertText(data, boldFormat);
    }

    // The header cells of the columns
    for (int col = 0; col < colCount + 1; col++) {
        QTextTableCell headerCell = table->cellAt(0, col + 1);
        headerCell.setFormat(bgFormat);
        QTextCursor headerCellCursor = headerCell.firstCursorPosition();
        QTextBlockFormat blockFormat = headerCellCursor.blockFormat();
        blockFormat.setAlignment(Qt::AlignHCenter);
        headerCellCursor.setBlockFormat(blockFormat);
        QString data = model()->headerData(col, Qt::Horizontal).toString();
        headerCellCursor.insertText(data, boldFormat);
    }

    // The cells
    for (const auto& it : cells) {
        if (sheet->isMergedCell(it)) {
            int rows, cols;
            sheet->getSpans(it, rows, cols);
            table->mergeCells(it.row() + 1, it.col() + 1, rows, cols);
        }
        QModelIndex index = model()->index(it.row(), it.col());

        QTextCharFormat cellFormat;
        QTextTableCell cell = table->cellAt(it.row() + 1, it.col() + 1);

        // font
        QVariant font = model()->data(index, Qt::FontRole);
        if (font.isValid()) {
            cellFormat.setFont(font.value<QFont>());
        }

        // foreground
        QVariant fgColor = model()->data(index, Qt::ForegroundRole);
        if (fgColor.isValid()) {
            cellFormat.setForeground(QBrush(fgColor.value<QColor>()));
        }

        // background
        QVariant cbgClor = model()->data(index, Qt::BackgroundRole);
        if (cbgClor.isValid()) {
            QTextCharFormat bgFormat;
            bgFormat.setBackground(QBrush(cbgClor.value<QColor>()));
            cell.setFormat(bgFormat);
        }

        QTextCursor cellCursor = cell.firstCursorPosition();

        // alignment
        QVariant align = model()->data(index, Qt::TextAlignmentRole);
        if (align.isValid()) {
            Qt::Alignment alignment = static_cast<Qt::Alignment>(align.toInt());
            QTextBlockFormat blockFormat = cellCursor.blockFormat();
            blockFormat.setAlignment(alignment);
            cellCursor.setBlockFormat(blockFormat);

            // This doesn't seem to have any effect on single cells but works if several
            // cells are merged
            QTextCharFormat::VerticalAlignment valign = QTextCharFormat::AlignMiddle;
            QTextCharFormat format = cell.format();
            if (alignment & Qt::AlignTop) {
                valign = QTextCharFormat::AlignTop;
            }
            else if (alignment & Qt::AlignBottom) {
                valign = QTextCharFormat::AlignBottom;
            }
            format.setVerticalAlignment(valign);
            cell.setFormat(format);
        }

        // text
        QString data = model()->data(index).toString().simplified();
        cellCursor.insertText(data, cellFormat);
    }

    cursor.movePosition(QTextCursor::End);
    cursor.insertBlock();
    return doc->toHtml();
}

#include "moc_SheetTableView.cpp"
