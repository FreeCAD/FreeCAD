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

#pragma once

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <QGraphicsItem>
#include <QGraphicsTextItem>
#include <QPointF>
#include <QTextCursor>

QT_BEGIN_NAMESPACE
class QPainter;
class QStyleOptionGraphicsItem;
QT_END_NAMESPACE

#include <Base/Parameter.h>
#include <Base/Vector3D.h>

#include "QGIUserTypes.h"

namespace TechDrawGui
{

class TechDrawGuiExport QGCustomText : public QGraphicsTextItem
{
    Q_OBJECT
public:
    explicit QGCustomText(QGraphicsItem* parent = nullptr);
    ~QGCustomText() override {}

    enum {Type = UserType::QGCustomText};
    int type() const override { return Type;}
    void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = nullptr ) override;

    QRectF tightBoundingRect() const;
    QRectF alignmentRect() const;
    QPointF tightBoundingAdjust() const;


    void setHighlighted(bool state);
    virtual void setPrettyNormal();
    virtual void setPrettyPre();
    virtual void setPrettySel();

    virtual void centerAt(QPointF centerPos);
    virtual void centerAt(double cX, double cY);
    virtual void justifyLeftAt(QPointF centerPos, bool vCenter = true);
    virtual void justifyLeftAt(double cX, double cY, bool vCenter = true);
    virtual void justifyRightAt(QPointF centerPos, bool vCenter = true);
    virtual void justifyRightAt(double cX, double cY, bool vCenter = true);

    virtual double getHeight();
    virtual double getWidth();

    virtual QColor getNormalColor();
    virtual QColor getPreColor();
    virtual QColor getSelectColor();
    virtual void setColor(QColor c);

    virtual void setTightBounding(bool tight);

    void makeMark(double x, double y);
    void makeMark(Base::Vector3d v);

Q_SIGNALS:
    void selectionChanged();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

    Base::Reference<ParameterGrp> getParmGroup();

    bool tightBounding;  // Option to use tighter boundingRect(), works only for plaintext QGCustomText
    QColor m_colNormal;

private:
    void checkCursorChange();
    QTextCursor m_lastCursor;

};

}