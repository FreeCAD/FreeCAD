#include "ComboBoxFilterModel.h"

#include <QAbstractItemModel>

using namespace Gui;

ComboBoxFilterModel::ComboBoxFilterModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setDynamicSortFilter(true);
}

void ComboBoxFilterModel::setSearchText(const QString& text)
{
    m_searchText = text.trimmed();
    invalidateFilter();
}

bool ComboBoxFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
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
