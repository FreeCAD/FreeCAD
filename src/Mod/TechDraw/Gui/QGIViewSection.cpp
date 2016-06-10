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

#include "ZVALUE.h"
#include "QGIViewSection.h"

using namespace TechDrawGui;

void QGIViewSection::draw()
{
    if (!isVisible()) {
        return;
    }

    QGIViewPart::draw();
    drawSectionFace();
}

void QGIViewSection::drawSectionFace()
{
    if(getViewObject() == 0 || !getViewObject()->isDerivedFrom(TechDraw::DrawViewSection::getClassTypeId()))
        return;

    TechDraw::DrawViewSection *section = dynamic_cast<TechDraw::DrawViewSection *>(getViewObject());
    if (!section->hasGeometry()) {
        return;
    }

    std::vector<TechDrawGeometry::Face*> sectionFaces;
    sectionFaces = section->getFaceGeometry();
    if (sectionFaces.empty()) {
        Base::Console().Log("INFO - QGIViewSection::drawSectionFace - No sectionFaces available. Check Section plane.\n");
        return;
    }
    std::vector<TechDrawGeometry::Face *>::iterator fit = sectionFaces.begin();
    QColor faceColor(0,0,255,40);              //temp. sb preference or property.
    for(; fit != sectionFaces.end(); fit++) {
        QGIFace* newFace = drawFace(*fit,-1);  //TODO: do we need to know which sectionFace this QGIFace came from?
        newFace->setZValue(ZVALUE::SECTIONFACE);
        newFace->setFill(faceColor, Qt::SolidPattern);
        newFace->setPrettyNormal();
        newFace->setAcceptHoverEvents(false);
        newFace->setFlag(QGraphicsItem::ItemIsSelectable, false);
        newFace->setAcceptHoverEvents(false);
    }
}

void QGIViewSection::updateView(bool update)
{
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
