#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include "SheetTableView.h"
#include <QKeyEvent>
#include "../App/Sheet.h"

using namespace SpreadsheetGui;

SheetTableView::SheetTableView(QWidget *parent)
    : QTableView(parent)
{
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
