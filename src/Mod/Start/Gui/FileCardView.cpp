// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2024 The FreeCAD Project Association AISBL               *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include "FileCardView.h"

#include <QScrollBar>
#include <QStyle>
#include <QStyleOption>

#include <App/Application.h>
#include "../App/DisplayedFilesModel.h"
#include <algorithm>

namespace StartGui
{

FileCardView::FileCardView(QWidget* parent)
    : QListView(parent)
{
    QSizePolicy sizePolicy(QSizePolicy::Policy::MinimumExpanding, QSizePolicy::Policy::MinimumExpanding);
    sizePolicy.setHeightForWidth(true);
    setSizePolicy(sizePolicy);
    setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAsNeeded);
    setViewMode(QListView::ViewMode::IconMode);
    setFlow(QListView::Flow::LeftToRight);
    setResizeMode(QListView::ResizeMode::Adjust);
    setUniformItemSizes(true);
    setMouseTracking(true);

    auto hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Start"
    );
    m_cardSpacing = static_cast<int>(hGrp->GetInt("FileCardSpacing", 16));  // NOLINT

    setSpacing(m_cardSpacing);
}

// Mirrors the layout bounds reduction in QListViewPrivate::prepareItemsLayout
int FileCardView::reservedScrollBarWidth() const
{
    const auto* scrollBar = verticalScrollBar();
    if (style()->pixelMetric(QStyle::PM_ScrollView_ScrollBarOverlap, nullptr, scrollBar) != 0) {
        return 0;
    }
    int frameAroundContents = 0;
    if (style()->styleHint(QStyle::SH_ScrollView_FrameOnlyAroundContents) != 0) {
        QStyleOption option;
        option.initFrom(this);
        frameAroundContents = 2 * style()->pixelMetric(QStyle::PM_DefaultFrameWidth, &option);
    }
    return style()->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, scrollBar) + frameAroundContents;
}

int FileCardView::heightForWidth(int width) const
{
    auto model = this->model();
    auto delegate = this->itemDelegate();
    if (!model || !delegate) {
        return 0;
    }
    int numCards = model->rowCount();
    auto cardSize = delegate->sizeHint(QStyleOptionViewItem(), model->index(0, 0));
    // Each row starts after one spacing; the card pitch is cardWidth + spacing
    const int usableWidth = width - 2 * frameWidth() - reservedScrollBarWidth() - m_cardSpacing;
    const int cardsPerRow = std::max(1, usableWidth / (cardSize.width() + m_cardSpacing));
    int numRows = static_cast<int>(
        ceil(static_cast<double>(numCards) / static_cast<double>(cardsPerRow))
    );
    int neededHeight = numRows * cardSize.height();
    constexpr int extra = 4;  // avoid tiny scrollbars
    return neededHeight + m_cardSpacing * (numRows - 1) + 2 * m_cardSpacing + extra;
}

QSize FileCardView::sizeHint() const
{
    auto model = this->model();
    auto delegate = this->itemDelegate();
    if (!model || !delegate) {
        // The model and/or delegate have not been set yet, this was an early startup call
        return {m_cardSpacing, m_cardSpacing};
    }
    int numCards = model->rowCount();
    auto cardSize = delegate->sizeHint(QStyleOptionViewItem(), model->index(0, 0));
    return {
        (cardSize.width() + m_cardSpacing) * numCards + m_cardSpacing,
        cardSize.height() + 2 * m_cardSpacing
    };
}

}  // namespace StartGui
