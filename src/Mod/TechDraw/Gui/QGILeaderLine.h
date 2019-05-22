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

#ifndef DRAWINGGUI_QGRAPHICSITEMLEADERLINE_H
#define DRAWINGGUI_QGRAPHICSITEMLEADERLINE_H

#include <QObject>
#include <QGraphicsView>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsItem>
#include <QGraphicsObject>
#include <QPainterPath>
#include <QColor>
#include <QFont>
#include <QPointF>

#include <Base/Vector3D.h>
#include "QGIView.h"

namespace TechDraw {
class DrawLeaderLine;
}

namespace TechDrawGui
{
class QGIPrimPath;
class QGIArrow;
class QGEPath;


//*******************************************************************

class TechDrawGuiExport QGILeaderLine : public QGIView
{
    Q_OBJECT

public:
    enum {Type = QGraphicsItem::UserType + 232};

    explicit QGILeaderLine(QGraphicsItem* myParent = nullptr,
                           TechDraw::DrawLeaderLine* lead = nullptr);
    ~QGILeaderLine() = default;

    int type() const override { return Type;}
    virtual void paint( QPainter * painter,
                        const QStyleOptionGraphicsItem * option,
                        QWidget * widget = 0 ) override;
    virtual QRectF boundingRect() const override;
    virtual QPainterPath shape(void) const override;
    double getEdgeFuzz(void) const;

    virtual void drawBorder() override;
    virtual void updateView(bool update = false) override;

    virtual TechDraw::DrawLeaderLine* getFeature(void);
    QGEPath* getLeaderLine(void) { return m_line; }
    std::vector<QPointF> convertWaypoints(void);
    void startPathEdit(void);
    void setArrows(std::vector<QPointF> pathPoints);

    void abandonEdit(void);
    double getScale(void);

public Q_SLOTS:
    void onLineEditFinished(std::vector<QPointF> pts);    //QGEPath is finished editing points
    void select(bool state);
    void hover(bool state);

Q_SIGNALS:
    void editComplete(std::vector<QPointF> pts, QGIView* parent);  //tell caller that edit session is finished

protected:
    virtual void draw() override;
    virtual QVariant itemChange( GraphicsItemChange change,
                                 const QVariant &value ) override;
    std::vector<QPointF> m_pathPoints;
    Base::Vector3d m_attachPoint;

protected:
    QColor getNormalColor() override;

    QGraphicsItem* m_parentItem;
    QGEPath* m_line;
    QGIArrow* m_arrow1;
    QGIArrow* m_arrow2;
    double m_lineWidth;
    QColor m_lineColor;
    Qt::PenStyle m_lineStyle;
    bool m_hasHover;
};

}

#endif // DRAWINGGUI_QGRAPHICSITEMLEADERLINE_H
