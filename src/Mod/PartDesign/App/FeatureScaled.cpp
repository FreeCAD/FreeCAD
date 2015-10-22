/******************************************************************************
 *   Copyright (c)2012 Jan Rheinlaender <jrheinlaender@users.sourceforge.net> *
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
# include <Precision.hxx>
# include <GProp_GProps.hxx>
# include <BRepGProp.hxx>
#endif

#include "FeatureScaled.h"
#include "FeatureAdditive.h"
#include "FeatureSubtractive.h"
#include <Mod/Part/App/TopoShape.h>

#include <Base/Console.h>
#include <Base/Exception.h>

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

const std::list<gp_Trsf> Scaled::getTransformations(const std::vector<App::DocumentObject*> originals)
{
    double factor = Factor.getValue();
    if (factor < Precision::Confusion())
        throw Base::Exception("Scaling factor too small");
    int occurrences = Occurrences.getValue();
    if (occurrences < 2)
        throw Base::Exception("At least two occurrences required");

    double f = (factor - 1.0) / double(occurrences - 1);

    // Find centre of gravity of first original
    // FIXME: This method will NOT give the expected result for more than one original!
    Part::Feature* originalFeature = static_cast<Part::Feature*>(originals.front());
    TopoDS_Shape original;

    if (originalFeature->getTypeId().isDerivedFrom(PartDesign::Additive::getClassTypeId())) {
        PartDesign::Additive* addFeature = static_cast<PartDesign::Additive*>(originalFeature);
        original = addFeature->AddShape.getShape()._Shape;
    } else if (originalFeature->getTypeId().isDerivedFrom(PartDesign::Subtractive::getClassTypeId())) {
        PartDesign::Subtractive* subFeature = static_cast<PartDesign::Subtractive*>(originalFeature);
        original = subFeature->SubShape.getShape()._Shape;
    }

    GProp_GProps props;
    BRepGProp::VolumeProperties(original,props);
    gp_Pnt cog = props.CentreOfMass();

    // Note: The original feature is NOT included in the list of transformations! Therefore
    // we start with occurrence number 1, not number 0
    std::list<gp_Trsf> transformations;
    gp_Trsf trans;
    transformations.push_back(trans); // identity transformation

    for (int i = 1; i < occurrences; i++) {
        trans.SetScale(cog, 1.0 + double(i) * f);
        transformations.push_back(trans);
    }

    return transformations;
}

}
