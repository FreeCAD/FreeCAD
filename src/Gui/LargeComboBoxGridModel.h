#pragma once

#include <QAbstractTableModel>

class QAbstractItemModel;

namespace Gui
{

class LargeComboBoxGridModel: public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit LargeComboBoxGridModel(QObject* parent = nullptr);

    void setSourceModel(QAbstractItemModel* model);
    void setColumnCount(int columns);
    void setRowCount(int rows);
    void resetRowCount();

    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    QModelIndex sourceIndex(const QModelIndex& index) const;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    QAbstractItemModel* m_sourceModel = nullptr;
    int m_columns = 1;
    int m_rows = 0;
};

}  // namespace Gui
