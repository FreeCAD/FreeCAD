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
# include <GProp_GProps.hxx>
# include <Precision.hxx>
#endif

#include "FeatureMultiTransform.h"
#include "FeatureAddSub.h"
#include "FeatureScaled.h"


using namespace PartDesign;

namespace PartDesign {


PROPERTY_SOURCE(PartDesign::MultiTransform, PartDesign::Transformed)

MultiTransform::MultiTransform()
{
    ADD_PROPERTY(Transformations,(nullptr));
    Transformations.setSize(0);
}

void MultiTransform::positionBySupport()
{
    PartDesign::Transformed::positionBySupport();
    std::vector<App::DocumentObject*> transFeatures = Transformations.getValues();
    for (auto f : transFeatures) {
        if (!(f->isDerivedFrom<PartDesign::Transformed>()))
            throw Base::TypeError("Transformation features must be subclasses of Transformed");
        PartDesign::Transformed* transFeature = static_cast<PartDesign::Transformed*>(f);
        transFeature->Placement.setValue(this->Placement.getValue());

        // To avoid that a linked transform feature stays touched after a recompute
        // we have to purge the touched state
        if (this->isRecomputing()) {
            transFeature->purgeTouched();
        }
    }
}

short MultiTransform::mustExecute() const
{
    if (Transformations.isTouched())
        return 1;
    return Transformed::mustExecute();
}

std::vector<TopoDS_Shape> MultiTransform::applyTransformation(std::vector<TopoDS_Shape> shapes) const
{
    std::vector<App::DocumentObject*> const transFeatures = Transformations.getValues();

    for (auto docObj : transFeatures) {
        if (!docObj->isDerivedFrom<PartDesign::Transformed>()) {
            throw Base::TypeError("Transformation features must be subclasses of Transformed");
        }
        auto transFeature = static_cast<PartDesign::Transformed const*>(docObj);

        shapes = transFeature->applyTransformation(std::move(shapes));
    }

    return shapes;
}

}
