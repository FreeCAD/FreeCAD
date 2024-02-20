/******************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_
# include <BRepGProp.hxx>
# include <BRepBuilderAPI_Transform.hxx>
# include <GProp_GProps.hxx>
# include <Precision.hxx>
#endif

#include "FeatureScaled.h"
#include "FeatureAddSub.h"


using namespace PartDesign;

namespace PartDesign {


PROPERTY_SOURCE(PartDesign::Scaled, PartDesign::Transformed)

Scaled::Scaled()
{
    ADD_PROPERTY(Factor,(2.0));
    ADD_PROPERTY(Occurrences,(2));
}

short Scaled::mustExecute() const
{
    if (Factor.isTouched() ||
        Occurrences.isTouched())
        return 1;
    return Transformed::mustExecute();
}

std::vector<TopoDS_Shape> Scaled::applyTransformation(std::vector<TopoDS_Shape> shapes) const
{
    double factor = Factor.getValue();
    if (factor < Precision::Confusion())
        throw Base::ValueError("Scaling factor too small");
    int occurrences = Occurrences.getValue();
    if (occurrences < 2)
        throw Base::ValueError("At least two occurrences required");

    double f = (factor - 1.0) / double(occurrences - 1);

    std::vector<TopoDS_Shape> result;

    int i = 0;
    for (auto const& shape : shapes) {
        GProp_GProps props;
        BRepGProp::VolumeProperties(shape, props);
        gp_Pnt cog = props.CentreOfMass();

        gp_Trsf trans;
        trans.SetScale(cog, 1.0 + double(i) * f);

        BRepBuilderAPI_Transform mkScale(shape, trans);
        if (!mkScale.IsDone()) {
            throw Base::CADKernelError(QT_TRANSLATE_NOOP("Exception", "Transformation failed"));
        }
        result.push_back(mkScale);

        if (++i == occurrences) i = 0;
    }

    return result;
}

}
