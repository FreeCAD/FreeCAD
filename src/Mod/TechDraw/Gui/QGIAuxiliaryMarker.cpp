/***************************************************************************
 *   Copyright (c) 2026 FreeCAD contributors                               *
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
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include <algorithm>

#include <QPainterPath>

#include "PreferencesGui.h"
#include "QGCustomText.h"
#include "QGIAuxiliaryMarker.h"
#include "QGIArrow.h"
#include "Rez.h"
#include "ZVALUE.h"

using namespace TechDrawGui;

QGIAuxiliaryMarker::QGIAuxiliaryMarker()
    : m_line(new QGraphicsPathItem())
    , m_arrow1(new QGIArrow())
    , m_arrow2(new QGIArrow())
    , m_label1(new QGCustomText())
    , m_label2(new QGCustomText())
    , m_start(0.0, 0.0)
    , m_end(0.0, 0.0)
    , m_direction(1.0, 0.0, 0.0)
    , m_labelText(QStringLiteral("A"))
    , m_font(PreferencesGui::labelFontQFont())
    , m_arrowSize(QGIArrow::getPrefArrowSize())
    , m_color(PreferencesGui::normalQColor())
{
    addToGroup(m_line);
    addToGroup(m_arrow1);
    addToGroup(m_arrow2);
    addToGroup(m_label1);
    addToGroup(m_label2);

    m_font.setPixelSize(std::max(1, PreferencesGui::labelFontSizePX()));
    setWidth(Rez::guiX(0.35));
    setColor(m_color);
}

void QGIAuxiliaryMarker::setEnds(const Base::Vector3d& start, const Base::Vector3d& end)
{
    m_start = QPointF(start.x, start.y);
    m_end = QPointF(end.x, end.y);
}

void QGIAuxiliaryMarker::setDirection(const Base::Vector3d& direction)
{
    Base::Vector3d qtDirection(direction.x, -direction.y, 0.0);
    if (qtDirection.Length() < 1.0e-12) {
        qtDirection = Base::Vector3d(1.0, 0.0, 0.0);
    }
    qtDirection.Normalize();
    m_direction = qtDirection;
}

void QGIAuxiliaryMarker::setLabel(const QString& label)
{
    m_labelText = label;
}

void QGIAuxiliaryMarker::setFont(const QFont& font)
{
    m_font = font;
}

void QGIAuxiliaryMarker::setArrowSize(double arrowSize)
{
    m_arrowSize = arrowSize;
}

void QGIAuxiliaryMarker::setLinePen(const QPen& pen)
{
    m_pen = pen;
}

void QGIAuxiliaryMarker::setMarkerColor(const QColor& color)
{
    m_color = color;
    setColor(color);
}

void QGIAuxiliaryMarker::draw()
{
    prepareGeometryChange();
    drawReferenceLine();
    drawArrow(m_arrow1, m_start);
    drawArrow(m_arrow2, m_end);
    drawLabel(m_label1, labelPosition(m_start));
    drawLabel(m_label2, labelPosition(m_end));
    update();
}

void QGIAuxiliaryMarker::drawReferenceLine()
{
    QPainterPath path;
    path.moveTo(m_start);
    path.lineTo(m_end);

    m_line->setPath(path);
    m_line->setPen(m_pen);
    m_line->setZValue(ZVALUE::SECTIONLINE);
}

void QGIAuxiliaryMarker::drawArrow(QGIArrow* arrow, const QPointF& position)
{
    if (!arrow) {
        return;
    }

    arrow->setStyle(TechDraw::ArrowType::FILLED_ARROW);
    arrow->setDirMode(true);
    arrow->setDirection(m_direction);
    arrow->setSize(m_arrowSize);
    arrow->setNormalColor(m_color);
    arrow->setFillColor(m_color);
    arrow->setPos(position);
    arrow->setZValue(ZVALUE::SECTIONLINE + 1);
    arrow->draw();
}

void QGIAuxiliaryMarker::drawLabel(QGCustomText* label, const QPointF& position)
{
    if (!label) {
        return;
    }

    label->setFont(m_font);
    label->setPlainText(m_labelText);
    label->setColor(m_color);
    label->centerAt(position);
    label->setTransformOriginPoint(label->mapFromParent(position));
    label->setRotation(360.0 - rotation());
    label->setZValue(ZVALUE::SECTIONLINE + 2);
}

QPointF QGIAuxiliaryMarker::labelPosition(const QPointF& reference) const
{
    const double fontGap = std::max(1, m_font.pixelSize());
    const double gap = std::max(fontGap, Rez::guiX(m_arrowSize));
    return reference + QPointF(m_direction.x * gap, m_direction.y * gap);
}
