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

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <QBrush>
#include <QGraphicsItem>
#include <QPen>

#include <Base/Parameter.h>

QT_BEGIN_NAMESPACE
class QPainter;
class QStyleOptionGraphicsItem;
QT_END_NAMESPACE

namespace TechDrawGui
{

class TechDrawGuiExport QGIPrimPath : public QGraphicsPathItem
{
public:
    explicit QGIPrimPath();
    ~QGIPrimPath() {}

    enum {Type = QGraphicsItem::UserType + 170};

    int type() const override { return Type;}
    virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = nullptr ) override;
    virtual QPainterPath shape() const override { return path(); }

    void setHighlighted(bool state);
    virtual void setPrettyNormal();
    virtual void setPrettyPre();
    virtual void setPrettySel();
    virtual void setWidth(double width);
    virtual double getWidth() { return m_pen.widthF();}
    Qt::PenStyle getStyle() { return m_styleCurrent; }
    void setStyle(Qt::PenStyle style);
    void setStyle(int style);
    virtual void setNormalColor(QColor color);
    virtual void setCapStyle(Qt::PenCapStyle c);

    void setFill(QColor color, Qt::BrushStyle style);
    void setFill(QBrush brush);
    void resetFill();
    void setFillColor(QColor c);
    QColor getFillColor(void) { return m_colDefFill; }
    void setFillOverride(bool b) { m_fillOverride = b; }
    QBrush m_brush;

protected:
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

    virtual QColor getNormalColor(void);
    virtual QColor getPreColor(void);
    virtual QColor getSelectColor(void);
    Base::Reference<ParameterGrp> getParmGroup(void);
    virtual Qt::PenCapStyle prefCapStyle(void);

    QPen m_pen;
    QColor m_colNormal;
    bool   m_colOverride;
    Qt::PenStyle m_styleCurrent;
    Qt::PenCapStyle m_capStyle;

    QColor m_colDefFill;                        //"no color" default normal fill color
    QColor m_colNormalFill;                     //current Normal fill color def or plain fill
    Qt::BrushStyle m_styleDef;                  //default Normal fill style
    Qt::BrushStyle m_styleNormal;               //current Normal fill style
    Qt::BrushStyle m_styleSelect;               //Select/preSelect fill style

    bool m_fillOverride;

private:

};

} // namespace MDIViewPageGui

#endif // DRAWINGGUI_QGIPRIMPATH_H
