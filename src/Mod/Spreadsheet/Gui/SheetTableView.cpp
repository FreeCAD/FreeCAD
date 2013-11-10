#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include "SheetTableView.h"
#include <QKeyEvent>
#include <QAction>
#include <QHeaderView>
#include <Gui/Command.h>
#include "../App/Sheet.h"

using namespace SpreadsheetGui;

SheetTableView::SheetTableView(QWidget *parent)
    : QTableView(parent)
{
    QAction * insertRows = new QAction(tr("Insert rows"), this);
    QAction * removeRows = new QAction(tr("Remove rows"), this);
    QAction * insertColumns = new QAction(tr("Insert columns"), this);
    QAction * removeColumns = new QAction(tr("Remove columns"), this);

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
                                Spreadsheet::Sheet::rowName(prev).c_str(), count);
    }
    Gui::Command::commitCommand();
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
}

void SheetTableView::removeRows()
{
    assert(sheet != 0);

    QModelIndexList rows = selectionModel()->selectedRows();
    std::vector<int> sortedRows;
    int extra = 0;

    /* Make sure rows are sorted in descending order */
    for (QModelIndexList::const_iterator it = rows.begin(); it != rows.end(); ++it)
        sortedRows.push_back(it->row());
    std::sort(sortedRows.begin(), sortedRows.end(), std::greater<int>());

    /* Remove rows */
    Gui::Command::openCommand("Remove rows");
    for (std::vector<int>::const_iterator it = sortedRows.begin(); it != sortedRows.end(); ++it) {
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.removeRows('%s', %d)", sheet->getNameInDocument(),
                                Spreadsheet::Sheet::rowName(*it).c_str(), 1);
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
                                Spreadsheet::Sheet::columnName(prev).c_str(), count);
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
                                Spreadsheet::Sheet::columnName(*it).c_str(), 1);
    Gui::Command::commitCommand();
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
}

SheetTableView::~SheetTableView()
{

}

void SheetTableView::updateCellSpan(int row, int col)
{
    int rows, cols;

    sheet->getSpans(row, col, rows, cols);

    setSpan(row, col, rows, cols);
}

void SheetTableView::setSheet(Spreadsheet::Sheet * _sheet)
{
    sheet = _sheet;
    cellSpanChangedConnection = sheet->cellSpanChanged.connect(bind(&SheetTableView::updateCellSpan, this, _1, _2));

    // Update row and column spans
    std::vector<std::string> usedCells = sheet->getUsedCells();

    for (std::vector<std::string>::const_iterator i = usedCells.begin(); i != usedCells.end(); ++i) {
        int row, col;

        Spreadsheet::Sheet::addressToRowCol(i->c_str(), row, col);

        if (sheet->isMergedCell(row, col))
            updateCellSpan(row, col);
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

    QModelIndex c = currentEditIndex;
    QModelIndex n = model()->index(c.row() + 1, c.column());

    if (n.isValid())
        setCurrentIndex(n);
}

bool SheetTableView::edit ( const QModelIndex & index, EditTrigger trigger, QEvent * event )
{
    if (trigger & (QAbstractItemView::DoubleClicked | QAbstractItemView::AnyKeyPressed | QAbstractItemView::EditKeyPressed) )
        currentEditIndex = index;
    QTableView::edit(index, trigger, event);
}

void SheetTableView::edit ( const QModelIndex & index )
{
    currentEditIndex = index;
    QTableView::edit(index);
}

#include "moc_SheetTableView.cpp"
