#include "LargeComboBoxFilterModel.h"

#include <QAbstractItemModel>

using namespace Gui;

LargeComboBoxFilterModel::LargeComboBoxFilterModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setDynamicSortFilter(true);
}

void LargeComboBoxFilterModel::setSearchText(const QString& text)
{
    m_searchText = text.trimmed();
    invalidateFilter();
}

bool LargeComboBoxFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    if (m_searchText.isEmpty()) {
        return true;
    }

    const QModelIndex index = sourceModel()->index(sourceRow, filterKeyColumn(), sourceParent);

    if (!index.isValid()) {
        return false;
    }

    const QString value = sourceModel()->data(index, Qt::DisplayRole).toString();

    const QStringList tokens = m_searchText.split(' ', Qt::SkipEmptyParts);

    for (const QString& token : tokens) {
        if (!value.contains(token, Qt::CaseInsensitive)) {
            return false;
        }
    }

    return true;
}
