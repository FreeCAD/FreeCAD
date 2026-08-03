/***************************************************************************
 *   Copyright (c) 2024 WandererFan <wandererfan@gmail.com>                *
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

#include <QColor>
#include <QFont>
#include <QPainterPath>
#include <QPointF>

#include <Base/Vector3D.h>
#include <Mod/TechDraw/App/DrawBrokenView.h>

#include "QGCustomText.h"
#include "QGIDecoration.h"
#include "QGIUserTypes.h"

using BreakType = TechDraw::DrawBrokenView::BreakType;

namespace TechDrawGui
{

class TechDrawGuiExport QGIBreakLine : public QGIDecoration
{
public:
    explicit QGIBreakLine();
    ~QGIBreakLine() override = default;

    enum {Type = UserType::QGIBreakLine};
    int type() const override { return Type;}

    void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = nullptr ) override;

    void setBounds(double left, double top, double right, double bottom);
    void setBounds(Base::Vector3d topLeft, Base::Vector3d bottomRight);
    void setDirection(Base::Vector3d dir);      // horizontal(1,0,0) vertical(0,1,0);
    void draw() override;

    void setLinePen(QPen isoPen);
    void setBreakColor(QColor c);

    void setBreakType(BreakType style) { m_breakType = style; }
    BreakType breakType() const { return m_breakType; }

protected:

private:
    void drawLargeZigZag();
    QPainterPath makeHorizontalZigZag(Base::Vector3d start) const;
    QPainterPath makeVerticalZigZag(Base::Vector3d start) const;
    void drawSimpleLines();
    QPainterPath pathFromPoints(Base::Vector3d start, Base::Vector3d end);

    void setTools();

    QGraphicsPathItem* m_line0;
    QGraphicsPathItem* m_line1;
    QGraphicsRectItem* m_background;

    Base::Vector3d     m_direction;

    double             m_top;
    double             m_bottom;
    double             m_left;
    double             m_right;

    BreakType m_breakType = BreakType::NONE;
};

}