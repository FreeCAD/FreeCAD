#pragma once

#include <QSortFilterProxyModel>

namespace Gui
{

class ComboBoxFilterModel: public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit ComboBoxFilterModel(QObject* parent = nullptr);

    void setSearchText(const QString& text);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
    QString m_searchText;
};

}  // namespace Gui
