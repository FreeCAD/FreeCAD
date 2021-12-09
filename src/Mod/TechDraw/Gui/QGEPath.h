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

#ifndef TECHDRAWGUI_EDITABLEPATH_H
#define TECHDRAWGUI_EDITABLEPATH_H

#include <QGraphicsScene>
#include <QGraphicsSceneHoverEvent>
#include <QMouseEvent>
#include <QObject>

#include <Base/Vector3D.h>
#include "QGIVertex.h"
#include "QGIPrimPath.h"

namespace TechDrawGui
{

class QGIPrimPath;
class QGIVertex;
class QGIView;

class TechDrawGuiExport QGMarker : public QObject, public QGIVertex
{
    Q_OBJECT
public:
    explicit QGMarker(int idx);
    virtual ~QGMarker(void) {}

    enum {Type = QGraphicsItem::UserType + 302};
    int type() const override { return Type;}
    virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 ) override;

    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void keyPressEvent(QKeyEvent * event) override;

    virtual void setRadius(float r) override;

Q_SIGNALS:
    void dragging(QPointF pos, int idx);
    void dragFinished(QPointF pos, int idx);
    void doubleClick(QPointF pos, int idx);
    void endEdit(void);

protected:
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private:
    bool m_dragging;

};

//******************************************************************************

class TechDrawGuiExport QGEPath : public QObject, public QGIPrimPath
{
    Q_OBJECT
public:
    explicit QGEPath(QGILeaderLine* leader);
    virtual ~QGEPath() {}

    enum {Type = QGraphicsItem::UserType + 301};
    int type() const override { return Type;}
    virtual QRectF boundingRect() const override;
    virtual QPainterPath shape() const override;
    virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 ) override;

    void inEdit(bool b) { m_inEdit = b; }
    bool inEdit(void)   { return m_inEdit; }
    void startPathEdit(std::vector<QPointF> pathPoints);

    void showMarkers(std::vector<QPointF> points);
    void clearMarkers();

    std::vector<QPointF> getDeltasFromLeader(void);

    void setScale(double s) { m_scale = s; }
    double getScale(void)   { return m_scale; }

    void setPoints(std::vector<QPointF> pts) { m_ghostPoints = pts; }

    void updateParent();
    void drawGhost(void);

    void dumpGhostPoints(const char* text);
    void dumpMarkerPos(const char* text);
    void setStartAdjust(double adjust);
    void setEndAdjust(double adjust);

public Q_SLOTS:
    void onDragFinished(QPointF pos, int index);
    void onDragging(QPointF pos, int index);
    void onDoubleClick(QPointF pos, int markerIndex);
    void onEndEdit(void);

Q_SIGNALS:
    void pointsUpdated(QPointF attach, std::vector<QPointF> deltas);

    void hover(bool state);
    void selected(bool state);

protected:
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    double getEdgeFuzz(void) const;

    std::vector<QPointF> m_ghostPoints;
    std::vector<QGMarker*> m_markers;

    double m_scale;
    bool m_inEdit;

    QGILeaderLine* m_parentLeader;
    QGIPrimPath* m_ghost;

    double m_startAdj;
    double m_endAdj;
};

}

#endif // TECHDRAWGUI_EDITABLEPATH_H
