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
# include <QMenu>
# include <QMessageBox>
# include <QMimeData>
#endif

#include <App/Application.h>
#include <App/AutoTransaction.h>
#include <App/Document.h>
#include <Gui/CommandT.h>
#include <Gui/MainWindow.h>
#include <boost_bind_bind.hpp>
#include "../App/Utils.h"
#include "../App/Cell.h"
#include <App/Range.h>
#include "SheetTableView.h"
#include "LineEdit.h"
#include "PropertiesDialog.h"

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
{
    setHorizontalHeader(new SheetViewHeader(this,Qt::Horizontal));
    setVerticalHeader(new SheetViewHeader(this,Qt::Vertical));
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    verticalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);

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
            menu.exec(horizontalHeader()->mapToGlobal(point));
       });
       
    auto cellProperties = new QAction(tr("Properties..."), this);
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

    /* Make sure rows are sorted in ascending order */
    for (QModelIndexList::const_iterator it = rows.begin(); it != rows.end(); ++it)
        sortedRows.push_back(it->row());
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
            else
                break;
        }

        Gui::cmdAppObjectArgs(sheet, "insertRows('%s', %d)", rowName(prev).c_str(), count);
    }
    Gui::Command::commitCommand();
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
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

    QModelIndexList rows = selectionModel()->selectedRows();
    std::vector<int> sortedRows;

    /* Make sure rows are sorted in descending order */
    for (QModelIndexList::const_iterator it = rows.begin(); it != rows.end(); ++it)
        sortedRows.push_back(it->row());
    std::sort(sortedRows.begin(), sortedRows.end(), std::greater<int>());

    /* Remove rows */
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Remove rows"));
    for (std::vector<int>::const_iterator it = sortedRows.begin(); it != sortedRows.end(); ++it) {
        Gui::cmdAppObjectArgs(sheet, "removeRows('%s', %d)", rowName(*it).c_str(), 1);
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
            else
                break;
        }

        Gui::cmdAppObjectArgs(sheet, "insertColumns('%s', %d)",
                                     columnName(prev).c_str(), count);
    }
    Gui::Command::commitCommand();
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
}

void SheetTableView::insertColumnsAfter()
{
    assert(sheet != 0);
    const auto columns = selectionModel()->selectedColumns();
    const auto & [min, max] = selectedMinMaxColumns(columns);
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

    /* Make sure rows are sorted in descending order */
    for (QModelIndexList::const_iterator it = cols.begin(); it != cols.end(); ++it)
        sortedColumns.push_back(it->column());
    std::sort(sortedColumns.begin(), sortedColumns.end(), std::greater<int>());

    /* Remove columns */
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Remove rows"));
    for (std::vector<int>::const_iterator it = sortedColumns.begin(); it != sortedColumns.end(); ++it)
        Gui::cmdAppObjectArgs(sheet, "removeColumns('%s', %d)",
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
    cellSpanChangedConnection = sheet->cellSpanChanged.connect(bind(&SheetTableView::updateCellSpan, this, bp::_1));

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
        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Clear cell(s)"));
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

static const QLatin1String _SheetMime("application/x-fc-spreadsheet");

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

    Base::StringWriter writer;
    sheet->getCells()->copyCells(writer,selectedRanges());
    QMimeData *mime = new QMimeData();
    mime->setText(selectedText);
    mime->setData(_SheetMime,QByteArray(writer.getString().c_str()));
    QApplication::clipboard()->setMimeData(mime);
}

void SheetTableView::cutSelection()
{
    copySelection();
    deleteSelection();
}

void SheetTableView::pasteClipboard()
{
    const QMimeData* mimeData = QApplication::clipboard()->mimeData();
    if(!mimeData || !mimeData->hasText())
        return;

    if(selectionModel()->selectedIndexes().size()>1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Spreadsheet"),
                QObject::tr("Spreadsheet does not support range selection when pasting.\n"
                            "Please select one cell only."));
        return;
    }

    QModelIndex current = currentIndex();

    App::AutoTransaction committer("Paste cell");
    try {
        if (!mimeData->hasFormat(_SheetMime)) {
            QStringList cells;
            QString text = mimeData->text();
            int i=0;
            for (auto it : text.split(QLatin1Char('\n'))) {
                QStringList cols = it.split(QLatin1Char('\t'));
                int j=0;
                for (auto jt : cols) {
                    QModelIndex index = model()->index(current.row()+i, current.column()+j);
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
            sheet->getCells()->pasteCells(reader,CellAddress(current.row(),current.column()));
        }

        GetApplication().getActiveDocument()->recompute();

    }catch(Base::Exception &e) {
        e.ReportException();
        QMessageBox::critical(Gui::getMainWindow(), QObject::tr("Copy & Paste failed"),
                QString::fromLatin1(e.what()));
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
