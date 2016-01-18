/***************************************************************************
 *   Copyright (c) 2015 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <vector>

#include <Base/Console.h>
#include <Base/Exception.h>


#include "ViewFeature.h"

using namespace Points;


//===========================================================================
// ViewFeature
//===========================================================================
/*
import Points
import random
import math

r=random.Random()

p=Points.Points()
pts=[]
for i in range(21):
  for j in range(21):
    pts.append(App.Vector(i,j,r.gauss(5,0.05)))

p.addPoints(pts)
doc=App.ActiveDocument
pts=doc.addObject('Points::ViewFeature','View')
pts.Points=p
pts.Width=21
pts.Height=21
*/

PROPERTY_SOURCE(Points::ViewFeature, Points::Feature)

ViewFeature::ViewFeature()
{
    App::PropertyType type = static_cast<App::PropertyType>(App::Prop_None);
    ADD_PROPERTY_TYPE(Width ,(0), "View", type, "The width of the point view");
    ADD_PROPERTY_TYPE(Height,(0), "View", type, "The height of the point view");
    ADD_PROPERTY_TYPE(Direction ,(Base::Vector3d(0,0,1)), "View", type, "The direction of the point view");

    Width.setStatus(App::Property::ReadOnly, true);
    Height.setStatus(App::Property::ReadOnly, true);
}

ViewFeature::~ViewFeature()
{
}

App::DocumentObjectExecReturn *ViewFeature::execute(void)
{
    std::size_t size = Height.getValue() * Width.getValue();
    if (size != Points.getValue().size())
        throw Base::ValueError("(Width * Height) doesn't match with number of points");
    this->Points.touch();
    return App::DocumentObject::StdReturn;
}
