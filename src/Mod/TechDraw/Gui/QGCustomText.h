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

#ifndef DRAWINGGUI_QGCUSTOMTEXT_H
#define DRAWINGGUI_QGCUSTOMTEXT_H

#include <QGraphicsItem>
#include <QGraphicsTextItem>
#include <QPointF>

QT_BEGIN_NAMESPACE
class QPainter;
class QStyleOptionGraphicsItem;
QT_END_NAMESPACE

#include <Base/Parameter.h>
#include <Base/Vector3D.h>

namespace TechDrawGui
{

class TechDrawGuiExport QGCustomText : public QGraphicsTextItem
{
public:
    explicit QGCustomText(void);
    ~QGCustomText() {}

    enum {Type = QGraphicsItem::UserType + 130};
    int type() const { return Type;}

    void setHighlighted(bool state);
    void setPrettyNormal();
    void setPrettyPre();
    void setPrettySel();

    virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );
    virtual void centerAt(QPointF centerPos);
    virtual void centerAt(double cX, double cY);
    virtual QColor getNormalColor(void);
    virtual QColor getPreColor(void);
    virtual QColor getSelectColor(void);
    virtual void setColor(QColor c) { m_colNormal = c; }
    void makeMark(double x, double y);
    void makeMark(Base::Vector3d v);

protected:
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    Base::Reference<ParameterGrp> getParmGroup(void);

    bool isHighlighted;
    QColor m_colCurrent;
    QColor m_colNormal;

private:

};

}

#endif // DRAWINGGUI_QGCUSTOMTEXT_H
