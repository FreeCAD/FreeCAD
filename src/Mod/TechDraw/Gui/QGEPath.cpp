/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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
# include <QGraphicsSceneHoverEvent>
# include <QKeyEvent>
# include <QPainterPath>
# include <QPainterPathStroker>
#endif

#include <Base/Console.h>
#include <Mod/TechDraw/App/DrawLeaderLine.h>
#include <Mod/TechDraw/App/DrawUtil.h>

#include "QGEPath.h"
#include "PreferencesGui.h"
#include "QGILeaderLine.h"
#include "QGIPrimPath.h"
#include "QGIVertex.h"
#include "QGIView.h"
#include "Rez.h"
#include "ZVALUE.h"
#include "DrawGuiUtil.h"


using namespace TechDrawGui;
using namespace TechDraw;
using DU = DrawUtil;
using DGU = DrawGuiUtil;

QGMarker::QGMarker(int idx) : QGIVertex(idx),
    m_dragging(false)
{
//    Base::Console().message("QGMarker::QGMarker(%d)\n", idx);
    setFlag(QGraphicsItem::ItemIsMovable, true);
}

void QGMarker::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
//    Base::Console().message("QGMarker::mousePressEvent() - focustype: %d\n",
//                            scene()->focusItem()->type() - QGraphicsItem::UserType);

    if (event->button() == Qt::RightButton) {    //we're done
        Q_EMIT endEdit();
        event->accept();
        return;
    }

    if(scene() && this == scene()->mouseGrabberItem()) {
        //start dragging
        m_dragging = true;
        Q_EMIT dragging(pos(), getProjIndex());      //pass center of marker[i] to epath
    }
    QGIVertex::mousePressEvent(event);
}

void QGMarker::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
    if (event->button() == Qt::RightButton) {
        // we are finished our edit session
        Q_EMIT endEdit();
        m_dragging = false;
        return;
    }

    if(this->scene() && this == this->scene()->mouseGrabberItem()) {
        if (m_dragging) {
            m_dragging = false;
            setSelected(false);
            // send this marker's new position to QGEPath
            Q_EMIT dragFinished(pos(), getProjIndex());      //pass center of marker[i] to epath
        }
    }
    QGIVertex::mouseReleaseEvent(event);
}

void QGMarker::mouseDoubleClickEvent(QGraphicsSceneMouseEvent * event)
{
//    Base::Console().message("QGMarker::mouseDoubleClickEvent(%d)\n", getProjIndex());
    if (event->button() == Qt::RightButton) {    //we're done
        // we are finished our edit session
        Q_EMIT endEdit();
        m_dragging = false;
        return;
    }
    QGIVertex::mouseDoubleClickEvent(event);
}

void QGMarker::keyPressEvent(QKeyEvent * event)
{
//    Base::Console().message("QGMarker::keyPressEvent(%d)\n", getProjIndex());
    if (event->key() == Qt::Key_Escape) {
        m_dragging = false;
        Q_EMIT endEdit();
    }
    QGIVertex::keyPressEvent(event);
}

//! adjust the size of this marker
void QGMarker::setRadius(double radius)
{
    //TODO:: implement different marker shapes. circle, square, triangle, ???
    //if (m_markerShape == Circle) { ...
    //setRect(QRectF) for rectangular markers
    m_radius = radius;
    QPainterPath pPath;
    pPath.addRect(-radius/2.0, -radius/2.0, radius, radius);
    setPath(pPath);
}

//******************************************************************************


//! QGEPath is an editable version of QGIPrimPath.  Points along the path can be dragged to new
//! positions that are returned to the caller on completion of the edit session.    The points are in
//! scene coordinates and are relative to the scene position of the caller.

QGEPath::QGEPath() :
    m_scale(1.0),
    m_inEdit(false),
    m_startAdj(0.0),
    m_endAdj(0.0)
{
    setHandlesChildEvents(false);
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, false);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    m_ghost = new QGIPrimPath();       //drawing/editing line
    m_ghost->setParentItem(this);
    m_ghost->setNormalColor(Qt::red);
    m_ghost->setStyle(Qt::DashLine);
    m_ghost->setPrettyNormal();
    m_ghost->hide();

}

QVariant QGEPath::itemChange(GraphicsItemChange change, const QVariant &value)
{
//    Base::Console().message("QGEP::itemChange(%d) - type: %d\n", change, type() - QGraphicsItem::UserType);
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
    auto view = dynamic_cast<QGIView *> (parentItem());
    assert(view);
    Q_UNUSED(view);

    Q_EMIT hover(false);
    QGraphicsItem* parent = parentItem();
    bool parentSel(false);
    if (parent) {
        parentSel = parent->isSelected();
    }
    if (!parentSel  && !isSelected()) {
        setPrettyNormal();
    }
    QGraphicsPathItem::hoverLeaveEvent(event);
//    QGIPrimPath::hoverLeaveEvent(event);  //QGIPP::hoverleave will reset pretty to normal
}


//! this begins an edit session for a path described by pathPoints.
void QGEPath::startPathEdit(const std::vector<QPointF>& pathPoints)
{
//    Base::Console().message("QGEPath::startPathEdit()\n");
    inEdit(true);
    m_ghostPoints = pathPoints;
    showMarkers(m_ghostPoints);
}

void QGEPath::showMarkers(const std::vector<QPointF>& points)
{
//    Base::Console().message("QGEPath::showMarkers()\n");
    if (!inEdit()) {
        return;
    }

    if (points.empty()) {
        Base::Console().message("QGEP::showMarkers - no deltas\n");
        return;
    }

    clearMarkers();
//    dumpGhostPoints("QGEPath::showMarkers");

    int pointDx = 0;
    for (auto& p: points) {
        QGMarker* v = new QGMarker(pointDx);
        v->setFlag(QGraphicsItem::ItemIsMovable, true);
        v->setFlag(QGraphicsItem::ItemIsFocusable, true);
        v->setParentItem(this);
        QObject::connect(
            v, &QGMarker::dragFinished,
            this, &QGEPath::onDragFinished
           );
        QObject::connect(
            v, &QGMarker::dragging,
            this, &QGEPath::onDragging
           );
        QObject::connect(
            v, &QGMarker::doubleClick,
            this, &QGEPath::onDoubleClick
           );
        QObject::connect(
            v, &QGMarker::endEdit,
            this, &QGEPath::onEndEdit
           );
        v->setRadius(PreferencesGui::get3dMarkerSize());
        // v->setRadius(50.0);  // this make a huge marker
        v->setNormalColor(PreferencesGui::getAccessibleQColor(QColor(Qt::black)));
        v->setZValue(ZVALUE::VERTEX);
        v->setPos(p);
        v->show();

        m_markers.push_back(v);
        pointDx++;
    }
}

void QGEPath::clearMarkers()
{
//    Base::Console().message("QGEPath::clearMarkers()\n");
    if (m_markers.empty()) {
        return;
    }
    for (auto& m: m_markers) {
        if (m) {
            m->hide();
            QGraphicsScene* s = m->scene();
            if (s) {
                s->removeItem(m);           //should this be setParentItem(nullptr) instead??
            }
            delete m;
        }
    }
    m_markers.clear();
}

// end of node marker drag
void QGEPath::onDragFinished(QPointF dragEndPos, int markerIndex)
{
//    Base::Console().message("QGEPath::onDragFinished(%s, %d)\n",
//                            TechDraw::DrawUtil::formatVector(dragEndPos).c_str(),
//                            markerIndex);
    if ((int) m_ghostPoints.size() > markerIndex) {
        m_ghostPoints.at(markerIndex) = dragEndPos;
    }
    drawGhost();
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
//    Base::Console().message("QGEPath::onDoubleClick()\n");
    onEndEdit();
}

void QGEPath::onEndEdit()
{
//    Base::Console().message("QGEPath::onEndEdit()\n");
    if (m_ghost) {
        scene()->removeItem(m_ghost);   //stop ghost from messing up brect
    }
    inEdit(false);

    updateParent();         //Q_EMIT pointsUpdated(m_featDeltas) ==> onLineEditComplete  <<<
    clearMarkers();
}


//announce points editing is finished
void QGEPath::updateParent()
{
//    Base::Console().message("QGEPath::updateParent() - inEdit: %d pts: %d\n", inEdit(), m_ghostPoints.size());
//    dumpGhostPoints("QGEP::updateParent");
    QPointF attach = m_ghostPoints.front();
    if (!inEdit()) {
        Q_EMIT pointsUpdated(attach, m_ghostPoints);
    }
}

//! the ghost is the red line drawn when creating or editing the points
void QGEPath::drawGhost()
{
//    Base::Console().message("QGEPath::drawGhost()\n");
    if (!m_ghost->scene()) {
        m_ghost->setParentItem(this);
    }
    QPainterPath qpp;
    qpp.moveTo(m_ghostPoints.front());
    for (int i = 1; i < (int)m_ghostPoints.size(); i++) {
        qpp.lineTo(m_ghostPoints.at(i));
    }
    m_ghost->setPath(qpp);
    m_ghost->show();
}

void QGEPath::setStartAdjust(double adj)
{
    m_startAdj = Rez::guiX(adj);
}

void QGEPath::setEndAdjust(double adj)
{
    m_endAdj = Rez::guiX(adj);
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

 double QGEPath::getEdgeFuzz() const
{
    return PreferencesGui::edgeFuzz();
}

void QGEPath::dumpGhostPoints(const char* text)
{
    int idb = 0;
    for (auto& d: m_ghostPoints) {
        Base::Console().message("%s - point: %d %s\n", text,
                                 idb, TechDraw::DrawUtil::formatVector(d).c_str());
        idb++;
    }
}

void QGEPath::dumpMarkerPos(const char* text)
{
    int idb = 0;
    for (auto& m: m_markers) {
        Base::Console().message("QGEP - %s - markerPos: %d %s\n", text,
                                 idb, TechDraw::DrawUtil::formatVector(m->pos()).c_str());
        idb++;
    }
}

#include <Mod/TechDraw/Gui/moc_QGEPath.cpp>

