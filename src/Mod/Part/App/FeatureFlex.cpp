// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 F. Foinant-Willig <flachyjoe@gmail.com>            *
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

#include <Base/Exception.h>

#include "FeatureFlex.h"
#include "Deformation.h"

using namespace Part;

PROPERTY_SOURCE(Part::Flex, Part::Feature)

App::PropertyIntegerConstraint::Constraints Flex::sampleRange = {10, 100, 1};

Flex::Flex()
{
    ADD_PROPERTY_TYPE(Base, (nullptr), "Flex", App::Prop_None, "Shape to deform");
    ADD_PROPERTY_TYPE(Pitch, (10.0), "Flex", App::Prop_None, "Pitch for the twist deformation");
    ADD_PROPERTY_TYPE(Samples, (10), "Flex", App::Prop_None, "Samples count for geometry approximation");
    Samples.setConstraints(&sampleRange);
}


short Flex::mustExecute() const
{
    if (Base.isTouched() || Pitch.isTouched() || Samples.isTouched()) {
        return 1;
    }
    return 0;
}

Flex::FlexParameters Flex::computeFinalParameters() const
{
    Flex::FlexParameters result;
    result.pitch = Pitch.getValue();
    result.samples = Samples.getValue();

    return result;
}

TopoShape Flex::FlexShape(const TopoShape& source, const Flex::FlexParameters& params)
{
    switch (params.mode) {
        case FlexMode::Bend:
            return twist(source, params);
        case FlexMode::Twist:
            return twist(source, params);
        case FlexMode::Identity:
            return identity(source, params);
    }
}


TopoShape Flex::twist(const TopoShape& source, const Flex::FlexParameters& params)
{
    double pitch = params.pitch;
    const TopoDS_Shape& shape = source.getShape();

    auto func = [pitch](gp_Pnt pt) {
        return Deformation::twistAlongX(pt, pitch);
    };

    try {
        return {Deformation::deform(shape, func, params.samples)};
    }
    catch (...) {
        Base::Console().warning("FeatureFlex failed on twist\n");
        return {};
    }
}

TopoShape Flex::identity(const TopoShape& source, const Flex::FlexParameters& params)
{
    const TopoDS_Shape& shape = source.getShape();

    auto identity = [](gp_Pnt pt) {
        return pt;
    };

    try {
        return {Deformation::deform(shape, identity, params.samples)};
    }
    catch (...) {
        Base::Console().warning("FeatureFlex failed on identity\n");
        return {};
    }
}

App::DocumentObjectExecReturn* Flex::execute()
{
    //    Base::Console().message("FS::execute()\n");
    App::DocumentObject* link = Base.getValue();
    if (!link) {
        return new App::DocumentObjectExecReturn("No object linked");
    }

    try {
        Flex::FlexParameters params = computeFinalParameters();
        TopoShape result = FlexShape(
            Feature::getTopoShape(link, ShapeOption::ResolveLink | ShapeOption::Transform),
            params
        );
        if (result.isNull()) {
            return new App::DocumentObjectExecReturn("Null shape");
        }
        this->Shape.setValue(result);
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
}
