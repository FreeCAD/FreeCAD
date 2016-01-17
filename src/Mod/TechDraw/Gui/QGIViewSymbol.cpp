/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
 *                 2014 wandererfan <WandererFan@gmail.com>                *
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
#endif

#include <qmath.h>

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include "../App/DrawViewSymbol.h"
#include "QGIViewSymbol.h"

using namespace TechDrawGui;

QGIViewSymbol::QGIViewSymbol(const QPoint &pos, QGraphicsScene *scene) :QGIView(pos, scene)
{
    setHandlesChildEvents(false);
    setCacheMode(QGraphicsItem::NoCache);
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsMovable, true);

    m_svgRender = new QSvgRenderer();

    m_svgItem = new QGCustomSvg();
    addToGroup(m_svgItem);
    m_svgItem->setPos(0.,0.);
}

QGIViewSymbol::~QGIViewSymbol()
{
    // m_svgItem belongs to this group and will be deleted by Qt 
    delete(m_svgRender);
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
    if(getViewObject() == 0 || !getViewObject()->isDerivedFrom(TechDraw::DrawViewSymbol::getClassTypeId()))
        return;

    TechDraw::DrawViewSymbol *viewSymbol = dynamic_cast<TechDraw::DrawViewSymbol *>(getViewObject());

    if (update ||
        viewSymbol->isTouched() ||
        viewSymbol->Symbol.isTouched()) {
        draw();
    }
    
    if (viewSymbol->Scale.isTouched()) {
        setScale(viewSymbol->Scale.getValue());
        draw();
    }

    QGIView::updateView(update);
}

void QGIViewSymbol::draw()
{
    drawSvg();
    if (borderVisible) {
        drawBorder();
    }
}

void QGIViewSymbol::drawSvg()
{
    if(getViewObject() == 0 || !getViewObject()->isDerivedFrom(TechDraw::DrawViewSymbol::getClassTypeId()))
        return;

    TechDraw::DrawViewSymbol *viewSymbol = dynamic_cast<TechDraw::DrawViewSymbol *>(getViewObject());

    QString qs(QString::fromUtf8(viewSymbol->Symbol.getValue()));
    QByteArray qba;
    qba.append(qs);
    if (!load(&qba)) {
        Base::Console().Error("QGIViewSymbol::drawSvg - Could not load %s.Symbol into renderer\n", viewSymbol->getNameInDocument());
    }
    m_svgItem->setPos(0.,0.);
}

QRectF QGIViewSymbol::boundingRect() const
{
    return childrenBoundingRect();
}

bool QGIViewSymbol::load(QByteArray *svgBytes)
{
    bool success = m_svgRender->load(*svgBytes);
    m_svgItem->setSharedRenderer(m_svgRender);
    return(success);
}

#include "moc_QGIViewSymbol.cpp"

