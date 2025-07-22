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
#include <vector>
#endif

#include "Structured.h"


using namespace Points;


//===========================================================================
// Structured
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
pts=doc.addObject('Points::Structured','View')
pts.Points=p
pts.Width=21
pts.Height=21
*/

// ---------------------------------------------------------

PROPERTY_SOURCE(Points::Structured, Points::Feature)

Structured::Structured()
{
    //    App::PropertyType type = static_cast<App::PropertyType>(App::Prop_None);
    App::PropertyType type = static_cast<App::PropertyType>(App::Prop_ReadOnly);
    ADD_PROPERTY_TYPE(Width, (1), "Structured points", type, "Width of the image");
    ADD_PROPERTY_TYPE(Height, (1), "Structured points", type, "Height of the image");
    // Width.setStatus(App::Property::ReadOnly, true);
    // Height.setStatus(App::Property::ReadOnly, true);
}

App::DocumentObjectExecReturn* Structured::execute()
{
    std::size_t size = Height.getValue() * Width.getValue();
    if (size != Points.getValue().size()) {
        throw Base::ValueError("(Width * Height) doesn't match with number of points");
    }
    this->Points.touch();
    return App::DocumentObject::StdReturn;
}

// ---------------------------------------------------------

namespace App
{
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Points::StructuredCustom, Points::Structured)
/// @endcond

// explicit template instantiation
template class PointsExport FeatureCustomT<Points::Structured>;
}  // namespace App
