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
# include <QGraphicsSceneEvent>
#endif

#include <Mod/TechDraw/App/DrawUtil.h>

#include "QGIGhostHighlight.h"
#include "PreferencesGui.h"
#include "Rez.h"


using namespace TechDrawGui;
using namespace TechDraw;

QGIGhostHighlight::QGIGhostHighlight()
{
    setInteractive(true);
    m_dragging = false;

    //make the ghost very visible
    QFont f(Preferences::labelFontQString());
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
