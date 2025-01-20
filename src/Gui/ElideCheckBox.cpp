/***************************************************************************
 *   Copyright (c) 2025 Alfredo Monclus <alfredomonclus@gmail.com>         *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 51 Franklin Street,      *
 *   Fifth Floor, Boston, MA  02110-1301, USA                              *
 *                                                                         *
 ***************************************************************************/

// This custom widget adds the missing ellipsize functionality in QT5

#include "ElideCheckBox.h"

const int CheckboxSpacing = 25;

ElideCheckBox::ElideCheckBox(QWidget *parent)
    : QCheckBox(parent) {
}

void ElideCheckBox::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QStyleOptionButton option;
    option.initFrom(this);
    option.state = (isChecked() ? QStyle::State_On : QStyle::State_Off) |
                   (isEnabled() ? QStyle::State_Enabled : QStyle::State_ReadOnly);

    QPainter painter(this);
    style()->drawControl(QStyle::CE_CheckBox, &option, &painter, this);

    QRect textRect = option.rect;
    int padding = 4;
    textRect.setX(textRect.x() + 25);

    QFontMetrics fm(font());
    QString elidedText = fm.elidedText(text(), Qt::ElideRight, textRect.width() - padding);

    painter.setPen(palette().color(QPalette::WindowText));
    painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);
}

QSize ElideCheckBox::sizeHint() const {
    QFontMetrics fm(font());
    int width = fm.horizontalAdvance(this->text()) + CheckboxSpacing;
    int height = fm.height();
    return QSize(width, height);
}

QSize ElideCheckBox::minimumSizeHint() const {
    QFontMetrics fm(font());
    QString minimumText = QStringLiteral("A...");
    int width = fm.horizontalAdvance(minimumText) + CheckboxSpacing;
    int height = fm.height();
    return QSize(width, height);
}

