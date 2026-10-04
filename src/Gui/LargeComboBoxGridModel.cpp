#include "LargeComboBoxGridModel.h"

#include <QAbstractItemModel>

using namespace Gui;

LargeComboBoxGridModel::LargeComboBoxGridModel(QObject* parent)
    : QAbstractTableModel(parent)
{}

void LargeComboBoxGridModel::setColumnCount(int columns)
{
    columns = qMax(1, columns);

    if (m_columns == columns) {
        return;
    }

    beginResetModel();

    m_columns = columns;

    endResetModel();
}

void LargeComboBoxGridModel::setRowCount(int rows)
{
    rows = qMax(0, rows);

    if (m_rows == rows) {
        return;
    }

    beginResetModel();

    m_rows = rows;

    endResetModel();
}

void LargeComboBoxGridModel::resetRowCount()
{
    if (m_rows == 0) {
        return;
    }

    beginResetModel();

    m_rows = 0;

    endResetModel();
}

void LargeComboBoxGridModel::setSourceModel(QAbstractItemModel* model)
{
    beginResetModel();

    m_sourceModel = model;

    endResetModel();
}

int LargeComboBoxGridModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);

    return m_columns;
}

int LargeComboBoxGridModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid() || !m_sourceModel || m_columns <= 0) {
        return 0;
    }

    return m_rows;
}

QModelIndex LargeComboBoxGridModel::sourceIndex(const QModelIndex& index) const
{
    if (!index.isValid() || !m_sourceModel) {
        return QModelIndex();
    }

    const int sourceRow = index.column() * m_rows + index.row();

    if (sourceRow < 0 || sourceRow >= m_sourceModel->rowCount()) {
        return QModelIndex();
    }

    return m_sourceModel->index(sourceRow, 0);
}

QVariant LargeComboBoxGridModel::data(const QModelIndex& index, int role) const
{
    const QModelIndex sourceIndexValue = sourceIndex(index);

    if (!sourceIndexValue.isValid()) {
        return QVariant();
    }

    return m_sourceModel->data(sourceIndexValue, role);
}

Qt::ItemFlags LargeComboBoxGridModel::flags(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    const QModelIndex sourceIndexValue = sourceIndex(index);

    if (!sourceIndexValue.isValid()) {
        return Qt::NoItemFlags;
    }

    return m_sourceModel->flags(sourceIndexValue);
}

QHash<int, QByteArray> LargeComboBoxGridModel::roleNames() const
{
    if (!m_sourceModel) {
        return {};
    }

    return m_sourceModel->roleNames();
}
