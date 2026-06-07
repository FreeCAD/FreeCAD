// SPDX-License-Identifier: LGPL-2.0-or-later

/***************************************************************************
 *   Copyright (c) 2026 meaqua9420                                        *
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

#include <QPainterPath>

#include <Mod/TechDraw/App/ArrowPropEnum.h>

#include "QGIAuxiliaryMarker.h"
#include "QGIArrow.h"
#include "QGCustomText.h"
#include "QGIView.h"
#include "Rez.h"


using namespace TechDrawGui;

QGIAuxiliaryMarker::QGIAuxiliaryMarker()
    : m_line(new QGraphicsPathItem())
    , m_arrow(new QGIArrow())
    , m_label(new QGCustomText())
    , m_start(0.0, 0.0)
    , m_end(0.0, 0.0)
    , m_direction(1.0, 0.0, 0.0)
    , m_fontSize(0.0)
    , m_arrowSize(QGIArrow::getPrefArrowSize())
{
    addToGroup(m_line);
    addToGroup(m_arrow);
    addToGroup(m_label);

    m_linePen.setWidthF(Rez::guiX(0.35));
    m_linePen.setColor(getColor());
    m_linePen.setStyle(Qt::SolidLine);
    m_linePen.setCapStyle(Qt::FlatCap);
}

void QGIAuxiliaryMarker::setEnds(Base::Vector3d start, Base::Vector3d end)
{
    m_start = QPointF(start.x, start.y);
    m_end = QPointF(end.x, end.y);
}

void QGIAuxiliaryMarker::setDirection(Base::Vector3d direction)
{
    if (direction.Length() < 1.0e-7) {
        direction = Base::Vector3d(1.0, 0.0, 0.0);
    }

    direction.z = 0.0;
    direction.Normalize();
    m_direction = direction;
}

void QGIAuxiliaryMarker::setLabel(const char* label)
{
    m_labelText = label ? label : "";
}

void QGIAuxiliaryMarker::setFont(QFont font, double size)
{
    m_font = font;
    m_fontSize = size;
}

void QGIAuxiliaryMarker::setArrowSize(double arrowSize)
{
    m_arrowSize = arrowSize;
}

void QGIAuxiliaryMarker::setLinePen(const QPen& linePen)
{
    m_linePen = linePen;
}

void QGIAuxiliaryMarker::draw()
{
    prepareGeometryChange();

    QPainterPath path;
    path.moveTo(m_start);
    path.lineTo(m_end);
    m_line->setPen(m_linePen);
    m_line->setPath(path);

    QPointF arrowPos = (m_start + m_end) / 2.0;
    m_arrow->setStyle(TechDraw::ArrowType::FILLED_ARROW);
    m_arrow->setSize(m_arrowSize);
    m_arrow->setDirMode(true);
    m_arrow->setDirection(m_direction);
    m_arrow->setPos(arrowPos);
    m_arrow->draw();

    if (m_fontSize >= 0.0) {
        m_font.setPixelSize(QGIView::exactFontSize(m_font.family().toStdString(), m_fontSize));
    }
    m_label->setFont(m_font);
    m_label->setPlainText(QString::fromUtf8(m_labelText.c_str()));
    m_label->setColor(m_linePen.color());

    QRectF labelRect = m_label->boundingRect();
    double labelGap = Rez::guiX(m_arrowSize) + 0.5 * labelRect.height();
    QPointF labelMotion(m_direction.x, m_direction.y);
    QPointF labelPos = arrowPos + labelMotion * labelGap;
    m_label->centerAt(labelPos);

    update();
}
