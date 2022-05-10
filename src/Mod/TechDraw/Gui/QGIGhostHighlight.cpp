/***************************************************************************
 *   Copyright (c) 2020 WandererFan <wandererfan@gmail.com>                *
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
#include <QGraphicsSceneEvent>
#include <QPainter>
#include <QPainterPathStroker>
#include <QStyleOptionGraphicsItem>
#include <QPen>
#include <QColor>
#endif

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include <Mod/TechDraw/App/DrawUtil.h>
//#include <Mod/TechDraw/App/Preferences.h>

#include <qmath.h>
#include "Rez.h"
#include "DrawGuiUtil.h"
#include "PreferencesGui.h"
#include "QGIView.h"
#include "QGIGhostHighlight.h"

using namespace TechDrawGui;
using namespace TechDraw;

QGIGhostHighlight::QGIGhostHighlight()
{
    setInteractive(true);
    m_dragging = false;

    //make the ghost very visible
    QFont f(QGIView::getPrefFont());
    double fontSize = Preferences::labelFontSizeMM();
    setFont(f, fontSize);
    setReference("drag");
    setStyle(Qt::SolidLine);
    setColor(prefSelectColor());
    setWidth(Rez::guiX(1.0));
    setRadius(10.0);         //placeholder
}

QGIGhostHighlight::~QGIGhostHighlight()
{

}

QVariant QGIGhostHighlight::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionHasChanged && scene()) {
        // nothing to do here?
    }
    return QGIHighlight::itemChange(change, value);
}

void QGIGhostHighlight::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
//    Base::Console().Message("QGIGhostHighlight::mousePress() - %X\n", this);
    if ( (event->button() == Qt::LeftButton) && 
        (flags() & QGraphicsItem::ItemIsMovable) ) {
            m_dragging = true;
            event->accept();
    }
    QGIHighlight::mousePressEvent(event);
}

void QGIGhostHighlight::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
//    Base::Console().Message("QGIGhostHighlight::mouseRelease() - pos: %s scenePos: %s\n", 
//                                 DrawUtil::formatVector(pos()).c_str(),
//                                 DrawUtil::formatVector(mapToScene(pos())).c_str());
    if (m_dragging) {
        m_dragging = false;
        Q_EMIT positionChange(scenePos());
        event->accept();
    }
    QGIHighlight::mouseReleaseEvent(event);
}

void QGIGhostHighlight::setInteractive(bool state)
{
    setFlag(QGraphicsItem::ItemIsSelectable, state);
    setFlag(QGraphicsItem::ItemIsMovable, state);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, state);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, state);
}

//radius should scaled, but not Rez::guix()
void QGIGhostHighlight::setRadius(double r)
{
    setBounds(-r, r, r, -r);
}

#include <Mod/TechDraw/Gui/moc_QGIGhostHighlight.cpp>
