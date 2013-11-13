#ifndef SHEETTABLEVIEW_H
#define SHEETTABLEVIEW_H



#include <QTableView>
#include <boost/signals/connection.hpp>
#include "PreCompiled.h"
#include <Mod/Spreadsheet/App/Sheet.h>

namespace SpreadsheetGui {

class SheetTableView : public QTableView
{
    Q_OBJECT
public:
    explicit SheetTableView(QWidget *parent = 0);
    ~SheetTableView();
    
    void edit(const QModelIndex &index);
    void setSheet(Spreadsheet::Sheet *_sheet);
    std::vector<Spreadsheet::Sheet::Range> selectedRanges() const;
protected Q_SLOTS:
    void commitData(QWidget *editor);
    void updateCellSpan(int row, int col);
    void insertRows();
    void removeRows();
    void insertColumns();
    void removeColumns();
    void cellProperties();
protected:
    bool edit(const QModelIndex &index, EditTrigger trigger, QEvent *event);

    QModelIndex currentEditIndex;
    Spreadsheet::Sheet * sheet;

    boost::BOOST_SIGNALS_NAMESPACE::scoped_connection cellSpanChangedConnection;
};

}

#endif // SHEETTABLEVIEW_H
