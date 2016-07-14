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

#ifndef DRAWINGGUI_QGIPRIMPATH_H
#define DRAWINGGUI_QGIPRIMPATH_H

#include <QGraphicsItem>

QT_BEGIN_NAMESPACE
class QPainter;
class QStyleOptionGraphicsItem;
QT_END_NAMESPACE

#include <Base/Parameter.h>

namespace TechDrawGui
{

class TechDrawGuiExport QGIPrimPath : public QGraphicsPathItem
{
public:
    explicit QGIPrimPath();
    ~QGIPrimPath() {}

    enum {Type = QGraphicsItem::UserType + 170};

    int type() const { return Type;}
    virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );

    void setHighlighted(bool state);
    virtual void setPrettyNormal();
    virtual void setPrettyPre();
    virtual void setPrettySel();

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    QColor getNormalColor(void);
    QColor getPreColor(void);
    QColor getSelectColor(void);
    Base::Reference<ParameterGrp> getParmGroup(void);

    bool isHighlighted;
    QPen m_pen;
    QColor m_colCurrent;
    Qt::PenStyle m_styleCurrent;

private:

};

} // namespace MDIViewPageGui

#endif // DRAWINGGUI_QGIPRIMPATH_H
