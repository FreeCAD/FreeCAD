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

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <QColor>
#include <QGraphicsItem>
#include <QPainterPath>
#include <QPointF>
#include <QStyleOptionGraphicsItem>

#include <Base/Vector3D.h>

#include "QGIView.h"


namespace TechDraw
{
class DrawLeaderLine;
class DrawView;
}// namespace TechDraw

namespace TechDrawGui
{
class QGIPrimPath;
class QGIArrow;
class QGEPath;


//*******************************************************************

class TechDrawGuiExport QGILeaderLine: public QGIView
{
    Q_OBJECT

public:
    enum
    {
        Type = QGraphicsItem::UserType + 232
    };

    explicit QGILeaderLine();
    ~QGILeaderLine() override = default;

    int type() const override
    {
        return Type;
    }
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget = nullptr) override;
    QRectF boundingRect() const override;

    void drawBorder() override;
    void updateView(bool update = false) override;

    virtual TechDraw::DrawLeaderLine* getFeature();

    void startPathEdit();
    void setArrows(std::vector<QPointF> pathPoints);

    void abandonEdit();
    void closeEdit();

    double getLineWidth();
    double getEdgeFuzz() const;

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

    void setPrettyNormal();
    void setPrettyPre();
    void setPrettySel();

    void setLeaderFeature(TechDraw::DrawLeaderLine* feat);

public Q_SLOTS:
    void onLineEditFinished(QPointF tipDisplace,
                            std::vector<QPointF> points);//QGEPath is finished editing points
    void onSourceChange(TechDraw::DrawView* newParent) override;

Q_SIGNALS:
    void editComplete();//tell caller that edit session is finished

protected:
    void draw() override;
    QPainterPath makeLeaderPath(std::vector<QPointF> qPoints);
    std::vector<QPointF> getWayPointsFromFeature();
    QPointF getAttachFromFeature();

    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

    void saveState();
    void restoreState();

    QColor prefNormalColor();
    void setNormalColorAll();

private:
    std::vector<QPointF> m_pathPoints;
    QGraphicsItem* m_parentItem;
    QGIPrimPath* m_line;//actual leader line
    QColor m_lineColor;
    Qt::PenStyle m_lineStyle;
    QGIArrow* m_arrow1;
    QGIArrow* m_arrow2;

    QGEPath* m_editPath;//line editor
    QColor m_editPathColor;

    bool m_hasHover;

    double m_saveX;
    double m_saveY;
    std::vector<Base::Vector3d> m_savePoints;

    bool m_blockDraw;//prevent redraws while updating.
};

}// namespace TechDrawGui

#endif// DRAWINGGUI_QGRAPHICSITEMLEADERLINE_H
