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
#include <algorithm>    // std::find
#include <QGraphicsScene>
#endif

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include <Mod/TechDraw/App/DrawViewClip.h>

#include "Rez.h"
#include "QGCustomRect.h"
#include "QGCustomClip.h"
#include "DrawGuiUtil.h"
#include "QGIViewClip.h"

using namespace TechDrawGui;

QGIViewClip::QGIViewClip()
{
    setHandlesChildEvents(false);
    setCacheMode(QGraphicsItem::NoCache);
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);

    m_cliparea = new QGCustomClip();
    addToGroup(m_cliparea);
    m_cliparea->setPos(0.,0.);
    m_cliparea->setRect(0.,0.,Rez::guiX(5.),Rez::guiX(5.));

    m_frame = new QGCustomRect();
    addToGroup(m_frame);
    m_frame->setPos(0.,0.);
    m_frame->setRect(0.,0.,Rez::guiX(5.),Rez::guiX(5.));
}


QVariant QGIViewClip::itemChange(GraphicsItemChange change, const QVariant &value)
{
    return QGIView::itemChange(change, value);
}

void QGIViewClip::updateView(bool update)
{
    auto viewClip( dynamic_cast<TechDraw::DrawViewClip *>(getViewObject()) );
    if( viewClip == nullptr ) {
        return;
    }

    if (update ||
        viewClip->isTouched() ||
        viewClip->Height.isTouched() ||
        viewClip->Width.isTouched() ||
        viewClip->ShowFrame.isTouched() ||
        viewClip->Views.isTouched() ) {

        draw();
    }

    QGIView::updateView(update);
}

void QGIViewClip::draw()
{
    if (!isVisible()) {
        return;
    }

    drawClip();
    if (borderVisible) {
        drawBorder();
    }
}

void QGIViewClip::drawClip()
{
    auto viewClip( dynamic_cast<TechDraw::DrawViewClip *>(getViewObject()) );

    if( viewClip == nullptr ) {
        return;
    }

    prepareGeometryChange();
    double h = viewClip->Height.getValue();
    double w = viewClip->Width.getValue();
    QRectF r = QRectF(-Rez::guiX(w)/2.0,-Rez::guiX(h)/2.0,Rez::guiX(w),Rez::guiX(h));
    m_frame->setRect(r);                    // (-50,-50) -> (50,50)
    m_frame->setPos(0.,0.);
    if (viewClip->ShowFrame.getValue()) {
        m_frame->show();
    } else {
        m_frame->hide();
    }

    //probably a slicker way to do this?
    QPointF midFrame   = m_frame->boundingRect().center();
    QPointF midMapped  = mapFromItem(m_frame,midFrame);
    QPointF clipOrigin = mapToItem(m_cliparea,midMapped);

    m_cliparea->setRect(r.adjusted(-1,-1,1,1));
    
    std::vector<std::string> childNames = viewClip->getChildViewNames();
    //for all child Views in Clip, add the graphics representation of the View to the Clip group
    for(std::vector<std::string>::iterator it = childNames.begin(); it != childNames.end(); it++) {
        QGIView* qgiv = getQGIVByName((*it));
        if (qgiv) {
            //TODO: why is qgiv never already in a group?
            if (qgiv->group() != m_cliparea) {
                qgiv->hide();
                scene()->removeItem(qgiv);
                m_cliparea->addToGroup(qgiv);
                qgiv->isInnerView(true);
                double x = Rez::guiX(qgiv->getViewObject()->X.getValue());
                double y = Rez::guiX(qgiv->getViewObject()->Y.getValue());
                qgiv->setPosition(clipOrigin.x() + x, clipOrigin.y() + y);
                if (viewClip->ShowLabels.getValue()) {
                    qgiv->toggleBorder(true);
                } else {
                    qgiv->toggleBorder(false);
                }
                qgiv->show();
            }
        } else {
            Base::Console().Warning("Logic error? - drawClip() - qgiv for %s not found\n",(*it).c_str());   //gview for feature !exist
        }
    }

    //for all graphic views in qgigroup, remove from qgigroup the ones that aren't in ViewClip
    QList<QGraphicsItem*> qgItems = m_cliparea->childItems();
    QList<QGraphicsItem*>::iterator it = qgItems.begin();
    for (; it != qgItems.end(); it++) {
        QGIView* qv = dynamic_cast<QGIView*>((*it));
        if (qv) {
            std::string qvName = std::string(qv->getViewName());
            if (std::find(childNames.begin(),childNames.end(),qvName) == childNames.end()) {
                m_cliparea->removeFromGroup(qv);
                removeFromGroup(qv);
                qv->isInnerView(false);
                qv->toggleBorder(true);
            }
        }
    }
}
