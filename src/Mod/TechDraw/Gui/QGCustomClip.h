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

#ifndef DRAWINGGUI_QGCUSTOMCLIP_H
#define DRAWINGGUI_QGCUSTOMCLIP_H

#include <QGraphicsItem>
#include <QPointF>
#include <QRectF>

#include <Base/Vector3D.h>

QT_BEGIN_NAMESPACE
class QPainter;
class QStyleOptionGraphicsItem;
QT_END_NAMESPACE

namespace TechDrawGui
{

class TechDrawGuiExport QGCustomClip : public QGraphicsItemGroup
{
public:
    explicit QGCustomClip(void);
    ~QGCustomClip() {}

    enum {Type = QGraphicsItem::UserType + 132};
    int type() const { return Type;}
    virtual QRectF boundingRect() const;

    void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );
    virtual void centerAt(QPointF centerPos);
    virtual void centerAt(double cX, double cY);
    virtual void setRect(QRectF r);
    virtual void setRect(double x, double y, double w, double h);
    virtual QRectF rect();
    void makeMark(double x, double y);
    void makeMark(Base::Vector3d v);

protected:

private:
    QRectF m_rect;

};

} // namespace MDIViewPageGui

#endif // DRAWINGGUI_QGCUSTOMCLIP_H

