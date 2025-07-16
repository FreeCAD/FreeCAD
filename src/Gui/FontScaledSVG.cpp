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

// This custom widget scales an svg according to fonts

#include "PreCompiled.h"

#include "FontScaledSVG.h"

namespace Gui {

FontScaledSVG::FontScaledSVG(QWidget *parent)
    : QWidget(parent), m_svgRenderer(new QSvgRenderer(this)) {
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void FontScaledSVG::setSvg(const QString &svgPath) {
    if (m_svgRenderer->load(svgPath)) {
        updateScaledSize();
        update();
    }
}

void FontScaledSVG::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);

    if (m_svgRenderer->isValid()) {
        QRect targetRect(0, 0, width(), height());
        m_svgRenderer->render(&painter, targetRect);
    }
}

void FontScaledSVG::resizeEvent(QResizeEvent *event) {
    Q_UNUSED(event);
    updateScaledSize();
}

void FontScaledSVG::updateScaledSize() {
    QSize baseSize = m_svgRenderer->defaultSize();

    QFontMetrics metrics(font());
    qreal spacing = metrics.lineSpacing();
    constexpr int baseFactor = 18;
    qreal scalingFactor = spacing / baseFactor;

    QSize targetSize = baseSize * scalingFactor;
    setFixedSize(targetSize);
}

}  // namespace Gui

#include "moc_FontScaledSVG.cpp" // NOLINT
