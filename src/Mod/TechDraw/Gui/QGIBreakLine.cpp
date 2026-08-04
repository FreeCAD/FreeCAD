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

# include <QGraphicsScene>
# include <QGraphicsSceneMouseEvent>
# include <QKeyEvent>
# include <QPainter>
# include <QPainterPath>
# include <QStyleOptionGraphicsItem>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>
#include <vector>

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Tools.h>

#include <Mod/TechDraw/App/Preferences.h>
#include <Mod/TechDraw/App/DrawUtil.h>

#include "QGIBreakLine.h"
#include "PreferencesGui.h"
#include "Rez.h"

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

    m_background = new QGraphicsPathItem();
    addToGroup(m_background);
    m_line0 = new QGraphicsPathItem();
    addToGroup(m_line0);
    m_line1 = new QGraphicsPathItem();
    addToGroup(m_line1);
    m_background->setAcceptedMouseButtons(Qt::NoButton);
    m_line0->setAcceptedMouseButtons(Qt::NoButton);
    m_line1->setAcceptedMouseButtons(Qt::NoButton);


    setColor(PreferencesGui::sectionLineQColor());
    m_brush.setStyle(Qt::SolidPattern);
}

void QGIBreakLine::draw()
{
    if (m_genericGeometry) {
        drawGenericLines();
        update();
        return;
    }

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

    if (breakType() == DrawBrokenView::BreakType::SINUSOID) {
        const Base::Vector3d horizontal{1.0, 0.0, 0.0};
        const QRectF bounds(
            QPointF(m_left, m_bottom), QPointF(m_right, m_top));
        if (DU::fpCompare(
                std::fabs(m_direction.Dot(horizontal)), 1.0, EWTOLERANCE)) {
            setLineGeometry(
                QPointF(m_left, bounds.center().y()),
                QPointF(m_right, bounds.center().y()),
                QPointF(0.0, 1.0),
                bounds);
        }
        else {
            setLineGeometry(
                QPointF(bounds.center().x(), m_bottom),
                QPointF(bounds.center().x(), m_top),
                QPointF(1.0, 0.0),
                bounds);
        }
        drawGenericLines();
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
    QPainterPath background;
    background.addRect(backgroundRect);
    m_background->setPath(background);
}

void QGIBreakLine::setLineGeometry(const QPointF& first,
                                   const QPointF& second,
                                   const QPointF& tangent,
                                   const QRectF& bounds)
{
    m_firstPoint = first;
    m_secondPoint = second;
    m_tangent = tangent;
    const double length = std::hypot(m_tangent.x(), m_tangent.y());
    if (length > 1.0e-12) {
        m_tangent /= length;
    }
    m_clipBounds = bounds.normalized();
    m_genericGeometry = true;
}

std::optional<std::pair<QPointF, QPointF>>
QGIBreakLine::clippedLine(const QPointF& point, const QPointF& tangent) const
{
    constexpr double epsilon = 1.0e-9;
    std::vector<std::pair<double, QPointF>> hits;
    auto addHit = [&](double parameter) {
        const QPointF hit = point + tangent * parameter;
        if (hit.x() >= m_clipBounds.left() - epsilon
            && hit.x() <= m_clipBounds.right() + epsilon
            && hit.y() >= m_clipBounds.top() - epsilon
            && hit.y() <= m_clipBounds.bottom() + epsilon) {
            hits.emplace_back(parameter, hit);
        }
    };
    if (std::abs(tangent.x()) > epsilon) {
        addHit((m_clipBounds.left() - point.x()) / tangent.x());
        addHit((m_clipBounds.right() - point.x()) / tangent.x());
    }
    if (std::abs(tangent.y()) > epsilon) {
        addHit((m_clipBounds.top() - point.y()) / tangent.y());
        addHit((m_clipBounds.bottom() - point.y()) / tangent.y());
    }
    if (hits.size() < 2) {
        return std::nullopt;
    }
    std::sort(hits.begin(), hits.end(),
              [](const auto& left, const auto& right) {
                  return left.first < right.first;
              });
    if (std::hypot(hits.back().second.x() - hits.front().second.x(),
                   hits.back().second.y() - hits.front().second.y())
        <= epsilon) {
        return std::nullopt;
    }
    return std::pair{hits.front().second, hits.back().second};
}

QPainterPath QGIBreakLine::makeStyledLine(const QPointF& start,
                                          const QPointF& end,
                                          double outwardSign) const
{
    QPainterPath path(start);
    if (breakType() == BreakType::SIMPLE) {
        path.lineTo(end);
        return path;
    }

    QPointF tangent = end - start;
    const double length = std::hypot(tangent.x(), tangent.y());
    if (length <= 1.0e-9) {
        return path;
    }
    tangent /= length;
    const QPointF normal(-tangent.y(), tangent.x());
    const double amplitude = Rez::guiX(1.5);
    const int segments = breakType() == BreakType::SINUSOID ? 100 : 12;
    for (int segment = 1; segment <= segments; ++segment) {
        const double fraction =
            static_cast<double>(segment) / static_cast<double>(segments);
        double offset = 0.0;
        if (breakType() == BreakType::SINUSOID) {
            const double phase = fraction * 10.0 * std::numbers::pi;
            offset = outwardSign == 0.0
                ? amplitude * std::sin(phase)
                : outwardSign * amplitude * 0.5 * (1.0 - std::cos(phase));
        }
        else if (segment < segments) {
            offset = outwardSign == 0.0
                ? (segment % 2 == 0 ? -amplitude : amplitude)
                : (segment % 2 == 0 ? 0.0 : outwardSign * amplitude);
        }
        path.lineTo(start + tangent * (length * fraction) + normal * offset);
    }
    return path;
}

void QGIBreakLine::drawGenericLines()
{
    prepareGeometryChange();
    if (breakType() == BreakType::NONE) {
        m_background->hide();
        m_line0->hide();
        m_line1->hide();
        return;
    }

    const auto firstLine = clippedLine(m_firstPoint, m_tangent);
    const auto secondLine = clippedLine(m_secondPoint, m_tangent);
    if (!firstLine || !secondLine) {
        m_background->setPath({});
        m_line0->setPath({});
        m_line1->setPath({});
        m_background->hide();
        m_line0->hide();
        m_line1->hide();
        return;
    }

    const QPointF lineVector = firstLine->second - firstLine->first;
    const double lineLength = std::hypot(lineVector.x(), lineVector.y());
    const QPointF gapVector = m_secondPoint - m_firstPoint;
    const double gapLength = std::hypot(gapVector.x(), gapVector.y());
    double firstOutward = 0.0;
    double secondOutward = 0.0;
    if (lineLength > 1.0e-9 && gapLength > 1.0e-9) {
        const QPointF lineNormal(-lineVector.y() / lineLength,
                                 lineVector.x() / lineLength);
        const QPointF gapNormal = gapVector / gapLength;
        const double alignment =
            QPointF::dotProduct(lineNormal, gapNormal);
        firstOutward = alignment < 0.0 ? 1.0 : -1.0;
        secondOutward = -firstOutward;
    }

    const QPainterPath firstPath =
        makeStyledLine(firstLine->first, firstLine->second, firstOutward);
    const QPainterPath secondPath =
        makeStyledLine(secondLine->first, secondLine->second, secondOutward);
    m_line0->setPath(firstPath);
    m_line1->setPath(secondPath);

    // The page-coloured mask must have the same boundary as the visible
    // break lines.  A straight-sided mask leaves the HLR cut edge visible
    // underneath zigzag and sinusoidal decorations.
    QPainterPath background = firstPath;
    background.connectPath(secondPath.toReversed());
    background.closeSubpath();
    m_background->setPath(background);
    m_background->show();
    m_line0->show();
    m_line1->show();
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
    if (isSelected()) {
        QPen selectedPen = m_pen;
        selectedPen.setColor(prefSelectColor());
        m_line0->setPen(selectedPen);
        m_line1->setPen(selectedPen);
    }

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

void QGIBreakLine::setDeleteCallback(std::function<void()> callback)
{
    m_deleteCallback = std::move(callback);
    const bool selectable = static_cast<bool>(m_deleteCallback);
    setFlag(QGraphicsItem::ItemIsSelectable, selectable);
    setFlag(QGraphicsItem::ItemIsFocusable, selectable);
    setAcceptedMouseButtons(selectable ? Qt::LeftButton : Qt::NoButton);
    setCursor(selectable ? Qt::PointingHandCursor : Qt::ArrowCursor);
    if (selectable) {
        setToolTip(QObject::tr("Select and press Delete to remove this break"));
    }
}

void QGIBreakLine::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    QGIDecoration::mousePressEvent(event);
    if (m_deleteCallback) {
        setFocus();
        event->accept();
    }
}

void QGIBreakLine::keyPressEvent(QKeyEvent* event)
{
    if (m_deleteCallback
        && (event->key() == Qt::Key_Delete
            || event->key() == Qt::Key_Backspace)) {
        m_deleteCallback();
        event->accept();
        return;
    }
    QGraphicsItemGroup::keyPressEvent(event);
}


void QGIBreakLine::setLinePen(QPen isoPen)
{
    m_pen = isoPen;
}
