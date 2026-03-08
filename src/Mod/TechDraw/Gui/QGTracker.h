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

#pragma once

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <QGraphicsItem>
#include <QPen>

QT_BEGIN_NAMESPACE
class QPainter;
class QStyleOptionGraphicsItem;
QT_END_NAMESPACE

#include <Base/Parameter.h>
#include <Base/Vector3D.h>

#include "QGIUserTypes.h"
#include "QGIPrimPath.h"

namespace TechDrawGui
{

class QGSPage;
class QGIView;

enum class TrackerAction
{
    PICK = 0,
    EDIT = 1,
    CANCEL = 2,
    CANCELEDIT = 3,
    FINISHED = 4,
    SAVE = 5
};

class TechDrawGuiExport QGTracker : public QObject, public QGIPrimPath
{
    Q_OBJECT
public:
    enum class TrackerMode {
        None,
        Line,
        Circle,
        Rectangle,
        Point
    };

    explicit QGTracker(QGSPage* scene = nullptr, TrackerMode m = TrackerMode::None);
    ~QGTracker() override = default;

    enum {Type = UserType::QGTracker};

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

    void setOwnerQView(QGIView* owner) { m_qgParent = owner; }

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
    std::vector<QPointF> m_points;
    bool m_sleep;
    QGIView* m_qgParent;
    TrackerMode m_trackerMode;
    QPen m_tailPen;
    QPointF m_lastClick;
};

} // namespace