/***************************************************************************
 *   Copyright (c) 2016 WandererFan   (wandererfan@gmail.com)              *
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

#include <Mod/TechDraw/App/DrawViewImage.h>

#include "Rez.h"
#include "QGCustomImage.h"
#include "QGCustomClip.h"
#include "QGIViewImage.h"

using namespace TechDrawGui;

QGIViewImage::QGIViewImage()
{
    setHandlesChildEvents(false);
    setFlag(QGraphicsItem::ItemClipsChildrenToShape, false);
    setCacheMode(QGraphicsItem::NoCache);
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    m_cliparea = new QGCustomClip();
    addToGroup(m_cliparea);
    m_cliparea->setRect(0.,0.,5.,5.);
    m_cliparea->centerAt(0.,0.);

    m_imageItem = new QGCustomImage();
    m_imageItem->setTransformationMode(Qt::SmoothTransformation);
    m_cliparea->addToGroup(m_imageItem);
    m_imageItem->centerAt(0.,0.);
}

QGIViewImage::~QGIViewImage()
{
    // m_imageItem belongs to this group and will be deleted by Qt
}

QVariant QGIViewImage::itemChange(GraphicsItemChange change, const QVariant &value)
{

    return QGIView::itemChange(change, value);
}

void QGIViewImage::setViewImageFeature(TechDraw::DrawViewImage *obj)
{
    setViewFeature(static_cast<TechDraw::DrawView *>(obj));
}

void QGIViewImage::updateView(bool update)
{
    auto viewImage( dynamic_cast<TechDraw::DrawViewImage *>(getViewObject()) );
    if( viewImage == nullptr ) {
        return;
    }

    if (update ||
        viewImage->isTouched() ||
        viewImage->Width.isTouched() ||
        viewImage->Height.isTouched() ||
        viewImage->ImageFile.isTouched()) {
        draw();
    }

    if (viewImage->Scale.isTouched()) {
        draw();
    }

    QGIView::updateView(update);
}

void QGIViewImage::draw()
{
    if (!isVisible()) {
        return;
    }

    auto viewImage( dynamic_cast<TechDraw::DrawViewImage*>(getViewObject()) );
    if (!viewImage)
        return;
    QRectF newRect(0.0,0.0,viewImage->Width.getValue(),viewImage->Height.getValue());
    m_cliparea->setRect(newRect);
    drawImage();
    m_cliparea->centerAt(0.0,0.0);

    QGIView::draw();
}

void QGIViewImage::drawImage()
{
    auto viewImage( dynamic_cast<TechDraw::DrawViewImage *>(getViewObject()) );
    if( viewImage == nullptr ) {
        return;
    }

    if (!viewImage->ImageFile.isEmpty()) {
        QString fileSpec = QString::fromUtf8(viewImage->ImageFile.getValue(),strlen(viewImage->ImageFile.getValue()));
        m_imageItem->load(fileSpec);
        m_imageItem->setScale(viewImage->getScale());
        QRectF br = m_cliparea->rect();
        double midX = br.width()/2.0;
        double midY = br.height()/2.0;
        m_imageItem->centerAt(midX,midY);
        m_imageItem->show();
    }
}

void QGIViewImage::rotateView(void)
{
    QRectF r = m_cliparea->boundingRect();
    m_cliparea->setTransformOriginPoint(r.center());
    double rot = getViewObject()->Rotation.getValue();
    m_cliparea->setRotation(-rot);
}


