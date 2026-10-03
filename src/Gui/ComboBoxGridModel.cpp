#include "ComboBoxGridModel.h"

#include <QAbstractItemModel>

using namespace Gui;

ComboBoxGridModel::ComboBoxGridModel(QObject* parent)
    : QAbstractTableModel(parent)
{}

void ComboBoxGridModel::setSourceModel(QAbstractItemModel* model)
{
    beginResetModel();

    m_sourceModel = model;

    endResetModel();
}

void ComboBoxGridModel::setColumnCount(int columns)
{
    columns = qMax(1, columns);

    if (m_columns == columns) {
        return;
    }

    beginResetModel();

    m_columns = columns;

    endResetModel();
}

int ComboBoxGridModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);

    return m_columns;
}

int ComboBoxGridModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid() || !m_sourceModel || m_columns <= 0) {
        return 0;
    }

    const int count = m_sourceModel->rowCount();

    return (count + m_columns - 1) / m_columns;
}

QModelIndex ComboBoxGridModel::sourceIndex(const QModelIndex& index) const
{
    if (!index.isValid() || !m_sourceModel) {
        return QModelIndex();
    }

    const int sourceRow = index.row() * m_columns + index.column();

    if (sourceRow < 0 || sourceRow >= m_sourceModel->rowCount()) {

        return QModelIndex();
    }

    return m_sourceModel->index(sourceRow, 0);
}

QVariant ComboBoxGridModel::data(const QModelIndex& index, int role) const
{
    const QModelIndex sourceIndexValue = sourceIndex(index);

    if (!sourceIndexValue.isValid()) {
        return QVariant();
    }

    return m_sourceModel->data(sourceIndexValue, role);
}

Qt::ItemFlags ComboBoxGridModel::flags(const QModelIndex& index) const
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

QHash<int, QByteArray> ComboBoxGridModel::roleNames() const
{
    if (!m_sourceModel) {
        return {};
    }

    return m_sourceModel->roleNames();
}
