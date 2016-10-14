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

#ifndef DRAWINGGUI_QGRAPHICSITEMARROW_H
#define DRAWINGGUI_QGRAPHICSITEMARROW_H

# include "QGIPrimPath.h"

QT_BEGIN_NAMESPACE
class QPainter;
class QStyleOptionGraphicsItem;
QT_END_NAMESPACE

namespace TechDrawGui
{

class TechDrawGuiExport QGIArrow : public QGIPrimPath
{
public:
    explicit QGIArrow();
    ~QGIArrow() {}

    enum {Type = QGraphicsItem::UserType + 109};
    int type() const { return Type;}

public:
    void draw();
    //void setHighlighted(bool state);
    void flip(bool state);
    double getSize() { return m_size; }
    void setSize(double s);
    int getStyle() { return m_style; }
    void setStyle(int s);
    //QPainterPath shape() const;
    virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );

protected:
    //QVariant itemChange(GraphicsItemChange change, const QVariant &value);

private:
    QBrush m_brush;
    Qt::BrushStyle m_fill;
    double m_size;
    int m_style;
    bool isFlipped;
};

} // namespace MDIViewPageGui

#endif // DRAWINGGUI_QGRAPHICSITEMARROW_H
