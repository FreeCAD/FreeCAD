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

#include <Base/Parameter.h>
#include <Mod/TechDraw/App/Preferences.h>

#include "Rez.h"

using namespace TechDraw;
using namespace TechDrawGui;

//*** initial static var outside methods!
double Rez::m_rezFactor = Rez::getParameter();
//***

double Rez::getRezFactor()
{
    return Rez::m_rezFactor;
}

void Rez::setRezFactor(double f)
{
    Rez::m_rezFactor = f;
}

//turn App side value to Gui side value
double Rez::guiX(double x)
{
   return getRezFactor() * x;
}

Base::Vector3d Rez::guiX(Base::Vector3d v)
{
    return Base::Vector3d(guiX(v.x), guiX(v.y), guiX(v.z));
}

Base::Vector2d Rez::guiX(Base::Vector3d v, bool planar)
{
    Q_UNUSED(planar);
    return Base::Vector2d(guiX(v.x), guiX(v.y));
}

QPointF Rez::guiX(QPointF p)
{
    return Rez::guiPt(p);
}

//turn Gui side value to App side value
double Rez::appX(double x)
{
   return x / getRezFactor();
}

Base::Vector3d Rez::appX(Base::Vector3d v)
{
    return Base::Vector3d(appX(v.x), appX(v.y), appX(v.z));
}

QPointF Rez::appX(QPointF p)
{
    return appPt(p);
}



//Misc conversions
QPointF Rez::guiPt(QPointF p)
{
    return p * getRezFactor();
}

QPointF Rez::appPt(QPointF p)
{
    return QPointF(appX(p.x()), appX(p.y()));
}

QRectF Rez::guiRect(QRectF r)
{
    return QRectF(guiX(r.left()),
                  guiX(r.top()),
                  guiX(r.width()),
                  guiX(r.height()));
}

QSize Rez::guiSize(QSize s)
{
    return QSize((int)guiX(s.width()), (int)guiX(s.height()));
}

QSize Rez::appSize(QSize s)
{
    return QSize((int)appX(s.width()), (int)appX(s.height()));
}

double Rez::getParameter()
{
    return Preferences::getPreferenceGroup("Rez")->GetFloat("Resolution", 10.0);
}

