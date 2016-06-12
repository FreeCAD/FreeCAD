/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
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

#ifndef DRAWINGGUI_QGRAPHICSITEMFACE_H
#define DRAWINGGUI_QGRAPHICSITEMFACE_H

#include <Qt>
#include <QGraphicsItem>

QT_BEGIN_NAMESPACE
class QPainter;
class QStyleOptionGraphicsItem;
QT_END_NAMESPACE

namespace TechDrawGeometry {
class BaseGeom;
}

namespace TechDrawGui
{

class QGIFace : public QGraphicsPathItem
{
public:
    explicit QGIFace(int index = -1);
    ~QGIFace() {}

    enum {Type = QGraphicsItem::UserType + 104};
    int type() const { return Type;}
    QRectF boundingRect() const;
    QPainterPath shape() const;
    virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );

public:
    int getProjIndex() const { return projIndex; }

    void setHighlighted(bool state);
    void setPrettyNormal();
    void setPrettyPre();
    void setPrettySel();
    void setFill(QColor c, Qt::BrushStyle s);
    void setFill(QBrush b);
    void resetFill(void);

    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent * event);

protected:
    // Preselection events:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
    // Selection detection
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent * event);

protected:
    int projIndex;                              //index of face in Projection. -1 for SectionFace.
    bool isHighlighted;

private:
    QPen m_pen;
    QBrush m_brush;
    QColor m_colNormal;
    QColor m_colPre;
    QColor m_colSel;
    QColor m_colCurrent;
    QColor m_defNormal;                         //pen default normal color

    QColor m_colDefFill;                        //"no color"
    QColor m_colCurrFill;                       //current color
    QColor m_colNormalFill;
    Qt::BrushStyle m_styleDef;                  //default Normal fill fill style
    Qt::BrushStyle m_styleCurr;                 //current fill style
    Qt::BrushStyle m_styleNormal;               //Normal fill style
    Qt::BrushStyle m_styleSelect;               //Select/preSelect fill style
    QBrush m_brushNormal;
    QBrush m_brushPre;
    QBrush m_brushSel;
    QBrush m_brushDef;
    QBrush m_brushCurrent;
};

}
#endif // DRAWINGGUI_QGRAPHICSITEMFACE_H
