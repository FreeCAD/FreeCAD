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
class DrawView;
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

    virtual void drawBorder() override;
    virtual void updateView(bool update = false) override;

    virtual TechDraw::DrawLeaderLine* getFeature(void);

    void startPathEdit(void);
    void setArrows(std::vector<QPointF> pathPoints);

    void abandonEdit(void);
    void closeEdit(void);
    
    double getLineWidth(void);
    double getEdgeFuzz(void) const;



public Q_SLOTS:
    void onLineEditFinished(QPointF attach, std::vector<QPointF> deltas);    //QGEPath is finished editing points
    void select(bool state);
    void hover(bool state);
    virtual void onSourceChange(TechDraw::DrawView* newParent) override;

Q_SIGNALS:
    void editComplete(void);  //tell caller that edit session is finished

protected:
    virtual void draw() override;
    QPainterPath makeLeaderPath(std::vector<QPointF> qPoints);
    std::vector<QPointF> getWayPointsFromFeature(void);
    QPointF getAttachFromFeature(void);

    virtual QVariant itemChange( GraphicsItemChange change,
                                 const QVariant &value ) override;
    std::vector<QPointF> m_pathPoints;
    
    void saveState(void);
    void restoreState(void);

protected:
    QColor getNormalColor() override;

    QGraphicsItem* m_parentItem;
    QGIPrimPath* m_line;               //actual leader line
    double m_lineWidth;
    QColor m_lineColor;
    Qt::PenStyle m_lineStyle;
    QGIArrow* m_arrow1;
    QGIArrow* m_arrow2;

    QGEPath* m_editPath;               //line editor
    QColor m_editPathColor;
    Qt::PenStyle m_editPathStyle;

    bool m_hasHover;

    double m_saveX;
    double m_saveY;
    std::vector<Base::Vector3d> m_savePoints;

    bool m_blockDraw;    //prevent redraws while updating.
};

}

#endif // DRAWINGGUI_QGRAPHICSITEMLEADERLINE_H
