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

#pragma once

#include <Mod/TechDraw/TechDrawGlobal.h>

#include "QGIPrimPath.h"
#include "QGIUserTypes.h"

namespace Base {
class Vector2d;
}

namespace TechDrawGui
{

class TechDrawGuiExport QGIVertex : public QGIPrimPath
{
public:
    explicit QGIVertex(int index);
    ~QGIVertex() override = default;

    enum {Type = UserType::QGIVertex};
    int type() const override { return Type;}
    void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = nullptr ) override;

    int getProjIndex() const { return projIndex; }

    double getRadius() const { return m_radius; }
    virtual void setRadius(double r);

    Base::Vector2d toVector2d() const;
    Base::Vector2d vector2dBetweenPoints(const QGIVertex* p2) const;

protected:
    bool multiselectEligible() override { return true; }

    int projIndex;
    double m_radius;
};

}