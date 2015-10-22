/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <gp_Circ.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
#endif

#include "FeaturePartCircle.h"
#include <Base/Tools.h>

using namespace Part;

App::PropertyQuantityConstraint::Constraints Circle::angleRange = {0.0,360.0,1.0};

PROPERTY_SOURCE(Part::Circle, Part::Primitive)


Circle::Circle()
{
    ADD_PROPERTY(Radius,(2.0f));
    ADD_PROPERTY(Angle0,(0.0f));
    Angle0.setConstraints(&angleRange);
    ADD_PROPERTY(Angle1,(360.0f));
    Angle1.setConstraints(&angleRange);
}

Circle::~Circle()
{
}

short Circle::mustExecute() const
{
    if (Angle0.isTouched() ||
        Angle1.isTouched() ||
        Radius.isTouched())
        return 1;
    return Part::Feature::mustExecute();
}

App::DocumentObjectExecReturn *Circle::execute(void)
{
    gp_Circ circle;
    circle.SetRadius(this->Radius.getValue());
    
    BRepBuilderAPI_MakeEdge clMakeEdge(circle, Base::toRadians<double>(this->Angle0.getValue()),
                                               Base::toRadians<double>(this->Angle1.getValue()));
    const TopoDS_Edge& edge = clMakeEdge.Edge();
    this->Shape.setValue(edge);

    return App::DocumentObject::StdReturn;
}

void Circle::onChanged(const App::Property* prop)
{
    if (!isRestoring()) {
        if (prop == &Radius || prop == &Angle0 || prop == &Angle1){
            try {
                App::DocumentObjectExecReturn *ret = recompute();
                delete ret;
            }
            catch (...) {
            }
        }
    }
    Part::Feature::onChanged(prop);
}
