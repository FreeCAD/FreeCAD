/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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

#ifndef DRAWINGGUI_QGRAPHICSITEMDIMLINES_H
#define DRAWINGGUI_QGRAPHICSITEMDIMLINES_H

# include "QGIPrimPath.h"

QT_BEGIN_NAMESPACE
class QPainter;
class QStyleOptionGraphicsItem;
QT_END_NAMESPACE

namespace TechDrawGui
{

class TechDrawGuiExport QGIDimLines : public QGIPrimPath
{
public:
    explicit QGIDimLines();
    ~QGIDimLines() {}

    enum {Type = QGraphicsItem::UserType + 175};
    int type() const override { return Type;}
    virtual QRectF boundingRect() const override;
    virtual QPainterPath shape() const override;

public:
    void draw();
    //void setHighlighted(bool state);
    //double getLineWidth() { return m_lineWidth; }
    //void setLineWidth(double w);
    //QPainterPath shape() const;
    virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = nullptr) override;

protected:
    //QVariant itemChange(GraphicsItemChange change, const QVariant &value);
    double getEdgeFuzz(void) const;


private:
};

}

#endif // DRAWINGGUI_QGRAPHICSITEMDIMLINES_H
