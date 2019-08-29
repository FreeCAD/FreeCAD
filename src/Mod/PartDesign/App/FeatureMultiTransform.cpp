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

#include "FeatureMultiTransform.h"
#include "FeatureScaled.h"
#include "FeatureAddSub.h"
#include <Mod/Part/App/TopoShape.h>

#include <Base/Console.h>
#include <Base/Exception.h>

using namespace PartDesign;

namespace PartDesign {


PROPERTY_SOURCE(PartDesign::MultiTransform, PartDesign::Transformed)

MultiTransform::MultiTransform()
{
    ADD_PROPERTY(Transformations,(0));
    Transformations.setSize(0);
}

void MultiTransform::positionBySupport(void)
{
    PartDesign::Transformed::positionBySupport();
    std::vector<App::DocumentObject*> transFeatures = Transformations.getValues();
    for (std::vector<App::DocumentObject*>::const_iterator f = transFeatures.begin();
         f != transFeatures.end(); ++f) {
        if (!((*f)->getTypeId().isDerivedFrom(PartDesign::Transformed::getClassTypeId())))
            throw Base::TypeError("Transformation features must be subclasses of Transformed");
        PartDesign::Transformed* transFeature = static_cast<PartDesign::Transformed*>(*f);
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

const std::list<gp_Trsf> MultiTransform::getTransformations(const std::vector<App::DocumentObject*> originals)
{
    std::vector<App::DocumentObject*> transFeatures = Transformations.getValues();

    // Find centre of gravity of first original
    // FIXME: This method will NOT give the expected result for more than one original!
    Part::Feature* originalFeature = static_cast<Part::Feature*>(originals.front());
    TopoDS_Shape original;

    if (originalFeature->getTypeId().isDerivedFrom(PartDesign::FeatureAddSub::getClassTypeId())) {
        PartDesign::FeatureAddSub* addFeature = static_cast<PartDesign::FeatureAddSub*>(originalFeature);
        //if (addFeature->getAddSubType() == FeatureAddSub::Additive)
        //    original = addFeature->AddSubShape.getShape().getShape();
        //else
            original = addFeature->AddSubShape.getShape().getShape();
    }

    GProp_GProps props;
    BRepGProp::VolumeProperties(original,props);
    gp_Pnt cog = props.CentreOfMass();

    std::list<gp_Trsf> result;
    std::list<gp_Pnt> cogs;
    std::vector<App::DocumentObject*>::const_iterator f;

    for (f = transFeatures.begin(); f != transFeatures.end(); ++f) {
        if (!((*f)->getTypeId().isDerivedFrom(PartDesign::Transformed::getClassTypeId())))
            throw Base::TypeError("Transformation features must be subclasses of Transformed");
        PartDesign::Transformed* transFeature = static_cast<PartDesign::Transformed*>(*f);
        std::list<gp_Trsf> newTransformations = transFeature->getTransformations(originals);

        if (result.empty()) {
            // First transformation Feature
            result = newTransformations;
            for (std::list<gp_Trsf>::const_iterator nt = newTransformations.begin(); nt != newTransformations.end(); ++nt) {
                cogs.push_back(cog.Transformed(*nt));
            }
        } else {
            // Retain a copy of the first set of transformations for iterator ot
            // We can't iterate through result if we are also adding elements with push_back()!
            std::list<gp_Trsf> oldTransformations;
            result.swap(oldTransformations); // empty result to receive new transformations
            std::list<gp_Pnt> oldCogs;
            cogs.swap(oldCogs); // empty cogs to receive new cogs

            if ((*f)->getTypeId() == PartDesign::Scaled::getClassTypeId()) {
                // Diagonal method
                // Multiply every element in the old transformations' slices with the corresponding
                // element in the newTransformations. Example:
                // a11 a12 a13 a14          b1    a11*b1 a12*b1 a13*b1 a14*b1
                // a21 a22 a23 a24   diag   b2  = a21*b2 a22*b2 a23*b2 a24*b1
                // a31 a23 a33 a34          b3    a31*b3 a23*b3 a33*b3 a34*b1
                // In other words, the length of the result vector is equal to the length of the
                // oldTransformations vector

                if (newTransformations.empty())
                    throw Base::ValueError("Number of occurrences must be a divisor of previous number of occurrences");
                if (oldTransformations.size() % newTransformations.size() != 0)
                    throw Base::ValueError("Number of occurrences must be a divisor of previous number of occurrences");

                unsigned sliceLength = oldTransformations.size() / newTransformations.size();
                std::list<gp_Trsf>::const_iterator ot = oldTransformations.begin();
                std::list<gp_Pnt>::const_iterator oc = oldCogs.begin();

                for (std::list<gp_Trsf>::const_iterator nt = newTransformations.begin(); nt != newTransformations.end(); ++nt) {
                    for (unsigned s = 0; s < sliceLength; s++) {
                        gp_Trsf trans;
                        double factor = nt->ScaleFactor(); // extract scale factor

                        if (factor > Precision::Confusion()) {
                            trans.SetScale(*oc, factor); // recreate the scaled transformation to use the correct COG
                            trans = trans * (*ot);
                            cogs.push_back(*oc); // Scaling does not affect the COG
                        } else {
                            trans = (*nt) * (*ot);
                            cogs.push_back(oc->Transformed(*nt));
                        }
                        result.push_back(trans);
                        ++ot;
                        ++oc;
                    }
                }
            } else {
                // Multiplication method: Combine the new transformations with the old ones.
                // All old transformations are multiplied with all new ones, so that the length of the
                // result vector is the length of the old and new transformations multiplied.
                // a11 a12         b1    a11*b1 a12*b1 a11*b2 a12*b2 a11*b3 a12*b3
                // a21 a22   mul   b2  = a21*b1 a22*b1 a21*b2 a22*b2 a21*b3 a22*b3
                //                 b3
                for (std::list<gp_Trsf>::const_iterator nt = newTransformations.begin(); nt != newTransformations.end(); ++nt) {
                    std::list<gp_Pnt>::const_iterator oc = oldCogs.begin();

                    for (std::list<gp_Trsf>::const_iterator ot = oldTransformations.begin(); ot != oldTransformations.end(); ++ot) {
                        result.push_back((*nt) * (*ot));
                        cogs.push_back(oc->Transformed(*nt));
                        ++oc;
                    }
                }
            }
            // What about the Additive method: Take the last (set of) transformations and use them as
            // "originals" for the next transformationFeature, so that something similar to a sweep
            // for transformations could be put together?
        }
    }

    return result;
}

}
