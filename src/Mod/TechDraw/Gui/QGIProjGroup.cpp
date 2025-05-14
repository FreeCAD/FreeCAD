/***************************************************************************
 *   Copyright (c) 2012-2013 Luke Parry <l.parry@warwick.ac.uk>            *
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
# include <QGraphicsScene>
# include <QGraphicsSceneMouseEvent>
# include <QList>
#endif

#include <App/Document.h>
#include <Base/Console.h>
#include <Gui/Selection/Selection.h>
#include <Mod/TechDraw/App/DrawProjGroup.h>
#include <Mod/TechDraw/App/DrawProjGroupItem.h>

#include "QGIProjGroup.h"
#include "Rez.h"


using namespace TechDrawGui;

QGIProjGroup::QGIProjGroup()
{
    m_origin = new QGraphicsItemGroup();                  //QGIG added to this QGIG??
    m_origin->setParentItem(this);

    setFlag(ItemIsSelectable, false);
    setFlag(ItemIsMovable, true);
    setFiltersChildEvents(true);
//    setFrameState(false);
}

TechDraw::DrawProjGroup * QGIProjGroup::getDrawView() const
{
    App::DocumentObject *obj = getViewObject();
    return dynamic_cast<TechDraw::DrawProjGroup *>(obj);
}

bool QGIProjGroup::sceneEventFilter(QGraphicsItem* watched, QEvent *event)
{
// i want to handle events before the child item that would ordinarily receive them
    if(event->type() == QEvent::GraphicsSceneMousePress ||
       event->type() == QEvent::GraphicsSceneMouseMove  ||
       event->type() == QEvent::GraphicsSceneMouseRelease) {

        QGIView *qAnchor = getAnchorQItem();
        if(qAnchor && watched == qAnchor) {
            auto *mEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(event);

            switch(event->type()) {
              case QEvent::GraphicsSceneMousePress:
                  // TODO - Perhaps just pass the mouse event on to the anchor somehow?
                  if (scene() && !qAnchor->isSelected()) {
                      scene()->clearSelection();
                      qAnchor->setSelected(true);
                  }
                  mousePressEvent(mEvent);
                  break;
              case QEvent::GraphicsSceneMouseMove:
                  mouseMoveEvent(mEvent);
                  break;
              case QEvent::GraphicsSceneMouseRelease:
                  mouseReleaseEvent(mEvent);
                  break;
              default:
                  break;
            }
            return true;
        }
    }

    return false;
}
QVariant QGIProjGroup::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if(change == ItemChildAddedChange && scene()) {
         QGraphicsItem*childItem = value.value<QGraphicsItem*>();
         QGIView* gView = dynamic_cast<QGIView *>(childItem);
         if(gView) {
            TechDraw::DrawView *fView = gView->getViewObject();
            if(fView->isDerivedFrom<TechDraw::DrawProjGroupItem>()) {
                auto *projItemPtr = static_cast<TechDraw::DrawProjGroupItem *>(fView);
                QString type = QString::fromLatin1(projItemPtr->Type.getValueAsString());

                if (type == QStringLiteral("Front")) {
                    gView->alignTo(m_origin, QStringLiteral("None"));
                    installSceneEventFilter(gView);
                }
                else if ( type == QStringLiteral("Top") ||
                    type == QStringLiteral("Bottom")) {
                    gView->alignTo(m_origin, QStringLiteral("Vertical"));
                }
                else if ( type == QStringLiteral("Left")  ||
                            type == QStringLiteral("Right") ||
                            type == QStringLiteral("Rear") ) {
                    gView->alignTo(m_origin, QStringLiteral("Horizontal"));
                }
                else if ( type == QStringLiteral("FrontTopRight") ||
                            type == QStringLiteral("FrontBottomLeft") ) {
                    gView->alignTo(m_origin, QStringLiteral("45slash"));
                }
                else if ( type == QStringLiteral("FrontTopLeft") ||
                            type == QStringLiteral("FrontBottomRight") ) {
                    gView->alignTo(m_origin, QStringLiteral("45backslash"));
                }
            }
         }
    }
    return QGIViewCollection::itemChange(change, value);
}

void QGIProjGroup::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    QGIView *qAnchor = getAnchorQItem();
    if(qAnchor) {
        QPointF transPos = qAnchor->mapFromScene(event->scenePos());
        if(qAnchor->shape().contains(transPos)) {
            mousePos = event->screenPos();
        }
    }
    event->accept();
}

void QGIProjGroup::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
    QGIView *qAnchor = getAnchorQItem();
    if(scene() && qAnchor && (qAnchor == scene()->mouseGrabberItem())) {
        if((mousePos - event->screenPos()).manhattanLength() > 5) {    //if the mouse has moved more than 5, process the mouse event
            QGIViewCollection::mouseMoveEvent(event);
        }

    }
    event->accept();
}

void QGIProjGroup::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
     if(scene()) {
       QGIView *qAnchor = getAnchorQItem();
        if((mousePos - event->screenPos()).manhattanLength() < 5) {
            if(qAnchor && qAnchor->shape().contains(event->pos())) {
                event->ignore();
                qAnchor->mouseReleaseEvent(event);
            }
        }
        else if(scene() && qAnchor) {
            // End of Drag
            getViewObject()->setPosition(Rez::appX(x()), Rez::appX(getY()));
        }
    }
    QGIViewCollection::mouseReleaseEvent(event);
}

QGIView * QGIProjGroup::getAnchorQItem() const
{
    // Get the currently assigned anchor view
    App::DocumentObject *anchorObj = getDrawView()->Anchor.getValue();
    auto anchorView( dynamic_cast<TechDraw::DrawView *>(anchorObj) );
    if (!anchorView) {
        return nullptr;
    }

    // Locate the anchor view's qgraphicsitemview
    QList<QGraphicsItem*> list = childItems();

    for (QList<QGraphicsItem*>::iterator it = list.begin(); it != list.end(); ++it) {
        QGIView *view = dynamic_cast<QGIView *>(*it);
        if(view && strcmp(view->getViewName(), anchorView->getNameInDocument()) == 0) {
              return view;
        }
    }
    return nullptr;
}

//QGIPG does not rotate. Only individual views rotate
void QGIProjGroup::rotateView()
{
    Base::Console().warning("QGIPG: Projection Groups do not rotate. Change ignored\n");
}

void QGIProjGroup::drawBorder()
{
//QGIProjGroup does not have a border!
//    Base::Console().message("TRACE - QGIProjGroup::drawBorder - doing nothing!!\n");
}

