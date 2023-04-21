/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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

#ifndef DRAWINGGUI_TRACKER_H
#define DRAWINGGUI_TRACKER_H

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <QGraphicsItem>
#include <QPen>

QT_BEGIN_NAMESPACE
class QPainter;
class QStyleOptionGraphicsItem;
QT_END_NAMESPACE

#include <Base/Parameter.h>
#include <Base/Vector3D.h>

#include "QGIPrimPath.h"

namespace TechDrawGui
{

class QGSPage;
class QGIView;

//TODO: make this a proper enum
static constexpr int TRACKERPICK(0);
static constexpr int TRACKEREDIT(1);
static constexpr int TRACKERCANCEL(2);
static constexpr int TRACKERCANCELEDIT(3);
static constexpr int TRACKERFINISHED(4);
static constexpr int TRACKERSAVE(5);

class TechDrawGuiExport QGTracker : public QObject, public QGIPrimPath
{
    Q_OBJECT
public:
    enum TrackerMode { None, Line, Circle, Rectangle, Point };

    explicit QGTracker(QGSPage* scene = nullptr, QGTracker::TrackerMode m = QGTracker::TrackerMode::None);
    ~QGTracker() override;


    enum {Type = QGraphicsItem::UserType + 210};

    int type() const override { return Type;}
    void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = nullptr ) override;
    QPainterPath shape() const override;
    QRectF boundingRect() const override;

    void onMousePress(QPointF);
    void onMouseMove(QPointF pos);
    void onDoubleClick(QPointF pos);
    void drawTrackLine(QPointF pos);
    void drawTrackSquare(QPointF pos);
    void drawTrackCircle(QPointF pos);
    void drawTrackPoint(QPointF pos);
    void setPathFromPoints(std::vector<QPointF> pts);
    void setSquareFromPoints(std::vector<QPointF> pts);
    void setCircleFromPoints(std::vector<QPointF> pts);
    void setPoint(std::vector<QPointF> pts);
    std::vector<Base::Vector3d> convertPoints();
    void terminateDrawing();
    void sleep(bool b);
    TrackerMode getTrackerMode() { return m_trackerMode; }
    void setTrackerMode(TrackerMode m) { m_trackerMode = m; }
    QPointF snapToAngle(QPointF pt);

Q_SIGNALS:
    void drawingFinished(std::vector<QPointF> pts, TechDrawGui::QGIView* qgParent);
    void qViewPicked(QPointF pos, TechDrawGui::QGIView* qgParent);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    void keyPressEvent(QKeyEvent * event) override;

    void getPickedQGIV(QPointF pos);

    QColor getTrackerColor();
    double getTrackerWeight();

private:
    QGraphicsPathItem* m_track;
    QPointF m_segBegin;
    QPointF m_segEnd;
    std::vector<QPointF> m_points;
    bool m_sleep;
    QGIView* m_qgParent;
    TrackerMode m_trackerMode;
    QPen m_trackPen;
    QPen m_tailPen;
    QPointF m_lastClick;
};

} // namespace

#endif // DRAWINGGUI_TRACKER_H
