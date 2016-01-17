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

#include "PreCompiled.h"
#ifndef _PreComp_
#include <cmath>
#include <QAction>
#include <QApplication>
#include <QContextMenuEvent>
#include <QGraphicsScene>
#include <QMenu>
#include <QMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QPainterPathStroker>
#include <QPainter>
#include <QTextOption>
#include <strstream>
#endif

#include <qmath.h>

#include <Base/Console.h>

#include "../App/DrawViewSection.h"
#include "QGIViewSection.h"

using namespace TechDrawGui;

QGIViewSection::QGIViewSection(const QPoint &pos, QGraphicsScene *scene) :QGIViewPart(pos, scene)
{
}

QGIViewSection::~QGIViewSection()
{

}

void QGIViewSection::draw()
{
    QGIViewPart::draw();
    drawSectionFace();
}

void QGIViewSection::drawSectionFace()
{
    if(getViewObject() == 0 || !getViewObject()->isDerivedFrom(TechDraw::DrawViewSection::getClassTypeId()))
        return;

    TechDraw::DrawViewSection *part = dynamic_cast<TechDraw::DrawViewSection *>(getViewObject());
    if (!part->hasGeometry()) {
        return;
    }

    //Base::Console().Log("drawing section face\n");

    // Get the section face from the feature
    std::vector<TechDrawGeometry::Face *> faceGeoms;
    part->getSectionSurface(faceGeoms);
    if (faceGeoms.empty()) {
        Base::Console().Log("INFO - QGIViewSection::drawSectionFace - No Face available. Check Section plane.\n");
        return;
    }

#if MOD_TECHDRAW_HANDLE_FACES
    // Draw Faces
    std::vector<TechDrawGeometry::Face *>::const_iterator fit = faceGeoms.begin();

    QGraphicsItem*graphicsItem = 0;
    QPen facePen;

//TODO: check if this is the same logic as QGIVPart
    for(int i = 0 ; fit != faceGeoms.end(); ++fit, i++) {
        std::vector<TechDrawGeometry::Wire *> faceWires = (*fit)->wires;
        QPainterPath facePath;
        for(std::vector<TechDrawGeometry::Wire *>::iterator wire = faceWires.begin(); wire != faceWires.end(); ++wire) {
            QPainterPath wirePath;
            QPointF shapePos;
            for(std::vector<TechDrawGeometry::BaseGeom *>::iterator baseGeom = (*wire)->geoms.begin(); baseGeom != (*wire)->geoms.end(); ++baseGeom) {
                //Save the start Position
                QPainterPath edgePath = drawPainterPath(*baseGeom);

                // If the current end point matches the shape end point the new edge path needs reversing
                QPointF shapePos = (wirePath.currentPosition()- edgePath.currentPosition());
                if(sqrt(shapePos.x() * shapePos.x() + shapePos.y()*shapePos.y()) < 0.05) {
                    edgePath = edgePath.toReversed();
                }
                wirePath.connectPath(edgePath);
                wirePath.setFillRule(Qt::WindingFill);
            }
            facePath.addPath(wirePath);
        }

        QGIFace *item = new QGIFace(-1);

        item->setPath(facePath);
        //         item->setStrokeWidth(lineWidth);

        QBrush faceBrush(QBrush(QColor(0,0,255,40)));

        item->setBrush(faceBrush);
        facePen.setColor(Qt::black);
        item->setPen(facePen);
        item->moveBy(x(), y());
        graphicsItem = dynamic_cast<QGraphicsItem*>(item);

        if(graphicsItem) {
            // Hide any edges that are hidden if option is set.
            //             if((*fit)->extractType == TechDrawGeometry::WithHidden && !part->ShowHiddenLines.getValue())
            //                 graphicsItem->hide();

            addToGroup(graphicsItem);
            graphicsItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
        }
    }
#endif
}

void QGIViewSection::updateView(bool update)
{
      // Iterate
    if(getViewObject() == 0 || !getViewObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId()))
        return;

    TechDraw::DrawViewSection *viewPart = dynamic_cast<TechDraw::DrawViewSection *>(getViewObject());

    if(update ||
       viewPart->SectionNormal.isTouched() ||
       viewPart->SectionOrigin.isTouched()) {
        QGIViewPart::updateView(true);
    } else {
        QGIViewPart::updateView();
    }
}

#include "moc_QGIViewSection.cpp"
