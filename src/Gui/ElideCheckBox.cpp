// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Alfredo Monclus <alfredomonclus@gmail.com>          *
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

// This custom widget adds the missing ellipsize functionality in QT5

#include "PreCompiled.h"

#include "ElideCheckBox.h"

namespace Gui {

ElideCheckBox::ElideCheckBox(QWidget *parent)
    : QCheckBox(parent) {
}

void ElideCheckBox::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QStyleOptionButton option;
    option.initFrom(this);

    option.state = (isChecked() ? QStyle::State_On : QStyle::State_Off) |
                   (isEnabled() ? QStyle::State_Enabled : QStyle::State_ReadOnly) |
                   (underMouse() ? QStyle::State_MouseOver : QStyle::State_None) |
                   (hasFocus() ? QStyle::State_HasFocus : QStyle::State_None);

    QPainter painter(this);

    style()->drawControl(QStyle::CE_CheckBox, &option, &painter, this);

    QRect textRect = style()->subElementRect(QStyle::SE_CheckBoxContents, &option, this);

    QFontMetrics fm(font());
    QString elidedText = fm.elidedText(text(), Qt::ElideRight, textRect.width());

    painter.setPen(palette().color(QPalette::WindowText));
    painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);
}

QSize ElideCheckBox::sizeHint() const {
    QFontMetrics fm(font());
    int width =
        fm.horizontalAdvance(this->text())
        + style()->pixelMetric(QStyle::PM_IndicatorWidth, nullptr, this)
        + style()->pixelMetric(QStyle::PM_CheckBoxLabelSpacing, nullptr, this);
    int height = fm.height();
    return {width, height};
}

QSize ElideCheckBox::minimumSizeHint() const {
    QFontMetrics fm(font());
    QString minimumText = QStringLiteral("A...");
    int width =
        fm.horizontalAdvance(minimumText)
        + style()->pixelMetric(QStyle::PM_IndicatorWidth, nullptr, this)
        + style()->pixelMetric(QStyle::PM_CheckBoxLabelSpacing, nullptr, this);
    int height = fm.height();
    return {width, height};
}

}  // namespace Gui

#include "moc_ElideCheckBox.cpp" // NOLINT
