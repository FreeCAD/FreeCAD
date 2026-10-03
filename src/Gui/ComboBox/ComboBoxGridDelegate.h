#pragma once

#include <QStyledItemDelegate>
#include <QSize>

namespace Gui
{

class ComboBoxGridDelegate: public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit ComboBoxGridDelegate(QObject* parent = nullptr);

    void setItemSize(const QSize& size);
    QSize itemSize() const;

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    void paint(
        QPainter* painter,
        const QStyleOptionViewItem& option,
        const QModelIndex& index
    ) const override;

private:
    QSize m_itemSize {120, 42};
};

}  // namespace Gui
