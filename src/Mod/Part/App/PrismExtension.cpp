/***************************************************************************
 *   Copyright (c) 2021 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <BRepPrimAPI_MakePrism.hxx>
#endif

#include <Base/Tools.h>

#include "PrismExtension.h"


using namespace Part;

EXTENSION_PROPERTY_SOURCE(Part::PrismExtension, App::DocumentObjectExtension)

PrismExtension::PrismExtension()
{
    EXTENSION_ADD_PROPERTY_TYPE(FirstAngle, (0.0f), "Prism", App::Prop_None, "Angle in first direction");
    EXTENSION_ADD_PROPERTY_TYPE(SecondAngle, (0.0f), "Prism", App::Prop_None, "Angle in second direction");

    static const App::PropertyQuantityConstraint::Constraints angleConstraint = { -89.99999, 89.99999, 1.0 };
    FirstAngle.setConstraints(&angleConstraint);
    SecondAngle.setConstraints(&angleConstraint);

    initExtensionType(PrismExtension::getExtensionClassTypeId());
}

PrismExtension::~PrismExtension() = default;

short int PrismExtension::extensionMustExecute()
{
    if (FirstAngle.isTouched())
        return 1;
    if (SecondAngle.isTouched())
        return 1;
    return DocumentObjectExtension::extensionMustExecute();
}

App::DocumentObjectExecReturn *PrismExtension::extensionExecute()
{
    return App::DocumentObjectExtension::extensionExecute();
}

void PrismExtension::extensionOnChanged(const App::Property* prop)
{
    App::DocumentObjectExtension::extensionOnChanged(prop);
}

TopoDS_Shape PrismExtension::makePrism(double height, const TopoDS_Face& face) const
{
    // the direction vector for the prism is the height for z and the given angle
    BRepPrimAPI_MakePrism mkPrism(face,
        gp_Vec(height * tan(Base::toRadians<double>(FirstAngle.getValue())),
               height * tan(Base::toRadians<double>(SecondAngle.getValue())),
               height));
    return mkPrism.Shape();
}
