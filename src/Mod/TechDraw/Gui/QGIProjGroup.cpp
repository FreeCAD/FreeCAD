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
# include <QAction>
# include <QContextMenuEvent>
# include <QGraphicsScene>
# include <QGraphicsSceneMouseEvent>
# include <QList>
# include <QMenu>
# include <QMessageBox>
# include <QMouseEvent>
# include <QPainter>
# include <strstream>
#endif

#include <App/Document.h>
#include <Base/Console.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>

#include <Mod/Drawing/App/DrawProjGroupItem.h>
#include <Mod/Drawing/App/DrawProjGroup.h>

#include "QGIProjGroup.h"

using namespace TechDrawGui;

QGIProjGroup::QGIProjGroup(const QPoint &pos, QGraphicsScene *scene)
    :QGIViewCollection(pos, scene)
{
    setPos(pos);
    origin = new QGIGroup();
    origin->setParentItem(this);

    // In place to ensure correct drawing and bounding box calculations
    m_backgroundItem = new QGraphicsRectItem();
    m_backgroundItem->setPen(QPen(QColor(Qt::black)));

    //addToGroup(m_backgroundItem);
    setFlag(ItemIsSelectable, false);
    setFlag(ItemIsMovable, true);
    setFiltersChildEvents(true);
    borderVisible = false;
}

QGIProjGroup::~QGIProjGroup()
{
//TODO: if the QGIVO is deleted, should we clean up any remaining QGIVParts??
}

TechDraw::DrawProjGroup * QGIProjGroup::getDrawView(void) const
{
    App::DocumentObject *obj = getViewObject();
    return dynamic_cast<TechDraw::DrawProjGroup *>(obj);
}

bool QGIProjGroup::sceneEventFilter(QGI * watched, QEvent *event)
{
// i want to handle events before the child item that would ordinarily receive them
    if(event->type() == QEvent::GraphicsSceneMousePress ||
       event->type() == QEvent::GraphicsSceneMouseMove  ||
       event->type() == QEvent::GraphicsSceneMouseRelease) {

        QGIView *qAnchor = getAnchorQItem();
        if(qAnchor && watched == qAnchor) {
            QGraphicsSceneMouseEvent *mEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(event);

            switch(event->type()) {
              case QEvent::GraphicsSceneMousePress:
                  // TODO - Perhaps just pass the mouse event on to the anchor somehow?
                  if (scene()) {
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
         QGI *childItem = value.value<QGI*>();
         QGIView* gView = dynamic_cast<QGIView *>(childItem);
         if(gView) {
            TechDraw::DrawView *fView = gView->getViewObject();
            if(fView->getTypeId().isDerivedFrom(TechDraw::DrawProjGroupItem::getClassTypeId())) {
                TechDraw::DrawProjGroupItem *projItemPtr = static_cast<TechDraw::DrawProjGroupItem *>(fView);
                QString type = QString::fromAscii(projItemPtr->Type.getValueAsString());

                if (type == QString::fromAscii("Front")) {
                    gView->setLocked(true);
                    installSceneEventFilter(gView);
                    App::DocumentObject *docObj = getViewObject();
                    TechDraw::DrawProjGroup *projectionGroup = dynamic_cast<TechDraw::DrawProjGroup *>(docObj);
                    projectionGroup->Anchor.setValue(fView);
                    updateView();
                } else if ( type == QString::fromAscii("Top") ||
                    type == QString::fromAscii("Bottom")) {
                    gView->alignTo(origin, QString::fromAscii("Vertical"));
                } else if ( type == QString::fromAscii("Left")  ||
                            type == QString::fromAscii("Right") ||
                            type == QString::fromAscii("Rear") ) {
                    gView->alignTo(origin, QString::fromAscii("Horizontal"));
                } else if ( type == QString::fromAscii("FrontTopRight") ||
                            type == QString::fromAscii("FrontBottomLeft") ) {
                    gView->alignTo(origin, QString::fromAscii("45slash"));
                } else if ( type == QString::fromAscii("FrontTopLeft") ||
                            type == QString::fromAscii("FrontBottomRight") ) {
                    gView->alignTo(origin, QString::fromAscii("45backslash"));
                }
            }
         }
    }
    return QGIView::itemChange(change, value);
}


void QGIProjGroup::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    QGIView *qAnchor = getAnchorQItem();
    if(qAnchor) {
        QPointF transPos = qAnchor->mapFromScene(event->scenePos());
        if(qAnchor->shape().contains(transPos)) {
            //QGIViewCollection::mousePressEvent(event);
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
              qAnchor->mouseReleaseEvent(event);
            }
        } else if(scene() && qAnchor && (qAnchor == scene()->mouseGrabberItem())) {
            // End of Drag
            Gui::Command::openCommand("Drag Projection Group");
            //TODO: See if these commands actually handle the horizontal/vertical constraints properly...
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.X = %f",
                                    getViewObject()->getNameInDocument(), x());
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Y = %f",
                                    getViewObject()->getNameInDocument(), getY());// inverts Y 
            Gui::Command::commitCommand();
            //Gui::Command::updateActive();
        }
    }
    QGIViewCollection::mouseReleaseEvent(event);
}


QGIView * QGIProjGroup::getAnchorQItem() const
{
    // Get the currently assigned anchor view
    App::DocumentObject *anchorObj = getDrawView()->Anchor.getValue();
    TechDraw::DrawView *anchorView = dynamic_cast<TechDraw::DrawView *>(anchorObj);

    // Locate the anchor view's qgraphicsitemview
    QList<QGI *> list = childItems();

    for (QList<QGI *>::iterator it = list.begin(); it != list.end(); ++it) {
        QGIView *view = dynamic_cast<QGIView *>(*it);
        if(view && strcmp(view->getViewName(), anchorView->getNameInDocument()) == 0) {
              return view;
        }
    }
    return 0;
}

void QGIProjGroup::updateView(bool update)
{
    m_backgroundItem->setRect(boundingRect());
    return QGIViewCollection::updateView(update);
}

void QGIProjGroup::drawBorder()
{
//QGIProjGroup does not have a border!
//    Base::Console().Message("TRACE - QGIProjGroup::drawBorder - doing nothing!!\n");
}

#include "moc_QGIProjGroup.cpp"
