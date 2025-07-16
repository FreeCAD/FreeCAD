// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"
#include <QWidget>
#include "FlowLayout.h"

using namespace StartGui;

FlowLayout::FlowLayout(QWidget* parent, int margin, int hSpacing, int vSpacing)
    : QLayout(parent)
    , hSpace {hSpacing}
    , vSpace {vSpacing}
{
    setContentsMargins(margin, margin, margin, margin);
}

FlowLayout::~FlowLayout()
{
    QLayoutItem* item {};
    while ((item = takeAt(0))) {
        delete item;
    }
}

void FlowLayout::addItem(QLayoutItem* item)
{
    itemList.append(item);
}

int FlowLayout::horizontalSpacing() const
{
    if (hSpace >= 0) {
        return hSpace;
    }

    return smartSpacing(QStyle::PM_LayoutHorizontalSpacing);
}

int FlowLayout::verticalSpacing() const
{
    if (vSpace >= 0) {
        return vSpace;
    }

    return smartSpacing(QStyle::PM_LayoutVerticalSpacing);
}

int FlowLayout::count() const
{
    return itemList.size();
}

QLayoutItem* FlowLayout::itemAt(int index) const
{
    if (index >= 0 && index < itemList.size()) {
        return itemList[index];
    }

    return nullptr;
}

QLayoutItem* FlowLayout::takeAt(int index)
{
    if (index >= 0 && index < itemList.size()) {
        return itemList.takeAt(index);
    }

    return nullptr;
}

Qt::Orientations FlowLayout::expandingDirections() const
{
    return {};
}

bool FlowLayout::hasHeightForWidth() const
{
    return true;
}

int FlowLayout::heightForWidth(int width) const
{
    int height = doLayout(QRect(0, 0, width, 0), true);
    return height;
}

void FlowLayout::setGeometry(const QRect& rect)
{
    QLayout::setGeometry(rect);
    doLayout(rect, false);
}

QSize FlowLayout::sizeHint() const
{
    return minimumSize();
}

QSize FlowLayout::minimumSize() const
{
    QSize size;
    for (auto item : std::as_const(itemList)) {
        size = size.expandedTo(item->minimumSize());
    }

    QMargins margins = contentsMargins();
    size += QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
    return size;
}

int FlowLayout::smartSpacing(QStyle::PixelMetric pm) const
{
    QObject* par = parent();
    if (!par) {
        return -1;
    }

    if (par->isWidgetType()) {
        auto widget = qobject_cast<QWidget*>(par);
        return widget->style()->pixelMetric(pm, nullptr, widget);
    }

    return static_cast<QLayout*>(par)->spacing();
}

int FlowLayout::doLayout(const QRect& rect, bool testOnly) const
{
    int left {};
    int top {};
    int right {};
    int bottom {};
    getContentsMargins(&left, &top, &right, &bottom);
    QRect effectiveRect = rect.adjusted(+left, +top, -right, -bottom);

    int x = effectiveRect.x();
    int y = effectiveRect.y();
    int lineHeight = 0;

    QVector<QLayoutItem*> currentRow;

    for (auto item : std::as_const(itemList)) {
        QWidget* wid = item->widget();
        QSize itemSizeHint = item->sizeHint();

        int spaceX = horizontalSpacing();
        if (spaceX == -1) {
            spaceX = wid->style()->layoutSpacing(QSizePolicy::PushButton,
                                                 QSizePolicy::PushButton,
                                                 Qt::Horizontal);
        }

        int spaceY = verticalSpacing();
        if (spaceY == -1) {
            spaceY = wid->style()->layoutSpacing(QSizePolicy::PushButton,
                                                 QSizePolicy::PushButton,
                                                 Qt::Vertical);
        }

        int nextX = x + itemSizeHint.width() + spaceX;

        // Step 1: Wrap if necessary
        if (nextX - spaceX > effectiveRect.right() && !currentRow.isEmpty()) {
            // Apply row height to all items in the current row
            for (auto rowItem : currentRow) {
                if (!testOnly) {
                    rowItem->setGeometry(QRect(QPoint(rowItem->geometry().x(), y),
                                               QSize(rowItem->sizeHint().width(), lineHeight)));
                }
            }

            // Move to next row
            y += lineHeight + spaceY;
            x = effectiveRect.x();
            currentRow.clear();
        }

        currentRow.append(item);
        lineHeight = std::max(lineHeight, itemSizeHint.height());

        if (!testOnly) {
            item->setGeometry(QRect(QPoint(x, y), QSize(itemSizeHint.width(), lineHeight)));
        }

        x += itemSizeHint.width() + spaceX;
    }

    // Step 2: Apply the last row's height
    for (auto rowItem : currentRow) {
        if (!testOnly) {
            rowItem->setGeometry(QRect(QPoint(rowItem->geometry().x(), y),
                                       QSize(rowItem->sizeHint().width(), lineHeight)));
        }
    }

    return y + lineHeight - rect.y() + bottom;
}

#include "moc_FlowLayout.cpp"
