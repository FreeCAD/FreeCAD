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

#include "ElideLabel.h"

namespace Gui {

ElideLabel::ElideLabel(QWidget *parent)
    : QLabel(parent) {
}

void ElideLabel::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setPen(palette().color(QPalette::WindowText));
    painter.setFont(font());

    constexpr int padding = 4;
    QFontMetrics fm(painter.fontMetrics());

    int availableWidth = width() - padding * 2;
    if (availableWidth < 0) {
        return;
    }

    QString elidedText = fm.elidedText(text(), Qt::ElideRight, availableWidth);

    QRect textRect = rect().adjusted(padding, 0, -padding, 0);
    painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);
}

QSize ElideLabel::sizeHint() const {
    QFontMetrics fm(font());
    int width = fm.horizontalAdvance(this->text());
    int height = fm.height();
    return {width, height};
}

QSize ElideLabel::minimumSizeHint() const {
    QFontMetrics fm(font());
    QString minimumText = QStringLiteral("A...");
    int width = fm.horizontalAdvance(minimumText);
    int height = fm.height();
    return {width, height};
}

}  // namespace Gui

#include "moc_ElideLabel.cpp" // NOLINT
