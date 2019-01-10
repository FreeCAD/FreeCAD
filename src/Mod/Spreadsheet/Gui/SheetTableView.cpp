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
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <boost/bind.hpp>
#include "../App/Utils.h"
#include "../App/Cell.h"
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
    QAction * hideRows = new QAction(tr("Toggle rows"), this);
    actionShowRows = new QAction(tr("Show all rows"), this);
    actionShowRows->setCheckable(true);
    QAction * insertColumns = new QAction(tr("Insert columns"), this);
    QAction * removeColumns = new QAction(tr("Remove columns"), this);
    QAction * hideColumns = new QAction(tr("Toggle columns"), this);
    actionShowColumns = new QAction(tr("Show all columns"), this);
    actionShowColumns->setCheckable(true);

    setHorizontalHeader(new SheetViewHeader(Qt::Horizontal));
    setVerticalHeader(new SheetViewHeader(Qt::Vertical));
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

    setTabKeyNavigation(false);

    contextMenu = new QMenu(this);

    QAction * cellProperties = new QAction(tr("Properties..."), this);
    contextMenu->addAction(cellProperties);

    connect(cellProperties, SIGNAL(triggered()), this, SLOT(cellProperties()));

    QActionGroup *editGroup = new QActionGroup(this);
    editGroup->setExclusive(true);
    actionEditNormal = new QAction(tr("Normal"),this);
    actionEditNormal->setCheckable(true);
    actionEditNormal->setData(QVariant((int)Cell::EditNormal));
    editGroup->addAction(actionEditNormal);
    actionEditButton = new QAction(tr("Button"),this);
    actionEditButton->setCheckable(true);
    actionEditButton->setData(QVariant((int)Cell::EditButton));
    editGroup->addAction(actionEditButton);
    actionEditCombo = new QAction(tr("ComboBox"),this);
    actionEditCombo->setCheckable(true);
    actionEditCombo->setData(QVariant((int)Cell::EditCombo));
    editGroup->addAction(actionEditCombo);
    actionEditLabel = new QAction(tr("Label"),this);
    actionEditLabel->setCheckable(true);
    actionEditLabel->setData(QVariant((int)Cell::EditLabel));
    editGroup->addAction(actionEditLabel);

    QMenu *subMenu = new QMenu(tr("Edit mode"),contextMenu);
    contextMenu->addMenu(subMenu);
    subMenu->addActions(editGroup->actions());
    connect(editGroup, SIGNAL(triggered(QAction*)), this, SLOT(editMode(QAction*)));

    QAction *recompute = new QAction(tr("Recompute"),this);
    connect(recompute, SIGNAL(triggered()), this, SLOT(onRecompute()));
    contextMenu->addAction(recompute);

    contextMenu->addSeparator();
    actionCut = contextMenu->addAction(tr("Cut"));
    connect(actionCut,SIGNAL(triggered()), this, SLOT(cutSelection()));
    actionCopy = contextMenu->addAction(tr("Copy"));
    connect(actionCopy,SIGNAL(triggered()), this, SLOT(copySelection()));
    actionPaste = contextMenu->addAction(tr("Paste"));
    connect(actionPaste,SIGNAL(triggered()), this, SLOT(pasteClipboard()));
    actionDel = contextMenu->addAction(tr("Delete"));
    connect(actionDel,SIGNAL(triggered()), this, SLOT(deleteSelection()));
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
            auto cell = sheet->getCell(CellAddress(index.row(), index.column()));
            if(cell) {
                cell->setEditMode((Cell::EditMode)mode);
                if(mode == Cell::EditButton)
                    openPersistentEditor(index);
                else
                    closePersistentEditor(index);
            }
        }
    }catch(Base::Exception &e) {
        e.ReportException();
        Gui::Command::abortCommand();
    }
    Gui::Command::commitCommand();
}

void SheetTableView::onRecompute() {
    Gui::Command::openCommand("Recompute cells");
    for(auto &range : selectedRanges()) {
        FCMD_OBJ_CMD(sheet, "touchCells('" << range.fromCellString()
                << "','" << range.toCellString() << "')");
    }
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
    Gui::Command::commitCommand();
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

        FCMD_OBJ_CMD2("insertRows('%s', %d)", sheet,
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
        FCMD_OBJ_CMD2("removeRows('%s', %d)", sheet,
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

        FCMD_OBJ_CMD2("insertColumns('%s', %d)", sheet,
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
        FCMD_OBJ_CMD2("removeColumns('%s', %d)", sheet,
                                columnName(*it).c_str(), 1);
    Gui::Command::commitCommand();
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
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
    cellSpanChangedConnection = sheet->cellSpanChanged.connect(bind(&SheetTableView::updateCellSpan, this, _1));

    // Update row and column spans
    std::vector<std::string> usedCells = sheet->getUsedCells();

    for (std::vector<std::string>::const_iterator i = usedCells.begin(); i != usedCells.end(); ++i) {
        CellAddress address(*i);
        auto cell = sheet->getCell(address);
        if(cell && cell->getEditMode()==Cell::EditButton)
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
    if(!mimeData || (!mimeData->hasText() && !mimeData->hasText()))
        return;

    if(selectionModel()->selectedIndexes().size()>1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Spreadsheet"),
                QObject::tr("Spreadsheet does not support range selection when pasting.\n"
                            "Please select one cell only."));
        return;
    }

    QModelIndex current = currentIndex();

    GetApplication().setActiveTransaction("Paste cell");
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
    GetApplication().closeActiveTransaction();
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
    for(auto &range : selectedRanges()) {
        do {
            auto cell = sheet->getCell(range.address());
            if(!cell) continue;
            switch(cell->getEditMode()) {
            case Cell::EditButton:
                action = actionEditButton;
                break;
            case Cell::EditCombo:
                action = actionEditCombo;
                break;
            case Cell::EditLabel:
                action = actionEditLabel;
                break;
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

    const QMimeData* mimeData = QApplication::clipboard()->mimeData();
    if(!selectionModel()->hasSelection()) {
        actionCut->setEnabled(false);
        actionCopy->setEnabled(false);
        actionDel->setEnabled(false);
        actionPaste->setEnabled(false);
    }else{
        actionPaste->setEnabled(mimeData && (mimeData->hasText() || mimeData->hasText()));
        actionCut->setEnabled(true);
        actionCopy->setEnabled(true);
        actionDel->setEnabled(true);
    }

    contextMenu->exec(QCursor::pos());
}

void SheetTableView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight
#if QT_VERSION >= 0x050000
        , const QVector<int> &roles
#endif
        )
{
    App::Range range(topLeft.row(),topLeft.column(),bottomRight.row(),bottomRight.column());
    do {
        auto address = *range;
        auto cell = sheet->getCell(address);
        if(!cell)
            closePersistentEditor(model()->index(address.row(),address.col()));
    }while(range.next());

#if QT_VERSION >= 0x050000
    QTableView::dataChanged(topLeft,bottomRight,roles);
#else
    QTableView::dataChanged(topLeft,bottomRight);
#endif
}

#include "moc_SheetTableView.cpp"
