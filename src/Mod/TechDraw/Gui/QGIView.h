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

#ifndef DRAWINGGUI_QGRAPHICSITEMVIEW_H
#define DRAWINGGUI_QGRAPHICSITEMVIEW_H

#include <QGraphicsItemGroup>
#include <QObject>
#include <App/PropertyLinks.h>

#include "../App/DrawView.h"

QT_BEGIN_NAMESPACE
class QGraphicsScene;
class QGraphicsSceneMouseEvent;
QT_END_NAMESPACE

namespace TechDrawGui
{
class QGCustomBorder;
class QGCustomLabel;

class TechDrawGuiExport  QGIView : public QObject, public QGraphicsItemGroup
{
    Q_OBJECT

public:
    QGIView(const QPoint &position, QGraphicsScene *scene);
    virtual ~QGIView() = default;

    enum {Type = QGraphicsItem::UserType + 101};
    int type() const { return Type;}

    const char * getViewName() const;
    void setViewFeature(TechDraw::DrawView *obj);
    TechDraw::DrawView * getViewObject() const;

    virtual void toggleBorder(bool state = true);
    virtual void drawBorder(void);

    /// Methods to ensure that Y-Coordinates are orientated correctly.
    void setPosition(qreal x, qreal y);
    inline qreal getY() { return y() * -1; }
    bool isInnerView() { return m_innerView; }
    void isInnerView(bool state) { m_innerView = state; }
    double getYInClip(double y);

    void alignTo(QGraphicsItem*, const QString &alignment);
    void setLocked(bool state = true) { locked = true; }

    virtual void toggleCache(bool state);
    virtual void updateView(bool update = false);
    virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );
    //virtual QPainterPath shape(void) const;

    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent * event);

Q_SIGNALS:
    void dirty();

protected:
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);
    // Mouse handling
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent * event );
    virtual void mousePressEvent(QGraphicsSceneMouseEvent * event);
    // Preselection events:
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
    virtual QRectF customChildrenBoundingRect(void);

    TechDraw::DrawView *viewObj;
    std::string viewName;

    QHash<QString, QGraphicsItem*> alignHash;
    bool locked;
    bool borderVisible;
    bool m_innerView;                                                  //View is inside another View

    QPen m_pen;
    QBrush m_brush;
    QColor m_colCurrent;
    QColor m_colNormal;
    QColor m_colPre;
    QColor m_colSel;
    QFont m_font;
    QGCustomLabel* m_label;
    QGCustomBorder* m_border;
    QPen m_decorPen;
};

} // namespace MDIViewPageGui

#endif // DRAWINGGUI_QGRAPHICSITEMVIEW_H
