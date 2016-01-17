/***************************************************************************
 *   Copyright (c) 2015 WandererFan <wandererfan@gmail.com>                *
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

#ifndef DRAWINGGUI_QGRAPHICSITEMHATCH_H
#define DRAWINGGUI_QGRAPHICSITEMHATCH_H

#include <Qt>
#include <QGraphicsItem>
#include <QStyleOptionGraphicsItem>
#include <QBitmap>

QT_BEGIN_NAMESPACE
class QPainter;
class QStyleOptionGraphicsItem;
QT_END_NAMESPACE

namespace App {
class Color;
}

namespace TechDraw {
class DrawHatch;
}

namespace TechDrawGeometry {
class BaseGeom;
}

namespace TechDrawGui
{

class TechDrawGuiExport QGIHatch :  public QGraphicsPathItem
{

public:
    explicit QGIHatch(std::string parentHatch);
    ~QGIHatch();

    enum {Type = QGraphicsItem::UserType + 122};
    int type() const { return Type;}
    virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );

public:
    std::string getHatchName() const { return m_hatch; }
    void setPrettyNormal();
    void setPrettyPre();
    void setPrettySel();
    void setFill(std::string fillSpec);
    void setColor(App::Color c);

protected:
    // Preselection events:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
    // Selection detection
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);

protected:
    std::string m_hatch;

private:
    QPen m_pen;
    QBrush m_brush;
    QColor m_colNormal;
    QColor m_colPre;
    QColor m_colSel;
    QBitmap m_texture;
    Qt::BrushStyle m_fill;
    std::string m_lastFill;
};

} // namespace MDIViewPageGui

#endif // DRAWINGGUI_QGRAPHICSITEMHATCH_H
