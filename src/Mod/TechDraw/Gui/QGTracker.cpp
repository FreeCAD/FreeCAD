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
# include <cassert>
# include <limits>

# include <QGraphicsScene>
# include <QGraphicsSceneHoverEvent>
# include <QGraphicsView>
# include <QKeyEvent>
# include <QPainter>
# include <QPainterPath>
# include <QStyleOptionGraphicsItem>
# include <QTransform>
#endif

#include <Base/Console.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/Preferences.h>

#include "PreferencesGui.h"
#include "QGTracker.h"
#include "QGIVertex.h"
#include "QGIView.h"
#include "QGIViewPart.h"
#include "QGSPage.h"
#include "Rez.h"
#include "ZVALUE.h"

using namespace TechDraw;
using namespace TechDrawGui;

QGTracker::QGTracker(QGSPage* inScene, TrackerMode m):
    m_sleep(false),
    m_qgParent(nullptr),
    m_lastClick(QPointF(std::numeric_limits<float>::max(),
                        std::numeric_limits<float>::max()))
{
    setTrackerMode(m);
    if (inScene) {
        inScene->addItem(this);
    } else {
        throw Base::ValueError("QGT::QGT() - passed scene is NULL\n");
    }

    setCacheMode(QGraphicsItem::NoCache);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setFlag(QGraphicsItem::ItemIsFocusable, true);
    setAcceptHoverEvents(true);
    setZValue(ZVALUE::TRACKER);
    setPos(0.0, 0.0);

    QColor tColor = getTrackerColor();
    QColor tailColor(Qt::blue);
    double tWeight = getTrackerWeight();
    setWidth(tWeight);
    setStyle(Qt::DashLine);
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
    m_track->setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
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

void QGTracker::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QPointF myScenePos = event->scenePos();
    if (!m_sleep) {
        double someLimit = Rez::guiX(1.0);
        QPointF manhat = myScenePos - m_lastClick;

        if (manhat.manhattanLength() >= someLimit) {
            if (event->button() == Qt::LeftButton)  {
                if (event->modifiers() & Qt::ControlModifier) {
                    myScenePos = snapToAngle(myScenePos);
                }
                onMousePress(myScenePos);

            } else if (event->button() == Qt::RightButton)  {
                terminateDrawing();
            }
        }
    }
    m_lastClick = myScenePos;
    QGIPrimPath::mousePressEvent(event);
}

void QGTracker::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    m_lastClick = event->scenePos();
    QGIPrimPath::mouseReleaseEvent(event);
}

//TODO: fix this to handle click-release-doubleclick-release nonsense.
//      can generate two add points
void QGTracker::mouseDoubleClickEvent(QGraphicsSceneMouseEvent * event)
{
    if (!m_sleep) {
        onDoubleClick(event->scenePos());
    }
    m_lastClick = event->scenePos();
    QGIPrimPath::mouseDoubleClickEvent(event);
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
    if (event->key() == Qt::Key_Escape) {
        terminateDrawing();
    }
    QGIPrimPath::keyPressEvent(event);
}


// ?? why does this method exist? and why isn't it called changeCursor?
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
    // If no point selected yet, snapping has no sense
    if (m_points.empty())
        return dumbPt;

    QPointF result(dumbPt);
    double angleIncr = std::numbers::pi / 8.0;   //15*
    //mirror last clicked point and event point to get sensible coords
    QPointF last(m_points.back().x(), -m_points.back().y());
    QPointF pt(dumbPt.x(), -dumbPt.y());

    //let's do math with sensible coord system.
    QPointF qVec = last - pt;    //vec from end of track to end of tail
    double actual = atan2(-qVec.y(), qVec.x());
    if (actual < 0.0) {
        actual = (2 * std::numbers::pi) + actual;          //map to +ve angle
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

        double vecLen = sqrt(pow(qVec.x(), 2) + pow(qVec.y(), 2));
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
        //just return pos to caller
        getPickedQGIV(pos);
        setCursor(Qt::CrossCursor);  //why cross??

        if (mode == TrackerMode::Point) {
            setPoint(m_points);  //first point is mouse click scene pos
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
    Q_UNUSED(pos);
    TrackerMode mode = getTrackerMode();
    if (mode == TrackerMode::Point) {
        setPoint(m_points);
    }
    terminateDrawing();
}

void QGTracker::getPickedQGIV(QPointF pos)
{
    setVisible(false);
    m_qgParent = nullptr;
    QList<QGraphicsView *> views = scene()->views();
    QGraphicsView* ourView = views.front();  //only 1 view / 1 scene / 1 mdipage
    QTransform viewXForm = ourView->transform();
    QGraphicsItem* pickedItem = scene()->itemAt(pos, viewXForm);
    if (pickedItem) {
        QGraphicsItem* topItem = pickedItem->topLevelItem();
        if (topItem != pickedItem) {
            pickedItem = topItem;
        }                               //pickedItem sb a QGIV
        QGIView* qgParent = dynamic_cast<QGIView*>(pickedItem);
        if (qgParent) {
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

QPainterPath QGTracker::shape() const
{
    QPainterPath result;
    result.addRect(boundingRect());
    return result;
}

//***************
//actual art routines
void QGTracker::drawTrackLine(QPointF pos)
{
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
    m_segEnd = pos;
    QPainterPath tail;
    if (!m_points.empty()) {
        m_segBegin = m_points.front();   //sb front? 1st point picked??
        QRectF rect(m_segBegin, m_segEnd);
        tail.addRect(rect);
        m_track->setPath(tail);
        m_track->show();
    }
}

void QGTracker::drawTrackCircle(QPointF pos)
{
    QPointF circum = pos;
    QPainterPath tail;
    if (!m_points.empty()) {
        QPointF center = m_points.front();             //not nec (0, 0);
        QPointF ray = circum - center;
        double radius =  sqrt(pow(ray.x(), 2.0) + pow(ray.y(), 2.0));
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
    if (pts.empty()) {
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
    if (pts.empty()) {
        return;
    }
    prepareGeometryChange();
    QPainterPath newPath;
    QPointF start = pts.front();
    QPointF end   = pts.back();
    QRectF rect(start, end);
    newPath.addRect(rect);
    setPath(newPath);
    setPrettyNormal();
}

void QGTracker::setCircleFromPoints(std::vector<QPointF> pts)
{
    if (pts.empty()) {
        return;
    }
    prepareGeometryChange();
    QPainterPath newPath;
    QPointF center = pts.front();
    QPointF circum   = pts.back();
    QPointF ray    = circum - center;
    double radius =  sqrt(pow(ray.x(), 2.0) + pow(ray.y(), 2.0));
    newPath.addEllipse(center, radius, radius);
    setPath(newPath);
    setPrettyNormal();
}

void QGTracker::setPoint(std::vector<QPointF> pts)
{
    if (pts.empty()) {
        Base::Console().message("QGTracker::setPoint - no pts!\n");
        return;
    }
    prepareGeometryChange();

    auto point = new QGIVertex(-1);
    point->setParentItem(this);
    point->setPos(pts.front());
    point->setRadius(static_cast<QGIViewPart *>(m_qgParent)->getVertexSize());
    point->setNormalColor(Qt::blue);
    point->setFillColor(Qt::blue);
    point->setPrettyNormal();
    point->setZValue(ZVALUE::VERTEX);
}

std::vector<Base::Vector3d> QGTracker::convertPoints()
{
    std::vector<Base::Vector3d> result;
    for (auto& p: m_points) {
        Base::Vector3d v(p.x(), p.y(), 0.0);
        result.push_back(v);
    }
    return result;
}

void QGTracker::terminateDrawing()
{
    m_track->hide();
    setCursor(Qt::ArrowCursor);
    Q_EMIT drawingFinished(m_points, m_qgParent);
}

void QGTracker::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    painter->drawPath(shape());

    QGIPrimPath::paint(painter, &myOption, widget);
}

QColor QGTracker::getTrackerColor()
{
    Base::Color trackColor = Base::Color((uint32_t) Preferences::getPreferenceGroup("Tracker")->GetUnsigned("TrackerColor", 0xFF000000));
    return PreferencesGui::getAccessibleQColor(trackColor.asValue<QColor>());
}

double QGTracker::getTrackerWeight()
{
    return Preferences::getPreferenceGroup("Tracker")->GetFloat("TrackerWeight", 4.0);
}

#include <Mod/TechDraw/Gui/moc_QGTracker.cpp>
