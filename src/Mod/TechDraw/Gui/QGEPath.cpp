/***************************************************************************
 *   Copyright (c) 2019 Wandererfan <wandererfan@gmail.com>                *
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
#include <QGraphicsScene>
#include <QGraphicsSceneHoverEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPathStroker>
#include <QStyleOptionGraphicsItem>
#endif

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include "DrawGuiStd.h"
#include "QGIPrimPath.h"
#include "QGIVertex.h"
#include "QGIView.h"
#include "QGEPath.h"

using namespace TechDrawGui;

QGMarker::QGMarker(int idx) : QGIVertex(idx)
{
//    Base::Console().Message("QGMarker::QGMarker(%d)\n", idx);
    setFlag(QGraphicsItem::ItemIsMovable, true);
}

QVariant QGMarker::itemChange(GraphicsItemChange change, const QVariant &value)
{
//    Base::Console().Message("QGMarker::itemChange(%d)\n",change);
    return QGIVertex::itemChange(change, value);
}

void QGMarker::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
//    Base::Console().Message("QGMarker::hoverEnterEvent(%d)\n",getProjIndex());
    QGIVertex::hoverEnterEvent(event);
}

void QGMarker::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
//    Base::Console().Message("QGMarker::hoverLeaveEvent(%d)\n",getProjIndex());
    QGIVertex::hoverLeaveEvent(event);
}

void QGMarker::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
//    Base::Console().Message("QGMarker::mousePressEvent() - focustype: %d\n",
//                            scene()->focusItem()->type() - QGraphicsItem::UserType);

    if (event->button() == Qt::RightButton) {    //we're done
        Q_EMIT endEdit();
        event->accept();
        return;
    } else if(scene() && this == scene()->mouseGrabberItem()) {
        QPointF mapped = mapToParent(event->pos());
        Q_EMIT dragging(mapped, getProjIndex());
    }
    QGIVertex::mousePressEvent(event);
}

void QGMarker::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
//    Base::Console().Message("QGMarker::mouseMoveEvent(%d)\n", getProjIndex());
    QGIVertex::mouseMoveEvent(event);
}

void QGMarker::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
//    Base::Console().Message("QGMarker::mouseReleaseEvent(%d) - focus: %d\n", getProjIndex(), hasFocus());
    if (event->button() == Qt::RightButton) {    //we're done
        Q_EMIT endEdit();
        event->accept();      //this is mine!
        return;
    }

    if(this->scene() && this == this->scene()->mouseGrabberItem()) {
        QPointF mapped = mapToParent(event->pos());
        Q_EMIT dragFinished(mapped, getProjIndex());
        event->accept();
    }

    QGIVertex::mouseReleaseEvent(event);
}

void QGMarker::mouseDoubleClickEvent(QGraphicsSceneMouseEvent * event)
{
//    Base::Console().Message("QGMarker::mouseDoubleClickEvent(%d)\n",getProjIndex());
    if (event->button() == Qt::RightButton) {    //we're done
        Q_EMIT endEdit();
        event->accept();      //this is mine!
        return;
    }
    QGIVertex::mouseDoubleClickEvent(event);
}

//why can markers get the keyPress, but QGTracker can not??
void QGMarker::keyPressEvent(QKeyEvent * event)
{
    if (event->key() == Qt::Key_Escape) {
        Q_EMIT endEdit();
    }
    QGIVertex::keyPressEvent(event);
}


void QGMarker::setRadius(float r)
{
    //TODO:: implement different marker shapes. circle, square, triangle, ???
    //if (m_markerShape == Circle) { ...
    //setRect(QRectF) for rectangular markers
    m_radius = r;
    QPainterPath p;
    p.addRect(-r/2.0, -r/2.0, r, r);
    setPath(p);
}

//void QGMarker::setShape(int s)
//{
//    m_markerShape = s;
//}

QRectF QGMarker::boundingRect() const
{
    return QGIVertex::boundingRect();
}

QPainterPath QGMarker::shape() const
{
    return QGIVertex::shape();
}


void QGMarker::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    //~ painter->drawRect(boundingRect());          //good for debugging

    QGIVertex::paint (painter, &myOption, widget);
}

//******************************************************************************

QGEPath::QGEPath() :
    m_attach(QPointF(0.0,0.0)),
    m_scale(1.0),
    m_inEdit(false),
    m_parentItem(nullptr)
{
    setHandlesChildEvents(false);
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, false);
//    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, false);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges,true);

    QGraphicsItem* parent = parentItem();
    QGIView* pView = dynamic_cast<QGIView*>(parent);
    if (pView != nullptr) {
        m_parentItem = pView;
    }
}

QVariant QGEPath::itemChange(GraphicsItemChange change, const QVariant &value)
{
//    Base::Console().Message("QGEP::itemChange(%d) - type: %d\n", change,type() - QGraphicsItem::UserType);
    if (change == ItemSelectedHasChanged && scene()) {
        if(isSelected()) {
            Q_EMIT selected(true);
            setPrettySel();
        } else {
            Q_EMIT selected(false);
            setPrettyNormal();
        }
    }
    return QGIPrimPath::itemChange(change, value);
}

void QGEPath::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    Q_EMIT hover(true);
    if (!isSelected()) {
        setPrettyPre();
    }
    QGIPrimPath::hoverEnterEvent(event);
}

void QGEPath::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    QGIView *view = dynamic_cast<QGIView *> (parentItem());
    assert(view != 0);
    Q_UNUSED(view);

    Q_EMIT hover(false);
    QGraphicsItem* parent = parentItem();
    bool parentSel(false);
    if (parent != nullptr) {
        parentSel = parent->isSelected();
    }
    if (!parentSel  && !isSelected()) {
        setPrettyNormal();
    }
    QGraphicsPathItem::hoverLeaveEvent(event);
//    QGIPrimPath::hoverLeaveEvent(event);  //QGIPP::hoverleave will reset pretty to normal
}

void QGEPath::startPathEdit()
{
//    Base::Console().Message("QGEPath::startPathEdit()\n");
    inEdit(true);
    showMarkers(m_deltas);
}    

void QGEPath::showMarkers(std::vector<QPointF> deltas)
{
//    Base::Console().Message("QGEPath::showMarkers()\n");
    if (!inEdit()) {
        return;
    }
    QGraphicsItem* qgepParent = parentItem();
    if (qgepParent == nullptr) {
        Base::Console().Error("QGEPath::showMarkers - no parent item\n");
        return;
    }

    clearMarkers();
    
    int pointDx = 0;
    for (auto& p: deltas) {
        QGMarker* v = new QGMarker(pointDx);
        v->setFlag(QGraphicsItem::ItemIsMovable, true);
        v->setFlag(QGraphicsItem::ItemIsFocusable, true);
        v->setParentItem(this);
        QObject::connect(
            v, SIGNAL(dragFinished(QPointF, int)),
            this     , SLOT  (onDragFinished(QPointF, int))
           );
        QObject::connect(
            v, SIGNAL(dragging(QPointF, int)),
            this     , SLOT  (onDragging(QPointF, int))
           );
        QObject::connect(
            v, SIGNAL(doubleClick(QPointF, int)),
            this     , SLOT  (onDoubleClick(QPointF, int))
           );
        QObject::connect(
            v, SIGNAL(endEdit()),
            this     , SLOT  (onEndEdit())
           );
//TODO: double r = getMarkerSize();
//      v->setRadius(r);
        v->setRadius(50.0);
        v->setNormalColor(QColor(Qt::black));
        v->setZValue(ZVALUE::VERTEX);
        v->setPos(p * m_scale);
        v->show();
        
        m_markers.push_back(v);
        pointDx++;
    }        
}

void QGEPath::clearMarkers()
{
//    Base::Console().Message("QGEPath::clearMarkers()\n");
    if (m_markers.empty()) {
        return;
    }
    for (auto& m: m_markers) {
        if (m != nullptr) {
            QGraphicsScene* s = m->scene();
            if (s != nullptr) {
                s->removeItem(m);           //should this be setParentItem(nullptr) instead??
            }
            delete m;
        }
    }
    m_markers.clear();
}

// end of node marker drag
void QGEPath::onDragFinished(QPointF pos, int markerIndex)
{
//    Base::Console().Message("QGEPath::onDragFinished()\n");
    m_deltas.at(markerIndex) = pos / m_scale;
    updatePath();
}

void QGEPath::onDragging(QPointF pos, int markerIndex)
{
     Q_UNUSED(pos);
     Q_UNUSED(markerIndex);
     //Q_EMIT dragging(
    //TODO: could "live update" line during Drag
}

//this is for double click on a marker
void QGEPath::onDoubleClick(QPointF pos, int markerIndex)
{
    Q_UNUSED(pos);
    Q_UNUSED(markerIndex);
//    Base::Console().Message("QGEPath::onDoubleClick()\n");
    updatePath();
    updateFeature();
    clearMarkers(); 
    inEdit(false);
}

void QGEPath::onEndEdit(void)
{
//    Base::Console().Message("QGEPath::onEndEdit()\n");
    updatePath();
    inEdit(false);
    updateFeature();
    clearMarkers(); 
}

//updates the painterpath using our deltas
//path is (0,0),d(p1-p0), d(p2-p1),...
void QGEPath::updatePath(void)
{
//    Base::Console().Message("QGEPath::updatePath() - scale: %.3f\n", m_scale);
    if (m_deltas.empty()) {
        Base::Console().Log("QGEPath::updatePath - no points\n");
        return;
    }
    QPainterPath result;
    prepareGeometryChange();
    if (m_deltas.size() > 1) {
        result.moveTo(m_deltas.front());       //(0,0)
        for (int i = 1; i < (int)m_deltas.size(); i++) {
            result.lineTo(m_deltas.at(i) * m_scale);
        }
    }
    setPath(result);
//    setPrettyNormal();
}

void QGEPath::makeDeltasFromPoints(std::vector<QPointF> pts)
{
//    Base::Console().Message("QGEPath::makeDeltasFromPoints()\n");
    if (pts.empty()) {
        Base::Console().Log("QGEPath::makeDeltasFromPoints - no points\n");
        return;
    }
    std::vector<QPointF> deltas;
    QPointF firstPt = pts.front();
    QPointF newStart(0.0,0.0);
    deltas.push_back(newStart);
    unsigned int i = 1;
    for (; i < pts.size(); i++) {
        QPointF mapped = pts.at(i) - firstPt;
        deltas.push_back(mapped);
    }
    m_deltas = deltas;
}

//tell parent points editing is finished
void QGEPath::updateFeature(void)
{
//    Base::Console().Message("QGEPath::updateFeature()\n");
    if (!inEdit()) {
        Q_EMIT pointsUpdated(m_deltas);
    }
}

QRectF QGEPath::boundingRect() const
{
    return shape().controlPointRect();
}

QPainterPath QGEPath::shape() const
{
    QPainterPath outline;
    QPainterPathStroker stroker;
    stroker.setWidth(getEdgeFuzz() * 2.0);
    outline = stroker.createStroke(path()).simplified();
    return outline;
}

 double QGEPath::getEdgeFuzz(void) const
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
                                         GetGroup("Preferences")->GetGroup("Mod/TechDraw/General");
    double result = hGrp->GetFloat("EdgeFuzz",10.0);
    return result;
}


void QGEPath::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

//     painter->drawRect(boundingRect());          //good for debugging

    QGIPrimPath::paint (painter, &myOption, widget);
}

void QGEPath::addPoint(unsigned int before, unsigned int after) 
{
    std::vector<QPointF> deltaCopy = getDeltas();
    unsigned int iMax = deltaCopy.size() - 1;
    if ( (after <= before) ||
         (after > iMax) ) {
        Base::Console().Message("QGEP::addPoint - parameter out of range\n");
        return;
    }
    QPointF bPt = deltaCopy.at(before);
    QPointF aPt = deltaCopy.at(after);
    QPointF newPt = (bPt + aPt) / 2.0;
    deltaCopy.insert(deltaCopy.begin() + after, newPt);
    setDeltas(deltaCopy);
}

void QGEPath::deletePoint(unsigned int atX)
{
    std::vector<QPointF> deltaCopy = getDeltas();
    unsigned int iMax = deltaCopy.size() - 1;
    if ( (atX < 1) ||                          //can't delete attach point
         (atX > iMax) ) {
        Base::Console().Message("QGEP::deletePoint - parameter out of range\n");
        return;
    }
    deltaCopy.erase(deltaCopy.begin() + (unsigned int) atX);
    setDeltas(deltaCopy);
}

void QGEPath::dumpPoints(char* text)
{
    int idb = 0;
    for (auto& d: m_deltas) {
        Base::Console().Message("QGEP - %s - delta: %d %s\n", text,
                                 idb,TechDraw::DrawUtil::formatVector(d).c_str());
        idb++;
    } 
}

#include <Mod/TechDraw/Gui/moc_QGEPath.cpp>

