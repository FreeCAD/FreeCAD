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
#include <assert.h>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsSceneHoverEvent>
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPathStroker>
#include <QStyleOptionGraphicsItem>
#include <QKeyEvent>
#include <QTransform>
#endif

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>

#include <Mod/TechDraw/App/DrawUtil.h>

#include "Rez.h"
#include "ZVALUE.h"
#include "DrawGuiUtil.h"
#include "QGIView.h"
#include "QGTracker.h"

using namespace TechDrawGui;

QGTracker::QGTracker(QGraphicsScene* inScene, TrackerMode m):
    m_width(0),
    m_sleep(false),
    m_qgParent(nullptr)
{
    setTrackerMode(m);
    if (inScene != nullptr) {
        inScene->addItem(this);
    } else {
        throw Base::ValueError("QGT::QGT() - passed scene is NULL\n");
    }

    setCacheMode(QGraphicsItem::NoCache);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges,true);
    setFlag(QGraphicsItem::ItemIsFocusable, true);
    setAcceptHoverEvents(true);
    setZValue(ZVALUE::TRACKER);
    setPos(0.0,0.0);

    QColor tColor = getTrackerColor();
    QColor tailColor(Qt::blue);
    double tWeight = getTrackerWeight();
    setWidth(tWeight);
    setStyle(Qt::DashLine);
//    setStyle(Qt::SolidLine);
    setNormalColor(tailColor);
    setPrettyNormal();

    //m_track is the new segment of the line.
    m_track = new QGraphicsPathItem();
    m_track->setParentItem(this);
    m_trackPen.setColor(tColor);
    m_trackPen.setWidthF(tWeight);
    m_trackPen.setStyle(Qt::DashLine);
    m_track->setPen(m_trackPen);
    m_track->setBrush(QBrush(Qt::NoBrush));
    m_track->setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);
    m_track->setFlag(QGraphicsItem::ItemSendsGeometryChanges,true);
    m_track->setFocusProxy(this);

    setHandlesChildEvents(true);
    setVisible(true);
    setEnabled(true);
    setFocus();
    scene()->setFocusItem(this);
}

QGTracker::~QGTracker()
{
}

QVariant QGTracker::itemChange(GraphicsItemChange change, const QVariant &value)
{
    return QGIPrimPath::itemChange(change, value);
}

void QGTracker::mousePressEvent(QGraphicsSceneMouseEvent *event)
{ 
    if (!m_sleep) {
        QPointF scenePos(event->scenePos());
        if (event->button() == Qt::LeftButton)  {
            if (event->modifiers() & Qt::ControlModifier) {
                scenePos = snapToAngle(scenePos);
            }
            onMousePress(scenePos);

        } else if (event->button() == Qt::RightButton)  {
            terminateDrawing();
        }
    }
    QGIPrimPath::mousePressEvent(event);
}

void QGTracker::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QGIPrimPath::mouseReleaseEvent(event);
}

void QGTracker::mouseDoubleClickEvent(QGraphicsSceneMouseEvent * event)
{
//    Base::Console().Message("QGT::mouseDoubleClickEvent()\n");
    if (!m_sleep) {
        onDoubleClick(event->scenePos());
    }
    QGIPrimPath::mouseDoubleClickEvent(event);
}

void QGTracker::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    QGIPrimPath::hoverEnterEvent(event);
}

void QGTracker::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    QGIPrimPath::hoverLeaveEvent(event);
}

void QGTracker::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    if (!m_sleep) {
        QPointF scenePos(event->scenePos());
        if (event->modifiers() & Qt::ControlModifier) {
            scenePos = snapToAngle(scenePos);
        }
        onMouseMove(scenePos);
    }
    QGIPrimPath::hoverMoveEvent(event);
}

void QGTracker::keyPressEvent(QKeyEvent * event)
{
//    Base::Console().Message("QGT::keyPressEvent()\n");
    if (event->key() == Qt::Key_Escape) {
        terminateDrawing();
    }
    QGIPrimPath::keyPressEvent(event);
}

void QGTracker::sleep(bool b)
{
    m_sleep = b;
    if (m_sleep) {
        setCursor(Qt::ArrowCursor);
    } else {
        setCursor(Qt::CrossCursor);
    }
}

QPointF QGTracker::snapToAngle(QPointF dumbPt)
{
    QPointF result(dumbPt);
    double angleIncr = M_PI / 8.0;   //15*
    //mirror last clicked point and event point to get sensible coords
    QPointF last(m_points.back().x(), -m_points.back().y());
    QPointF pt(dumbPt.x(),-dumbPt.y());

    //let's do math with sensible coord system.
    QPointF qVec = last - pt;    //vec from end of track to end of tail
    double actual = atan2(-qVec.y(), qVec.x());
    if (actual < 0.0) {
        actual = (2 * M_PI) + actual;          //map to +ve angle
    }

    double intPart;
    double remain = modf(actual/angleIncr, &intPart);
    if (!TechDraw::DrawUtil::fpCompare(remain, 0.0)) {   //not n*15
        double low = intPart * angleIncr;
        double high = (intPart + 1) * angleIncr;
        double lowGap = actual - low;
        double highGap = high - actual;
        if (lowGap > highGap) {
            actual = high;
        } else {
            actual = low;
        }

        double vecLen = sqrt(pow(qVec.x(),2) + pow(qVec.y(),2));
        double xPart = vecLen * cos(actual) * -1.0;   //actual is not in Qt coords
        double yPart = vecLen * sin(actual) * -1.0;
        result = QPointF(last.x() + xPart, (-last.y()) + yPart);    //back to silly qt coords!
    }
    return result;
}

//mouse event reactions
void QGTracker::onMousePress(QPointF pos) 
{
    m_points.push_back(pos);
    TrackerMode mode = getTrackerMode();
    if (m_points.size() > 1) {
        switch (mode) {
            case TrackerMode::None:
                break;
            case TrackerMode::Line:
                setPathFromPoints(m_points);
                break;
            case TrackerMode::Rectangle:
                setSquareFromPoints(m_points);
                break;
            case TrackerMode::Circle:
                setCircleFromPoints(m_points);
                break;
            case TrackerMode::Point:
                //do nothing
                break;
        }
    } else if (m_points.size() == 1) {   //first point selected   
        getPickedQGIV(pos);
        setCursor(Qt::CrossCursor);
        Q_EMIT qViewPicked(pos, m_qgParent);
        if (mode == TrackerMode::Point) {
            setPoint(m_points);
            terminateDrawing();
        }
    }
    
    if ( (m_points.size() == 2) &&
        ( (getTrackerMode() == TrackerMode::Circle) ||
          (getTrackerMode() == TrackerMode::Rectangle) ) ) {          //only 2 points for square/circle
        terminateDrawing();
    }
}

void QGTracker::onMouseMove(QPointF pos) 
{
    //check distance moved? 
    TrackerMode m = getTrackerMode();
    switch (m) {
        case TrackerMode::None:
            break;
        case TrackerMode::Line:
            drawTrackLine(pos);
            break;
        case TrackerMode::Rectangle:
            drawTrackSquare(pos);
            break;
        case TrackerMode::Circle:
            drawTrackCircle(pos);
            break;
        case TrackerMode::Point:
            //don't do anything here.
            break;
    }
}

void QGTracker::onDoubleClick(QPointF pos) 
{
//    Base::Console().Message("QGTracker::onDoubleClick()\n");
    Q_UNUSED(pos);
    terminateDrawing();
}

void QGTracker::getPickedQGIV(QPointF pos) 
{
    setVisible(false);
    m_qgParent = nullptr;
    QList<QGraphicsView *> views = scene()->views();
    QGraphicsView* ourView = views.front();  //only 1 view / 1 scene / 1 mdipage
    QTransform viewXForm = ourView->transform();
    QGraphicsItem* pickedItem = scene()->itemAt(pos,viewXForm);
    if (pickedItem != nullptr) {
        QGraphicsItem* topItem = pickedItem->topLevelItem();
        if (topItem != pickedItem) {
            pickedItem = topItem;
        }                               //pickedItem sb a QGIV
        QGIView* qgParent = dynamic_cast<QGIView*>(pickedItem);
        if (qgParent != nullptr) {
            m_qgParent = qgParent;
        }
    }
    setVisible(true);
    return;
}

QRectF QGTracker::boundingRect() const
{
    return scene()->sceneRect();
}

QPainterPath QGTracker::shape(void) const
{
    QPainterPath result;
    result.addRect(boundingRect());
    return result;
}
 
//*************** 
//actual art routines 
void QGTracker::drawTrackLine(QPointF pos)
{
//    Base::Console().Message("QGTracker::drawTrackLine()\n");
    m_segEnd = pos;
    QPainterPath tail;
    if (!m_points.empty()) {
        m_segBegin = m_points.back();
        tail.moveTo(m_segBegin);
        tail.lineTo(m_segEnd);
        m_track->setPath(tail);
        m_track->show();
    }
}

void QGTracker::drawTrackSquare(QPointF pos)
{
//    Base::Console().Message("QGTracker::drawTrackSquare()\n");
    m_segEnd = pos;
    QPainterPath tail;
    if (!m_points.empty()) {
        m_segBegin = m_points.front();   //sb front? 1st point picked??
        QRectF rect(m_segBegin,m_segEnd);
        tail.addRect(rect);
        m_track->setPath(tail);
        m_track->show();
    }
}

void QGTracker::drawTrackCircle(QPointF pos)
{
//    Base::Console().Message("QGTracker::drawTrackCircle() - m_points: %d \n", m_points.size());
    QPointF circum = pos;
    QPainterPath tail;
    if (!m_points.empty()) {
        QPointF center = m_points.front();             //not nec (0,0);
        QPointF ray = circum - center;
        double radius =  sqrt(pow(ray.x(),2.0) + pow(ray.y(),2.0));
        tail.addEllipse(center, radius, radius);
        m_track->setPath(tail);
    }
}

//don't need this since Tracker ends as soon as 1st point is picked
void QGTracker::drawTrackPoint(QPointF pos)
{
    Q_UNUSED(pos);
}

void QGTracker::setPathFromPoints(std::vector<QPointF> pts)
{
//    Base::Console().Message("QGTracker::setPathFromPoints()\n");
    if (pts.empty()) {
        Base::Console().Log("QGTracker::setPathFromPoints - no pts!\n");
        return;
    }
    prepareGeometryChange();
    QPainterPath newPath;
    newPath.moveTo(pts.front());
    auto it = pts.begin() + 1;
    for (; it != pts.end(); it++) {
        newPath.lineTo(*it);
    }
    setPath(newPath);
    setPrettyNormal();
}
void QGTracker::setSquareFromPoints(std::vector<QPointF> pts)
{
//    Base::Console().Message("QGTracker::setSquareFromPoints()\n");
    if (pts.empty()) {
        Base::Console().Log("QGTracker::setSquareFromPoints - no pts!\n");
        return;
    }
    prepareGeometryChange();
    QPainterPath newPath;
    QPointF start = pts.front();
    QPointF end   = pts.back();
    QRectF rect(start,end);
    newPath.addRect(rect);
    setPath(newPath);
    setPrettyNormal();
}

void QGTracker::setCircleFromPoints(std::vector<QPointF> pts)
{
//    Base::Console().Message("QGTracker::setCircleFromPoints()\n");
    if (pts.empty()) {
        Base::Console().Log("QGTracker::setCircleFromPoints - no pts!\n");
        return;
    }
    prepareGeometryChange();
    QPainterPath newPath;
    QPointF center = pts.front();
    QPointF circum   = pts.back();
    QPointF ray    = circum - center;
    double radius =  sqrt(pow(ray.x(),2.0) + pow(ray.y(),2.0));
    newPath.addEllipse(center, radius, radius);
    setPath(newPath);
    setPrettyNormal();
}

void QGTracker::setPoint(std::vector<QPointF> pts)
{
//    Base::Console().Message("QGTracker::setPoint()\n");
    if (pts.empty()) {
        Base::Console().Message("QGTracker::setPoint - no pts!\n");
        return;
    }
    prepareGeometryChange();
    QPainterPath newPath;
    QPointF center = pts.front();
    double radius = 5.0;
    newPath.addEllipse(center, radius, radius);
    setPath(newPath);
    setPrettyNormal();
}

std::vector<Base::Vector3d> QGTracker::convertPoints(void)
{
    std::vector<Base::Vector3d> result;
    for (auto& p: m_points) {
        Base::Vector3d v(p.x(),p.y(),0.0);
        result.push_back(v);
    }
    return result;
}

void QGTracker::terminateDrawing(void)
{
//    Base::Console().Message("QGTracker::terminateDrawing()\n");
    m_track->hide();
    setCursor(Qt::ArrowCursor);
    Q_EMIT drawingFinished(m_points, m_qgParent);
}

void QGTracker::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

//    painter->drawRect(boundingRect());          //good for debugging
    painter->drawPath(shape());

    QGIPrimPath::paint(painter, &myOption, widget);
}

QColor QGTracker::getTrackerColor()
{
    QColor result;
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
                                                    GetGroup("Preferences")->GetGroup("Mod/TechDraw/Tracker");
    App::Color trackColor = App::Color((uint32_t) hGrp->GetUnsigned("TrackerColor", 0xFF000000));
    result = trackColor.asValue<QColor>();
    return result;
}

double QGTracker::getTrackerWeight()
{
    double result = 1.0;
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
                                                    GetGroup("Preferences")->GetGroup("Mod/TechDraw/Tracker");
    result = hGrp->GetFloat("TrackerWeight", 4.0);

    return result;
}

#include <Mod/TechDraw/Gui/moc_QGTracker.cpp>
