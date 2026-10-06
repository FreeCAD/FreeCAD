#include "LargeComboBoxFilterModel.h"

#include "FuzzyMatcher.h"

#include <QAbstractItemModel>

using namespace Gui;

LargeComboBoxFilterModel::LargeComboBoxFilterModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
    sort(0, Qt::AscendingOrder);
}

void LargeComboBoxFilterModel::setSearchText(const QString& text)
{
    const QString newSearchText = text.trimmed();

    if (m_searchText == newSearchText) {
        return;
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    beginFilterChange();
    m_searchText = newSearchText;
    endFilterChange(Direction::Rows);
#else
    m_searchText = newSearchText;
    invalidateFilter();
#endif

    sort(0, Qt::AscendingOrder);
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

    int score = 0;
    return FuzzyMatcher::match(m_searchText, value, score);
}

bool LargeComboBoxFilterModel::lessThan(const QModelIndex& sourceLeft, const QModelIndex& sourceRight) const
{
    if (m_searchText.isEmpty()) {
        return sourceLeft.row() < sourceRight.row();
    }

    const QString leftValue = sourceModel()->data(sourceLeft, Qt::DisplayRole).toString();
    const QString rightValue = sourceModel()->data(sourceRight, Qt::DisplayRole).toString();

    int leftScore = 0;
    int rightScore = 0;

    FuzzyMatcher::match(m_searchText, leftValue, leftScore);
    FuzzyMatcher::match(m_searchText, rightValue, rightScore);

    if (leftScore != rightScore) {
        return leftScore > rightScore;
    }

    // original order when same score
    return sourceLeft.row() < sourceRight.row();
}
