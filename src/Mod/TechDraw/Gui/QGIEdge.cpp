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


# include <QPainterPath>
# include <QPainterPathStroker>
# include <QStyleOptionGraphicsItem>

# include <cmath>


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
    return shape().controlPointRect();
}

QPainterPath QGIEdge::shape() const
{
    QPainterPath outline;
    QPainterPathStroker stroker;
    stroker.setWidth(this->m_edgeFuzz);
    outline = stroker.createStroke(path());
    return outline;
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

void QGIEdge::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;
    QPen pen(m_pen);

    if (m_source == TechDraw::SourceType::COSMETICEDGE ||
        m_source == TechDraw::SourceType::CENTERLINE) {
        
        QVector<qreal> pattern = pen.dashPattern();

        if (!pattern.isEmpty() && !path().isEmpty() && pen.widthF() > 0.0) {
            
            qreal length = 0.0;
            for (qreal line : pattern) {
                length += line;
            }

            if (length > 0.0) {
                qreal firstLineCenter = pattern.front() / 2.0;

                // The pattern is in terms of pen widths, so we also need to divide by the pen width
                qreal halfLength = path().length() / (2.0 * pen.widthF());

                qreal dashOffset = firstLineCenter - std::fmod(halfLength, length);
                
                if (dashOffset < 0.0) dashOffset += length;

                pen.setDashOffset(dashOffset);
            }
        }
    }

    setPen(pen);
    setBrush(m_brush);
    QGraphicsPathItem::paint(painter, &myOption, widget);
}

