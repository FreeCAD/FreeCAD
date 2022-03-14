/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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
#include <QPainter>
#include <QPainterPathStroker>
#include <QStyleOptionGraphicsItem>
#endif

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include <Mod/TechDraw/App/DrawUtil.h>

#include <qmath.h>
#include "Rez.h"
#include "DrawGuiUtil.h"
#include "PreferencesGui.h"
#include "QGIView.h"
#include "QGIHighlight.h"

using namespace TechDrawGui;
using namespace TechDraw;

QGIHighlight::QGIHighlight()
{
    m_refText = "";
    m_refSize = 0.0;
    setInteractive(false);

    m_circle = new QGraphicsEllipseItem();
    addToGroup(m_circle);
    m_circle->setFlag(QGraphicsItem::ItemIsSelectable, false);

    m_rect = new QGCustomRect();
    addToGroup(m_rect);
    m_rect->setFlag(QGraphicsItem::ItemIsSelectable, false);

    m_reference = new QGCustomText();
    addToGroup(m_reference);
    m_reference->setFlag(QGraphicsItem::ItemIsSelectable, false);

    setWidth(Rez::guiX(0.75));
}

QGIHighlight::~QGIHighlight()
{

}

void QGIHighlight::draw()
{
    prepareGeometryChange();
    makeHighlight();
    makeReference();
    update();
}

void QGIHighlight::makeHighlight()
{
    QRectF r(m_start,m_end);
    m_circle->setRect(r);
    m_rect->setRect(r);
    if (getHoleStyle() == 0) {
        m_rect->hide();
        m_circle->show();
    } else {
        m_rect->show();
        m_circle->hide();
    }
}

void QGIHighlight::makeReference()
{
    prepareGeometryChange();
    m_refFont.setPixelSize(QGIView::calculateFontPixelSize(m_refSize));
    m_reference->setFont(m_refFont);
    m_reference->setPlainText(QString::fromUtf8(m_refText));
    double fudge = Rez::guiX(1.0);
    QPointF newPos(m_end.x() + fudge, m_start.y() - m_refSize - fudge);
    m_reference->setPos(newPos);   

    double highRot = rotation();
    if (!TechDraw::DrawUtil::fpCompare(highRot,0.0)) {
        QRectF refBR = m_reference->boundingRect();
        QPointF refCenter = refBR.center();
        m_reference->setTransformOriginPoint(refCenter);
        m_reference->setRotation(-highRot);
    }
}

void QGIHighlight::setInteractive(bool state)
{
//    setAcceptHoverEvents(state);
    setFlag(QGraphicsItem::ItemIsSelectable, state);
    setFlag(QGraphicsItem::ItemIsMovable, state);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, state);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, state);
}

void QGIHighlight::setBounds(double x1,double y1,double x2,double y2)
{
    m_start = QPointF(Rez::guiX(x1),Rez::guiX(-y1));
    m_end = QPointF(Rez::guiX(x2),Rez::guiX(-y2));
}

void QGIHighlight::setReference(char* ref)
{
    m_refText = ref;
}

void QGIHighlight::setFont(QFont f, double fsize)
{
    m_refFont = f;
    m_refSize = fsize;
}


//obs?
QColor QGIHighlight::getHighlightColor()
{
    return PreferencesGui::sectionLineQColor();
}

//obs??
Qt::PenStyle QGIHighlight::getHighlightStyle()
{
    return PreferencesGui::sectionLineStyle();
}

int QGIHighlight::getHoleStyle()
{
    return TechDraw::Preferences::mattingStyle();
}

void QGIHighlight::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
//    myOption.state &= ~QStyle::State_Selected;

    setTools();
//    painter->drawRect(boundingRect());          //good for debugging
    QGIDecoration::paint (painter, &myOption, widget);
}

void QGIHighlight::setTools()
{
    m_pen.setWidthF(m_width);
    m_pen.setColor(m_colCurrent);
    m_pen.setStyle(m_styleCurrent);
    m_brush.setStyle(m_brushCurrent);
    m_brush.setColor(m_colCurrent);

    m_circle->setPen(m_pen);
    m_rect->setPen(m_pen);

    m_reference->setDefaultTextColor(m_colCurrent);
}

