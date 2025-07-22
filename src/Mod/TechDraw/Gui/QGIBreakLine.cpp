// SPDX-License-Identifier: LGPL-2.0-or-later

/***************************************************************************
 *   Copyright (c) 2024 WandererFan <wandererfan@gmail.com>                *
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

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QGraphicsScene>
# include <QPainter>
# include <QPainterPath>
# include <QStyleOptionGraphicsItem>
#endif

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Tools.h>

#include <Mod/TechDraw/App/Preferences.h>
#include <Mod/TechDraw/App/DrawUtil.h>

#include "QGIBreakLine.h"
#include "PreferencesGui.h"

using namespace TechDrawGui;
using namespace TechDraw;

using DU = DrawUtil;

constexpr double zigzagWidth{30.0};
constexpr double segments{8};

QGIBreakLine::QGIBreakLine()
{
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    m_background = new QGraphicsRectItem();
    addToGroup(m_background);
    m_line0 = new QGraphicsPathItem();
    addToGroup(m_line0);
    m_line1 = new QGraphicsPathItem();
    addToGroup(m_line1);


    setColor(PreferencesGui::sectionLineQColor());
    m_brush.setStyle(Qt::SolidPattern);
}

void QGIBreakLine::draw()
{
    if (breakType() == DrawBrokenView::BreakType::NONE) {
        // none
        m_background->hide();
        m_line0->hide();
        m_line1->hide();
    }

    if (breakType() == DrawBrokenView::BreakType::ZIGZAG) {
        drawLargeZigZag();
        m_background->show();
        m_line0->show();
        m_line1->show();
    }

    if (breakType() == DrawBrokenView::BreakType::SIMPLE) {
        // simple line from pref
        drawSimpleLines();
        m_background->hide();
        m_line0->show();
        m_line1->show();
    }

    update();
}

void QGIBreakLine::drawLargeZigZag()
{
    Base::Vector3d horizontal{1.0, 0.0, 0.0};
    prepareGeometryChange();
    double offset = zigzagWidth / 2.0;
    if (DU::fpCompare(fabs(m_direction.Dot(horizontal)), 1.0, EWTOLERANCE)) {
        // m_direction connects the two cut points.  The zigzags have
        // to be perpendicular to m_direction
        // 2x vertical zigzag
        Base::Vector3d start = Base::Vector3d(m_left - offset, m_bottom, 0.0);
        m_line0->setPath(makeVerticalZigZag(start));

        start = Base::Vector3d(m_right - offset, m_bottom, 0.0);
        m_line1->setPath(makeVerticalZigZag(start));
    } else {
        // m_top is lower than m_bottom due to Qt Y+ down coords
        // the higher break line
        // 2x horizontal zigszags
        Base::Vector3d start = Base::Vector3d(m_left, m_bottom - offset, 0.0);
        m_line0->setPath(makeHorizontalZigZag(start));

        // the lower break line
        start = Base::Vector3d(m_left, m_top - offset, 0.0);
        m_line1->setPath(makeHorizontalZigZag(start));
    }

    QRectF backgroundRect(m_left - offset, m_bottom - offset,
                          std::fabs(m_right - m_left + zigzagWidth),
                          std::fabs(m_top - m_bottom + zigzagWidth));
    m_background->setRect(backgroundRect);
}

// start needs to be Rez'd and +Y up
QPainterPath QGIBreakLine::makeHorizontalZigZag(Base::Vector3d start) const
{
    // Base::Console().message("QGIBL::makeHorizontalZigZag(%s)\n", DU::formatVector(start).c_str());
    QPainterPath pPath;
    double step = (m_right - m_left) / segments;
    Base::Vector3d xOffset = Base::Vector3d(step, 0.0, 0.0);        // 1/2 wave length
    Base::Vector3d yOffset = Base::Vector3d(0.0, zigzagWidth, 0.0); // amplitude

    pPath.moveTo(DU::toQPointF(start));
    Base::Vector3d current = start;
    int iSegment = 0;
    double flipflop = 1.0;
    for (; iSegment < segments; iSegment++) {
        current = current + xOffset;
        current = current + yOffset * flipflop;
        pPath.lineTo(DU::toQPointF(current));
        flipflop *= -1.0;
    }
    return pPath;
}

QPainterPath QGIBreakLine::makeVerticalZigZag(Base::Vector3d start) const
{
    // Base::Console().message("QGIBL::makeVerticalZigZag(%s)\n", DU::formatVector(start).c_str());
    QPainterPath pPath;
    double step = (m_top - m_bottom) / segments;
    Base::Vector3d xOffset = Base::Vector3d(zigzagWidth, 0.0, 0.0);  // amplitude
    Base::Vector3d yOffset = Base::Vector3d(0.0, step, 0.0);        // 1/2 wave length

    pPath.moveTo(DU::toQPointF(start));
    Base::Vector3d current = start;
    int iSegment = 0;
    double flipflop = 1.0;
    for (; iSegment < segments; iSegment++) {
        current = current + xOffset * flipflop;
        current = current + yOffset;
        pPath.lineTo(DU::toQPointF(current));
        flipflop *= -1.0;
    }
    return pPath;
}


void QGIBreakLine::drawSimpleLines()
{
    Base::Vector3d horizontal{1.0, 0.0, 0.0};
    prepareGeometryChange();
    if (DU::fpCompare(fabs(m_direction.Dot(horizontal)), 1.0, EWTOLERANCE)) {
        // m_direction connects the two cut points.  The break lines have
        // to be perpendicular to m_direction
        Base::Vector3d start = Base::Vector3d(m_left, m_bottom, 0.0);
        Base::Vector3d end   = Base::Vector3d(m_left, m_top, 0.0);
        m_line0->setPath(pathFromPoints(start, end));

        start = Base::Vector3d(m_right, m_bottom, 0.0);
        end   = Base::Vector3d(m_right, m_top, 0.0);
        m_line1->setPath(pathFromPoints(start, end));
    } else {
        // m_top is lower than m_bottom due to Qt Y+ down coords
        // the higher break line
        // 2x horizontal zigszags
        Base::Vector3d start = Base::Vector3d(m_left, m_bottom, 0.0);
        Base::Vector3d end   = Base::Vector3d(m_right, m_bottom, 0.0);
        m_line0->setPath(pathFromPoints(start, end));

        // the lower break line
        start = Base::Vector3d(m_left, m_top, 0.0);
        end   = Base::Vector3d(m_right, m_top, 0.0);
        m_line1->setPath(pathFromPoints(start, end));
    }
}

QPainterPath QGIBreakLine::pathFromPoints(Base::Vector3d start, Base::Vector3d end)
{
    QPainterPath result(DU::toQPointF(start));
    result.lineTo(DU::toQPointF(end));
    return result;
}


void QGIBreakLine::setBounds(double left, double top, double right, double bottom)
{
    // Base::Console().message("QGIBL::setBounds(%.3f, %.3f, %.3f, %.3f\n", left, top, right, bottom);
    m_left = left;
    m_right = right;
    m_top = top;
    m_bottom = bottom;
}

void QGIBreakLine::setBounds(Base::Vector3d topLeft, Base::Vector3d bottomRight)
{
    double left = std::min(topLeft.x, bottomRight.x);
    double right = std::max(topLeft.x, bottomRight.x);
    double bottom = std::min(topLeft.y, bottomRight.y);
    double top = std::max(topLeft.y, bottomRight.y);

    setBounds(left, top, right, bottom);
}

void QGIBreakLine::setDirection(Base::Vector3d dir)
{
    m_direction = dir;
}


void QGIBreakLine::setBreakColor(QColor c)
{
    setColor(c);
}

void QGIBreakLine::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    setTools();

    // painter->setPen(Qt::blue);
    // painter->drawRect(boundingRect());          //good for debugging

    QGIDecoration::paint (painter, &myOption, widget);
}

void QGIBreakLine::setTools()
{
    m_brush.setColor(PreferencesGui::pageQColor());

    m_line0->setPen(m_pen);
    m_line0->setBrush(Qt::NoBrush);
    m_line1->setPen(m_pen);
    m_line1->setBrush(Qt::NoBrush);

    m_background->setBrush(m_brush);
    m_background->setPen(Qt::NoPen);
}


void QGIBreakLine::setLinePen(QPen isoPen)
{
    m_pen = isoPen;
}

