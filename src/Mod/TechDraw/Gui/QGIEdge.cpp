/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
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


# include <QPainter>
# include <QPainterPath>
# include <QPainterPathStroker>


#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Gui/Control.h>
#include <Mod/TechDraw/App/DrawUtil.h>

#include "QGIEdge.h"
#include "PreferencesGui.h"
#include "TaskLineDecor.h"
#include "QGIView.h"

using namespace TechDrawGui;
using namespace TechDraw;

QGIEdge::QGIEdge(int index) :
    projIndex(index),
    isCosmetic(false),
    isHiddenEdge(false),
    isSmoothEdge(false)
{
    setFlag(QGraphicsItem::ItemIsFocusable, true);      // to get key press events
    setFlag(QGraphicsItem::ItemIsSelectable, true);

    setWidth(1.0);
    setCosmetic(isCosmetic);
    setFill(Qt::NoBrush);
}

// NOTE this refers to Qt cosmetic lines (a line with minimum width),
// not FreeCAD cosmetic lines
void QGIEdge::setCosmetic(bool state)
{
    isCosmetic = state;
    if (state) {
        setWidth(0.0);
    }
}

void QGIEdge::setHiddenEdge(bool b) {
    isHiddenEdge = b;
}

void QGIEdge::setPrettyNormal() {
    if (isHiddenEdge) {
        m_pen.setColor(getHiddenColor());
        return;
    }
    QGIPrimPath::setPrettyNormal();
}

QColor QGIEdge::getHiddenColor()
{
    Base::Color fcColor = Base::Color((uint32_t) Preferences::getPreferenceGroup("Colors")->GetUnsigned("HiddenColor", 0x000000FF));
    return PreferencesGui::getAccessibleQColor(fcColor.asValue<QColor>());
}

QRectF QGIEdge::boundingRect() const
{
    // Curved paths can have control points far outside the painted curve.
    return shape().boundingRect();
}

QPainterPath QGIEdge::shape() const
{
    const QPainterPath edgeShape = unclippedShape();
    if (m_paintClip.isEmpty()) {
        return edgeShape;
    }

    // Keep Bezier control points out of the Boolean operation. Intersecting
    // two polygonal envelopes gives a stable, tight bound at diagonal
    // partial-section boundaries.
    QPainterPath edgeEnvelope;
    edgeEnvelope.addRect(edgeShape.boundingRect());
    return edgeEnvelope.intersected(m_paintClip);
}

QPainterPath QGIEdge::unclippedShape() const
{
    QPainterPath outline;
    QPainterPathStroker stroker;
    stroker.setWidth(this->m_edgeFuzz);
    outline = stroker.createStroke(path());
    return outline;
}

bool QGIEdge::contains(const QPointF& point) const
{
    return (m_paintClip.isEmpty() || m_paintClip.contains(point))
        && unclippedShape().contains(point);
}

void QGIEdge::paint(QPainter* painter,
                    const QStyleOptionGraphicsItem* option,
                    QWidget* widget)
{
    painter->save();
    if (!m_paintClip.isEmpty()) {
        painter->setClipPath(m_paintClip, Qt::IntersectClip);
    }
    QGIPrimPath::paint(painter, option, widget);
    painter->restore();
}

void QGIEdge::setPaintClip(const QPainterPath& clip)
{
    prepareGeometryChange();
    m_paintClip = clip;
}

void QGIEdge::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event)
    auto* parent = dynamic_cast<QGIView *>(parentItem());
    if (parent && parent->getViewObject() && parent->getViewObject()->isDerivedFrom<TechDraw::DrawViewPart>()) {
        auto* baseFeat = static_cast<TechDraw::DrawViewPart *>(parent->getViewObject());
        std::vector<std::string> edgeName(1, DrawUtil::makeGeomName("Edge", getProjIndex()));

        Gui::Control().showDialog(new TaskDlgLineDecor(baseFeat, edgeName));
    }
}

void QGIEdge::setLinePen(const QPen& linePen)
{
    m_pen = linePen;
}

