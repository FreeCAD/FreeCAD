/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <sstream>

#include <BRep_Builder.hxx>
#include <Standard_Failure.hxx>
#include <TopoDS_Compound.hxx>
#endif

#include "FeatureProjection.h"
#include "ProjectionAlgos.h"


using namespace Drawing;

PROPERTY_SOURCE(Drawing::FeatureProjection, Part::Feature)

FeatureProjection::FeatureProjection()
{
    static const char* group = "Projection";
    ADD_PROPERTY_TYPE(Source, (nullptr), group, App::Prop_None, "Shape to project");
    ADD_PROPERTY_TYPE(Direction,
                      (Base::Vector3d(0, 0, 1)),
                      group,
                      App::Prop_None,
                      "Projection direction");
    ADD_PROPERTY_TYPE(VCompound, (true), group, App::Prop_None, "Projection parameter");
    ADD_PROPERTY_TYPE(Rg1LineVCompound, (true), group, App::Prop_None, "Projection parameter");
    ADD_PROPERTY_TYPE(RgNLineVCompound, (true), group, App::Prop_None, "Projection parameter");
    ADD_PROPERTY_TYPE(OutLineVCompound, (true), group, App::Prop_None, "Projection parameter");
    ADD_PROPERTY_TYPE(IsoLineVCompound, (true), group, App::Prop_None, "Projection parameter");
    ADD_PROPERTY_TYPE(HCompound, (true), group, App::Prop_None, "Projection parameter");
    ADD_PROPERTY_TYPE(Rg1LineHCompound, (true), group, App::Prop_None, "Projection parameter");
    ADD_PROPERTY_TYPE(RgNLineHCompound, (true), group, App::Prop_None, "Projection parameter");
    ADD_PROPERTY_TYPE(OutLineHCompound, (true), group, App::Prop_None, "Projection parameter");
    ADD_PROPERTY_TYPE(IsoLineHCompound, (true), group, App::Prop_None, "Projection parameter");
}

FeatureProjection::~FeatureProjection()
{}

App::DocumentObjectExecReturn* FeatureProjection::execute(void)
{
    App::DocumentObject* link = Source.getValue();
    if (!link) {
        return new App::DocumentObjectExecReturn("No object linked");
    }
    if (!link->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
        return new App::DocumentObjectExecReturn("Linked object is not a Part object");
    }
    const TopoDS_Shape& shape = static_cast<Part::Feature*>(link)->Shape.getShape().getShape();
    if (shape.IsNull()) {
        return new App::DocumentObjectExecReturn("Linked shape object is empty");
    }

    try {
        const Base::Vector3d& dir = Direction.getValue();
        Drawing::ProjectionAlgos alg(shape, dir);

        TopoDS_Compound comp;
        BRep_Builder builder;
        builder.MakeCompound(comp);
        if (!alg.V.IsNull() && VCompound.getValue()) {
            builder.Add(comp, alg.V);
        }
        if (!alg.V1.IsNull() && Rg1LineVCompound.getValue()) {
            builder.Add(comp, alg.V1);
        }
        if (!alg.VN.IsNull() && RgNLineVCompound.getValue()) {
            builder.Add(comp, alg.VN);
        }
        if (!alg.VO.IsNull() && OutLineVCompound.getValue()) {
            builder.Add(comp, alg.VO);
        }
        if (!alg.VI.IsNull() && IsoLineVCompound.getValue()) {
            builder.Add(comp, alg.VI);
        }
        if (!alg.H.IsNull() && HCompound.getValue()) {
            builder.Add(comp, alg.H);
        }
        if (!alg.H1.IsNull() && Rg1LineHCompound.getValue()) {
            builder.Add(comp, alg.H1);
        }
        if (!alg.HN.IsNull() && RgNLineHCompound.getValue()) {
            builder.Add(comp, alg.HN);
        }
        if (!alg.HO.IsNull() && OutLineHCompound.getValue()) {
            builder.Add(comp, alg.HO);
        }
        if (!alg.HI.IsNull() && IsoLineHCompound.getValue()) {
            builder.Add(comp, alg.HI);
        }

        Shape.setValue(comp);
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
}
