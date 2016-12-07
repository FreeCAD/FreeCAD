/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer <jrheinlaender[at]users.sourceforge.net>     *
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

#include "DatumFeature.h"


using namespace Part;
using namespace Attacher;

PROPERTY_SOURCE_ABSTRACT_WITH_EXTENSIONS(Part::Datum, Part::Feature)

Datum::Datum(void)
{
    AttachExtension::initExtension(this);
    touch();
}

Datum::~Datum()
{
}

void Datum::onDocumentRestored()
{
    // This seems to be the only way to make the ViewProvider display the datum feature
    Support.touch();
    Part::Feature::onDocumentRestored();
}

TopoDS_Shape Datum::getShape() const
{
    Part::TopoShape sh = Shape.getShape();
    sh.setPlacement(Placement.getValue());
    return sh.getShape();
}

Base::Vector3d Datum::getBasePoint () const {
    return Placement.getValue().getPosition();
}
