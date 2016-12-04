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
#include "PreCompiled.h"

#ifndef _PreComp_

#endif

#include "Rez.h"

using namespace TechDrawGui;

double Rez::getRezFactor()
{
    return 10.0;    // 1/10 mm
}

//turn App side value to Gui side value
double Rez::guiX(double x)
{
   return getRezFactor() * x;
}

Base::Vector2d Rez::guiX(Base::Vector2d v)
{
    Base::Vector2d result(guiX(v.y),guiX(v.y));
    return result;
}

Base::Vector3d Rez::guiX(Base::Vector3d v)
{
    Base::Vector3d result(guiX(v.x),guiX(v.y),guiX(v.z));
    return result;
}

//turn Gui side value to App side value
double Rez::appX(double x)
{
   return x / getRezFactor();
}

QPointF Rez::guiPt(QPointF p)
{
    QPointF result = p;
    result *= getRezFactor();
    return result;
}

QRectF Rez::guiRect(QRectF r)
{
    QRectF result(guiX(r.left()),
                  guiX(r.top()),
                  guiX(r.width()),
                  guiX(r.height()));
    return result;
}

QSize Rez::guiSize(QSize s)
{
    QSize result((int)guiX(s.width()),(int)guiX(s.height()));
    return result;
}

QSize Rez::appSize(QSize s)
{
    QSize result((int)appX(s.width()),(int)appX(s.height()));
    return result;
}

