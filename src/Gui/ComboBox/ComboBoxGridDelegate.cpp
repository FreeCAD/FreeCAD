#include "ComboBoxGridDelegate.h"

#include <QApplication>
#include <QPainter>
#include <QStyle>

using namespace Gui;

ComboBoxGridDelegate::ComboBoxGridDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{}

void ComboBoxGridDelegate::setItemSize(const QSize& size)
{
    if (size.width() <= 0 || size.height() <= 0) {
        return;
    }

    m_itemSize = size;
}

QSize ComboBoxGridDelegate::itemSize() const
{
    return m_itemSize;
}

QSize ComboBoxGridDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const
{
    return m_itemSize;
}

void ComboBoxGridDelegate::paint(
    QPainter* painter,
    const QStyleOptionViewItem& option,
    const QModelIndex& index
) const
{
    QStyleOptionViewItem opt = option;

    initStyleOption(&opt, index);

    QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();

    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);
}
