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

#ifndef DRAWINGGUI_QGRAPHICSITEMEDGE_H
#define DRAWINGGUI_QGRAPHICSITEMEDGE_H

# include <QGraphicsItem>

QT_BEGIN_NAMESPACE
class QPainter;
class QStyleOptionGraphicsItem;
QT_END_NAMESPACE

namespace TechDrawGeometry {
class BaseGeom;
}

namespace TechDrawGui
{

class TechDrawGuiExport QGIEdge : public QGraphicsPathItem
{
public:
    explicit QGIEdge(int index);
    ~QGIEdge() {}

    enum {Type = QGraphicsItem::UserType + 103};

    int type() const { return Type;}
    QRectF boundingRect() const;
    QPainterPath shape() const;
    virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );

    //TODO: edge doesn't need ref to 3D, just index in projection.  links to 3D handled in Features.
    int getReference() const { return reference; }
    void setReference(int ref) {reference = ref; }
    int getProjIndex() const { return projIndex; }

    void setHighlighted(bool state);
    void setCosmetic(bool state);
    void setStrokeWidth(float width);
    void setPrettyNormal();
    void setPrettyPre();
    void setPrettySel();
    void setHiddenEdge(bool b);
    bool getHiddenEdge() { return(isHiddenEdge); }
    void setSmoothEdge(bool b) { isSmoothEdge = b; }
    bool getSmoothEdge() { return(isSmoothEdge); }

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    int projIndex;                                                     //index of edge in Projection. must exist.
    int reference;                                                     //index of edge in DrawViewPart Source. may not exist(-1).

    bool isHighlighted;
    bool isCosmetic;
    bool isHiddenEdge;
    bool isSmoothEdge;

private:
    float strokeWidth;
    float strokeScale;
    QPen m_pen;
    QColor m_colCurrent;
    QColor m_colNormal;
    QColor m_colPre;
    QColor m_colSel;
    QColor m_colHid;
    QColor m_defNormal;
    Qt::PenStyle m_styleHid;
};

} // namespace MDIViewPageGui

#endif // DRAWINGGUI_QGRAPHICSITEMEDGE_H
