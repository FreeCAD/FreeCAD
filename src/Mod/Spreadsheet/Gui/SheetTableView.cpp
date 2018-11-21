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
# include <QApplication>
# include <QClipboard>
#endif

#include <Gui/Command.h>
#include <boost/bind.hpp>
#include "../App/Utils.h"
#include <App/Range.h>
#include "SheetTableView.h"
#include "LineEdit.h"
#include "PropertiesDialog.h"

using namespace SpreadsheetGui;
using namespace Spreadsheet;
using namespace App;

void SheetViewHeader::mouseReleaseEvent(QMouseEvent *event)
{
    QHeaderView::mouseReleaseEvent(event);
    resizeFinished();
}

SheetTableView::SheetTableView(QWidget *parent)
    : QTableView(parent)
    , sheet(0)
{
    QAction * insertRows = new QAction(tr("Insert rows"), this);
    QAction * removeRows = new QAction(tr("Remove rows"), this);
    QAction * insertColumns = new QAction(tr("Insert columns"), this);
    QAction * removeColumns = new QAction(tr("Remove columns"), this);

    setHorizontalHeader(new SheetViewHeader(Qt::Horizontal));
    setVerticalHeader(new SheetViewHeader(Qt::Vertical));

    horizontalHeader()->addAction(insertColumns);
    horizontalHeader()->addAction(removeColumns);
    horizontalHeader()->setContextMenuPolicy(Qt::ActionsContextMenu);

    verticalHeader()->addAction(insertRows);
    verticalHeader()->addAction(removeRows);
    verticalHeader()->setContextMenuPolicy(Qt::ActionsContextMenu);

    connect(insertRows, SIGNAL(triggered()), this, SLOT(insertRows()));
    connect(insertColumns, SIGNAL(triggered()), this, SLOT(insertColumns()));
    connect(removeRows, SIGNAL(triggered()), this, SLOT(removeRows()));
    connect(removeColumns, SIGNAL(triggered()), this, SLOT(removeColumns()));

    QAction * cellProperties = new QAction(tr("Properties..."), this);
    addAction(cellProperties);

    setContextMenuPolicy(Qt::ActionsContextMenu);
    setTabKeyNavigation(false);

    connect(cellProperties, SIGNAL(triggered()), this, SLOT(cellProperties()));
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

        result.push_back(Range(ul.first, ul.second,
                                                   ul.first + size.first - 1, ul.second + size.second - 1));
    }

    return result;
}

void SheetTableView::insertRows()
{
    assert(sheet != 0);

    QModelIndexList rows = selectionModel()->selectedRows();
    std::vector<int> sortedRows;

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

        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.insertRows('%s', %d)", sheet->getNameInDocument(),
                                rowName(prev).c_str(), count);
    }
    Gui::Command::commitCommand();
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
}

void SheetTableView::removeRows()
{
    assert(sheet != 0);

    QModelIndexList rows = selectionModel()->selectedRows();
    std::vector<int> sortedRows;

    /* Make sure rows are sorted in descending order */
    for (QModelIndexList::const_iterator it = rows.begin(); it != rows.end(); ++it)
        sortedRows.push_back(it->row());
    std::sort(sortedRows.begin(), sortedRows.end(), std::greater<int>());

    /* Remove rows */
    Gui::Command::openCommand("Remove rows");
    for (std::vector<int>::const_iterator it = sortedRows.begin(); it != sortedRows.end(); ++it) {
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.removeRows('%s', %d)", sheet->getNameInDocument(),
                                rowName(*it).c_str(), 1);
    }
    Gui::Command::commitCommand();
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
}

void SheetTableView::insertColumns()
{
    assert(sheet != 0);

    QModelIndexList cols = selectionModel()->selectedColumns();
    std::vector<int> sortedColumns;

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

        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.insertColumns('%s', %d)", sheet->getNameInDocument(),
                                columnName(prev).c_str(), count);
    }
    Gui::Command::commitCommand();
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
}

void SheetTableView::removeColumns()
{
    assert(sheet != 0);

    QModelIndexList cols = selectionModel()->selectedColumns();
    std::vector<int> sortedColumns;

    /* Make sure rows are sorted in descending order */
    for (QModelIndexList::const_iterator it = cols.begin(); it != cols.end(); ++it)
        sortedColumns.push_back(it->column());
    std::sort(sortedColumns.begin(), sortedColumns.end(), std::greater<int>());

    /* Remove columns */
    Gui::Command::openCommand("Remove rows");
    for (std::vector<int>::const_iterator it = sortedColumns.begin(); it != sortedColumns.end(); ++it)
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.removeColumns('%s', %d)", sheet->getNameInDocument(),
                                columnName(*it).c_str(), 1);
    Gui::Command::commitCommand();
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
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
    cellSpanChangedConnection = sheet->cellSpanChanged.connect(bind(&SheetTableView::updateCellSpan, this, _1));

    // Update row and column spans
    std::vector<std::string> usedCells = sheet->getUsedCells();

    for (std::vector<std::string>::const_iterator i = usedCells.begin(); i != usedCells.end(); ++i) {
        CellAddress address(*i);

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
        Gui::Command::commitCommand();
        Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
    }
}

void SheetTableView::copySelection()
{
    QModelIndexList selection = selectionModel()->selectedIndexes();
    int minRow = INT_MAX;
    int maxRow = 0;
    int minCol = INT_MAX;
    int maxCol = 0;

    for (auto it : selection) {
        int row = it.row();
        int col = it.column();
        minRow = std::min(minRow, row);
        maxRow = std::max(maxRow, row);
        minCol = std::min(minCol, col);
        maxCol = std::max(maxCol, col);
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
    QApplication::clipboard()->setText(selectedText);
}

void SheetTableView::cutSelection()
{
    copySelection();
    deleteSelection();
}

void SheetTableView::pasteClipboard()
{
    QString text = QApplication::clipboard()->text();
    QStringList rows = text.split(QLatin1Char('\n'));

    QModelIndex current = currentIndex();
    int i=0;
    for (auto it : rows) {
        QStringList cols = it.split(QLatin1Char('\t'));
        int j=0;
        for (auto jt : cols) {
            QModelIndex index = model()->index(current.row()+i, current.column()+j);
            model()->setData(index, jt);
            j++;
        }
        i++;
    }
}

void SheetTableView::closeEditor(QWidget * editor, QAbstractItemDelegate::EndEditHint hint)
{
    SpreadsheetGui::LineEdit * le = qobject_cast<SpreadsheetGui::LineEdit*>(editor);

    currentEditIndex = QModelIndex();
    QTableView::closeEditor(editor, hint);
    setCurrentIndex(le->next());
}

void SheetTableView::edit ( const QModelIndex & index )
{
    currentEditIndex = index;
    QTableView::edit(index);
}

#include "moc_SheetTableView.cpp"
