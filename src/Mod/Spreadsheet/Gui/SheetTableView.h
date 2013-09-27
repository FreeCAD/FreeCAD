#ifndef SHEETTABLEVIEW_H
#define SHEETTABLEVIEW_H

#include <boost/signals/connection.hpp>
#include <QTableView>

namespace Spreadsheet {
class Sheet;
}

namespace SpreadsheetGui {

class SheetTableView : public QTableView
{
    Q_OBJECT
public:
    explicit SheetTableView(QWidget *parent = 0);
    ~SheetTableView();
    
    void edit(const QModelIndex &index);
    void setSheet(Spreadsheet::Sheet *_sheet);
protected Q_SLOTS:
    void commitData(QWidget *editor);
    void updateCellSpan(int row, int col);
protected:
    bool edit(const QModelIndex &index, EditTrigger trigger, QEvent *event);

    QModelIndex currentEditIndex;
    Spreadsheet::Sheet * sheet;

#ifdef signals
#undef signals
#endif
#define signals signals
    boost::BOOST_SIGNALS_NAMESPACE::scoped_connection cellSpanChangedConnection;
#undef signals
};

}

#endif // SHEETTABLEVIEW_H
