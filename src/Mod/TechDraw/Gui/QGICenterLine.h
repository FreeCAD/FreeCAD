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

#pragma once

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <QColor>
#include <QPointF>

#include "QGIDecoration.h"
#include "QGIUserTypes.h"

namespace TechDrawGui
{

class TechDrawGuiExport QGICenterLine : public QGIDecoration
{
public:
    explicit QGICenterLine();
    ~QGICenterLine() override = default;

    enum {Type = UserType::QGICenterLine};
    int type() const override { return Type;}

    void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = nullptr ) override;

    void setBounds(double x1, double y1, double x2, double y2);
    void draw() override;

    void setIntersection(bool isIntersecting);

    void setLinePen(QPen isoPen);

protected:
    QColor getCenterColor();
    Qt::PenStyle getCenterStyle();
    void makeLine();
    void setTools();

private:
    QGraphicsPathItem* m_line;           //primpath?
    QPointF            m_start;
    QPointF            m_end;
    bool               m_isintersection;
};

}