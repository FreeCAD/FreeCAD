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
# include <BRepBuilderAPI_Transform.hxx>
# include <BRepAlgoAPI_Fuse.hxx>
# include <BRepAlgoAPI_Cut.hxx>
# include <BRep_Builder.hxx>
# include <TopExp.hxx>
# include <TopExp_Explorer.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <Precision.hxx>
# include <BRepBuilderAPI_Copy.hxx>
#endif


#include "FeatureTransformed.h"
#include "FeatureMultiTransform.h"
#include "FeatureAdditive.h"
#include "FeatureSubtractive.h"
#include "FeatureMirrored.h"

#include <Base/Console.h>

using namespace PartDesign;

namespace PartDesign {

PROPERTY_SOURCE(PartDesign::Transformed, PartDesign::Feature)

Transformed::Transformed() : rejected(0)
{
    ADD_PROPERTY(Originals,(0));
    Originals.setSize(0);
}

void Transformed::onChanged(const App::Property* prop)
{
    if (prop == &Originals) {
        // if attached then mark it as read-only
        this->Placement.StatusBits.set(2, Originals.getSize() != 0);
    }

    PartDesign::Feature::onChanged(prop);
}

void Transformed::positionBySupport(void)
{
    Part::Feature *support = static_cast<Part::Feature*>(getSupportObject());
    if ((support != NULL) && support->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        this->Placement.setValue(support->Placement.getValue());
}

App::DocumentObject* Transformed::getSupportObject() const
{
    if (!Originals.getValues().empty())
        return Originals.getValues().front();
    else
        return NULL;
}

App::DocumentObject* Transformed::getSketchObject() const
{
    std::vector<DocumentObject*> originals = Originals.getValues();
    if (!originals.empty() && originals.front()->getTypeId().isDerivedFrom(PartDesign::SketchBased::getClassTypeId()))
        return (static_cast<PartDesign::SketchBased*>(originals.front()))->getVerifiedSketch();
    else
        return NULL;
}

short Transformed::mustExecute() const
{
    if (Originals.isTouched())
        return 1;
    return PartDesign::Feature::mustExecute();
}

App::DocumentObjectExecReturn *Transformed::execute(void)
{
    rejected.clear();

    std::vector<App::DocumentObject*> originals = Originals.getValues();
    if (originals.empty()) // typically InsideMultiTransform
        return App::DocumentObject::StdReturn;

    this->positionBySupport();

    // get transformations from subclass by calling virtual method
    std::list<gp_Trsf> transformations;
    try {
        transformations = getTransformations(originals);
    } catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    if (transformations.empty())
        return App::DocumentObject::StdReturn; // No transformations defined, exit silently

    // Get the support
    // NOTE: Because of the way we define the support, FeatureTransformed can only work on
    // one Body feature at a time
    // TODO: Currently, the support is simply the first Original. Change this to the Body feature later
    Part::Feature* supportFeature = static_cast<Part::Feature*>(getSupportObject());
    const Part::TopoShape& supportTopShape = supportFeature->Shape.getShape();
    if (supportTopShape._Shape.IsNull())
        return new App::DocumentObjectExecReturn("Cannot transform invalid support shape");

    // create an untransformed copy of the support shape
    Part::TopoShape supportShape(supportTopShape);
    supportShape.setTransform(Base::Matrix4D());
    TopoDS_Shape support = supportShape._Shape;

    // NOTE: It would be possible to build a compound from all original addShapes/subShapes and then
    // transform the compounds as a whole. But we choose to apply the transformations to each
    // Original separately. This way it is easier to discover what feature causes a fuse/cut
    // to fail. The downside is that performance suffers when there are many originals. But it seems
    // safe to assume that in most cases there are few originals and many transformations
    for (std::vector<App::DocumentObject*>::const_iterator o = originals.begin(); o != originals.end(); o++)
    {
        TopoDS_Shape shape;
        bool fuse;

        if ((*o)->getTypeId().isDerivedFrom(PartDesign::Additive::getClassTypeId())) {
            PartDesign::Additive* addFeature = static_cast<PartDesign::Additive*>(*o);
            shape = addFeature->AddShape.getShape()._Shape;
            if (shape.IsNull())
                return new App::DocumentObjectExecReturn("Shape of additive feature is empty");
            fuse = true;
        } else if ((*o)->getTypeId().isDerivedFrom(PartDesign::Subtractive::getClassTypeId())) {
            PartDesign::Subtractive* subFeature = static_cast<PartDesign::Subtractive*>(*o);
            shape = subFeature->SubShape.getShape()._Shape;
            if (shape.IsNull())
                return new App::DocumentObjectExecReturn("Shape of subtractive feature is empty");
            fuse = false;
        } else {
            return new App::DocumentObjectExecReturn("Only additive and subtractive features can be transformed");
        }

        // Transform the add/subshape and build a compound from the transformations, 
        BRep_Builder builder;
        TopoDS_Compound transformedShapes;
        builder.MakeCompound(transformedShapes);
        std::vector<TopoDS_Shape> v_transformedShapes; // collect all the transformed shapes for intersection testing
        std::list<gp_Trsf>::const_iterator t = transformations.begin();
        t++; // Skip first transformation, which is always the identity transformation

        for (; t != transformations.end(); t++)
        {
            // Make an explicit copy of the shape because the "true" parameter to BRepBuilderAPI_Transform
            // seems to be pretty broken
            BRepBuilderAPI_Copy copy(shape);
            shape = copy.Shape();
            if (shape.IsNull())
                throw Base::Exception("Transformed: Linked shape object is empty");

            BRepBuilderAPI_Transform mkTrf(shape, *t, false); // No need to copy, now
            if (!mkTrf.IsDone())
                return new App::DocumentObjectExecReturn("Transformation failed", (*o));

            // Check for intersection with support
            if (!Part::checkIntersection(support, mkTrf.Shape(), false)) {
                Base::Console().Warning("Transformed shape does not intersect support %s: Removed\n", (*o)->getNameInDocument());
                // Note: The removal happens in getSolid() after the fuse
                rejected.push_back(*t);
            }
            builder.Add(transformedShapes, mkTrf.Shape());
            v_transformedShapes.push_back(mkTrf.Shape());

            /*
            // Note: This method is only stable for Linear and Polar transformations. No need to
            // make an explicit copy of the shape, either
            TopoDS_Shape trfShape = shape.Moved(TopLoc_Location(*t));

            // Check for intersection with support
            Bnd_Box transformed_bb;
            BRepBndLib::Add(trfShape, transformed_bb);
            if (support_bb.Distance(transformed_bb) > Precision::Confusion()) {
                Base::Console().Warning("Transformed shape does not intersect support %s: Removed\n", (*o)->getNameInDocument());
                // Note: The removal happens in getSolid() after the fuse
            }
            builder.Add(transformedShapes, trfShape);
            v_transformedShapes.push_back(trfShape);
            */
        }

        // Check for intersection of the original and the transformed shape
        for (std::vector<TopoDS_Shape>::const_iterator s = v_transformedShapes.begin(); s != v_transformedShapes.end(); s++)
        {
            // If there is only one transformed feature, this check is not necessary (though it might seem
            // illogical to the user why we allow overlapping shapes in this case!
            if (v_transformedShapes.size() == 1)
                break;

            if (Part::checkIntersection(shape, *s, false))
                return new App::DocumentObjectExecReturn("Transformed objects are overlapping, try using a higher length or reducing the number of occurrences", (*o));
                // Note: This limitation could be overcome by fusing the transformed features instead of
                // compounding them, probably at the expense of quite a bit of performance and complexity
                // in this code

            // For MultiTransform, just checking the first transformed shape is not sufficient - any two
            // features might overlap, even if the original and the first shape don't overlap!
            if (this->getTypeId() != PartDesign::MultiTransform::getClassTypeId())
                break;
            else {
                // Check intersection with all other transformed shapes as well
                std::vector<TopoDS_Shape>::const_iterator s2 = s;
                s2++;
                for (; s2 != v_transformedShapes.end(); s2++)
                    if (Part::checkIntersection(*s, *s2, false))
                        return new App::DocumentObjectExecReturn("Transformed objects are overlapping, try using a higher length or reducing the number of occurrences", (*o));
            }
        }

        // Fuse/Cut the compounded transformed shapes with the support
        TopoDS_Shape result;

        if (fuse) {
            BRepAlgoAPI_Fuse mkFuse(support, transformedShapes);
            if (!mkFuse.IsDone())
                return new App::DocumentObjectExecReturn("Fusion with support failed", *o);
            // we have to get the solids (fuse sometimes creates compounds)
            // Note: Because getSolid() only returns the first solid in the explorer, all
            // solids that are outside the support automatically disappear!
            result = this->getSolid(mkFuse.Shape());
            // lets check if the result is a solid
            if (result.IsNull())
                return new App::DocumentObjectExecReturn("Resulting shape is not a solid", *o);
            // check if mkFuse created more than one solids
            TopExp_Explorer xp;
            xp.Init(mkFuse.Shape(),TopAbs_SOLID);
            if (xp.More())
                xp.Next();
            if (!xp.More())        // There are no rejected transformations even
                rejected.clear();  // if the bb check guessed that there would be
        } else {
            BRepAlgoAPI_Cut mkCut(support, transformedShapes);
            if (!mkCut.IsDone())
                return new App::DocumentObjectExecReturn("Cut out of support failed", *o);
            result = mkCut.Shape();
        }

        support = result; // Use result of this operation for fuse/cut of next original
    }

    this->Shape.setValue(support);

    return App::DocumentObject::StdReturn;
}

}
