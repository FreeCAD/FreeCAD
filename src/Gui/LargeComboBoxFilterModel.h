#pragma once

#include <QSortFilterProxyModel>

namespace Gui
{

class LargeComboBoxFilterModel: public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit LargeComboBoxFilterModel(QObject* parent = nullptr);

    void setSearchText(const QString& text);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
    QString m_searchText;
};

}  // namespace Gui
