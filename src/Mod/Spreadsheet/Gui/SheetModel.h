#ifndef SHEETMODEL_H
#define SHEETMODEL_H

#include <QAbstractTableModel>
#include <boost/signals/connection.hpp>

namespace Spreadsheet {
class Sheet;
}

namespace SpreadsheetGui {

class SheetModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit SheetModel(Spreadsheet::Sheet * _sheet, QObject *parent = 0);
    ~SheetModel();
    
    SheetModel(QObject *parent);
    int rowCount(const QModelIndex &parent = QModelIndex()) const ;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    Qt::ItemFlags flags(const QModelIndex &) const;

private:
    void cellUpdated(int row, int col);

    boost::BOOST_SIGNALS_NAMESPACE::scoped_connection cellUpdatedConnection;
    Spreadsheet::Sheet * sheet;
};

}

#endif // SHEETMODEL_H
