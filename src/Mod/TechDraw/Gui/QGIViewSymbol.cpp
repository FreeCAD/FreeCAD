/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
 *   Copyright (c) 2014 WandererFan <wandererfan@gmail.com>                *
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
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsSceneHoverEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QString>
#include <sstream>
#include <QRectF>
#endif

//#include <qmath.h>

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include <Mod/TechDraw/App/DrawViewSymbol.h>
#include <Mod/TechDraw/App/DrawViewDraft.h>
#include <Mod/TechDraw/App/DrawViewArch.h>

#include "QGCustomSvg.h"
#include "QGDisplayArea.h"
#include "QGIViewSymbol.h"
#include "DrawGuiUtil.h"
#include "Rez.h"

using namespace TechDrawGui;

QGIViewSymbol::QGIViewSymbol()
{
    setHandlesChildEvents(false);
    setCacheMode(QGraphicsItem::NoCache);
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    m_displayArea = new QGDisplayArea();
    addToGroup(m_displayArea);
    m_displayArea->centerAt(0.,0.);

    m_svgItem = new QGCustomSvg();
    m_displayArea->addToGroup(m_svgItem);
    m_svgItem->centerAt(0.,0.);
}

QGIViewSymbol::~QGIViewSymbol()
{
    // m_svgItem belongs to this group and will be deleted by Qt
}

QVariant QGIViewSymbol::itemChange(GraphicsItemChange change, const QVariant &value)
{

    return QGIView::itemChange(change, value);
}

void QGIViewSymbol::setViewSymbolFeature(TechDraw::DrawViewSymbol *obj)
{
    // called from QGVPage. (once)
    setViewFeature(static_cast<TechDraw::DrawView *>(obj));
}

void QGIViewSymbol::updateView(bool update)
{
    auto viewSymbol( dynamic_cast<TechDraw::DrawViewSymbol *>(getViewObject()) );
    if( viewSymbol == nullptr ) {
        return;
    }

    if (update ||
        viewSymbol->isTouched() ||
        viewSymbol->Symbol.isTouched()) {
        draw();
    }

    if (viewSymbol->Scale.isTouched()) {
        draw();
    }

    QGIView::updateView(update);
}

void QGIViewSymbol::draw()
{
    if (!isVisible()) {
        return;
    }

    drawSvg();
    QGIView::draw();
}

void QGIViewSymbol::drawSvg()
{
    auto viewSymbol( dynamic_cast<TechDraw::DrawViewSymbol *>(getViewObject()) );
    if( viewSymbol == nullptr ) {
        return;
    }

    double rezfactor = Rez::getRezFactor();
    double scaling = viewSymbol->getScale();
    double pxMm = 3.78;                 //96px/25.4mm ( CSS/SVG defined value of 96 pixels per inch)
//    double pxMm = 3.54;                 //90px/25.4mm ( inkscape value version <= 0.91)
                                        //some software uses different px/in, so symbol will need Scale adjusted.
    //Arch/Draft views are in px and need to be scaled @ rezfactor px/mm to ensure proper representation
    if (viewSymbol->isDerivedFrom(TechDraw::DrawViewArch::getClassTypeId()) ||
        viewSymbol->isDerivedFrom(TechDraw::DrawViewDraft::getClassTypeId()) ) {
        scaling = scaling * rezfactor;
    } else {
        scaling = scaling * rezfactor / pxMm;
    }
    m_svgItem->setScale(scaling);

    QByteArray qba(viewSymbol->Symbol.getValue(),strlen(viewSymbol->Symbol.getValue()));
    symbolToSvg(qba);
    rotateView();
}

void QGIViewSymbol::symbolToSvg(QByteArray qba)
{
    if (qba.isEmpty()) {
        return;
    }

    prepareGeometryChange();
    if (!m_svgItem->load(&qba)) {
        Base::Console().Error("Error - Could not load Symbol into SVG renderer for %s\n", getViewName());
    }
    m_svgItem->centerAt(0.,0.);
}

void QGIViewSymbol::rotateView(void)
{
    QRectF r = m_displayArea->boundingRect();
    m_displayArea->setTransformOriginPoint(r.center());
    double rot = getViewObject()->Rotation.getValue();
    m_displayArea->setRotation(-rot);
}

