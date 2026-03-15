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

#pragma once

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <QGraphicsScene>
#include <QGraphicsSceneHoverEvent>
#include <QObject>

#include "QGIPrimPath.h"
#include "QGIUserTypes.h"
#include "QGIVertex.h"

namespace TechDrawGui
{

class QGIPrimPath;
class QGIVertex;
class QGIView;
class QGILeaderLine;

//! QGMarker provides movable symbols
class TechDrawGuiExport QGMarker : public QObject, public QGIVertex
{
    Q_OBJECT
public:
    explicit QGMarker(int idx);
    ~QGMarker() override = default;

    enum {Type = UserType::QGMarker};
    int type() const override { return Type;}

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    void keyPressEvent(QKeyEvent * event) override;

    void setRadius(double radius) override;

Q_SIGNALS:
    void dragging(QPointF pos, int idx);
    void dragFinished(QPointF pos, int idx);
    void doubleClick(QPointF pos, int idx);
    void endEdit();

protected:
    bool multiselectEligible() override { return false; }
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

private:
    bool m_dragging;

};

//******************************************************************************



class TechDrawGuiExport QGEPath : public QObject, public QGIPrimPath
{
    Q_OBJECT

public:
    explicit QGEPath();
    ~QGEPath() override = default;

    enum {Type = UserType::QGEPath};
    int type() const override { return Type;}
    QRectF boundingRect() const override;
    QPainterPath shape() const override;

    void inEdit(bool isInEdit) { m_inEdit = isInEdit; }
    bool inEdit() const { return m_inEdit; }
    void startPathEdit(const std::vector<QPointF>& pathPoints);

    void showMarkers(const std::vector<QPointF>& points);
    void clearMarkers();

    void setScale(double scale) { m_scale = scale; }
    double getScale() const { return m_scale; }

    void updateParent();
    void drawGhost();

    void dumpGhostPoints(const char* text);
    void dumpMarkerPos(const char* text);
    void setStartAdjust(double adjust);
    void setEndAdjust(double adjust);

public Q_SLOTS:
    void onDragFinished(QPointF pos, int index);
    void onDragging(QPointF pos, int index);
    void onDoubleClick(QPointF pos, int markerIndex);
    void onEndEdit();

Q_SIGNALS:
    void pointsUpdated(QPointF attach, std::vector<QPointF> deltas);

    void hover(bool state);
    void selected(bool state);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    double getEdgeFuzz() const;

private:
    std::vector<QPointF> m_ghostPoints;
    std::vector<QGMarker*> m_markers;

    double m_scale;
    bool m_inEdit;

    QGIPrimPath* m_ghost;

    double m_startAdj;
    double m_endAdj;
};

}