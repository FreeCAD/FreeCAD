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
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <App/Application.h>
#include <Mod/Part/App/modelRefine.h>

using namespace PartDesign;

namespace PartDesign {

PROPERTY_SOURCE(PartDesign::Transformed, PartDesign::Feature)

Transformed::Transformed() : rejected(0)
{
    ADD_PROPERTY(Originals,(0));
    Originals.setSize(0);
    Placement.StatusBits.set(2, true);
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
    std::vector<gp_Trsf> transformations;
    try {
        std::list<gp_Trsf> t_list = getTransformations(originals);
        transformations.insert(transformations.end(), t_list.begin(), t_list.end());
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

    std::set<std::vector<gp_Trsf>::const_iterator> nointersect_trsfms;
    std::set<std::vector<gp_Trsf>::const_iterator> overlapping_trsfms;

    // NOTE: It would be possible to build a compound from all original addShapes/subShapes and then
    // transform the compounds as a whole. But we choose to apply the transformations to each
    // Original separately. This way it is easier to discover what feature causes a fuse/cut
    // to fail. The downside is that performance suffers when there are many originals. But it seems
    // safe to assume that in most cases there are few originals and many transformations
    for (std::vector<App::DocumentObject*>::const_iterator o = originals.begin(); o != originals.end(); o++)
    {
        // Extract the original shape and determine whether to cut or to fuse
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

        // Transform the add/subshape and collect the resulting shapes for overlap testing
        std::vector<std::vector<gp_Trsf>::const_iterator> v_transformations;
        std::vector<TopoDS_Shape> v_transformedShapes;

        std::vector<gp_Trsf>::const_iterator t = transformations.begin();
        t++; // Skip first transformation, which is always the identity transformation
        for (; t != transformations.end(); t++) {
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
            if (!Part::checkIntersection(support, mkTrf.Shape(), false, true)) {
                Base::Console().Warning("Transformed shape does not intersect support %s: Removed\n", (*o)->getNameInDocument());
                nointersect_trsfms.insert(t);
            } else {
                v_transformations.push_back(t);
                v_transformedShapes.push_back(mkTrf.Shape());
                // Note: Transformations that do not intersect the support are ignored in the overlap tests
            }
        }

        if (v_transformedShapes.empty())
            break; // Skip the overlap check and go on to next original

        // Check for overlapping of the original and the transformed shapes, and remove the overlapping transformations
        if (this->getTypeId() != PartDesign::MultiTransform::getClassTypeId()) {
            // If there is only one transformed feature, we allow an overlap (though it might seem
            // illogical to the user why we allow overlapping shapes in this case!)
            if (v_transformedShapes.size() > 1)
                if (Part::checkIntersection(shape, v_transformedShapes.front(), false, false)) {
                    // For single transformations, if one overlaps, all overlap, as long as we have uniform increments
                    overlapping_trsfms.insert(v_transformations.begin(),v_transformations.end());
                    v_transformedShapes.clear();
                }
        } else {
            // For MultiTransform, just checking the first transformed shape is not sufficient - any two
            // features might overlap, even if the original and the first shape don't overlap!

            std::set<std::vector<TopoDS_Shape>::iterator> rejected_iterators;

            std::vector<TopoDS_Shape>::iterator s1 = v_transformedShapes.begin();
            std::vector<TopoDS_Shape>::iterator s2 = s1;
            s2++;
            std::vector<std::vector<gp_Trsf>::const_iterator>::const_iterator t1 = v_transformations.begin();
            std::vector<std::vector<gp_Trsf>::const_iterator>::const_iterator t2 = t1;
            t2++;
            for (; s2 != v_transformedShapes.end();) {
                // Check intersection with the original
                if (Part::checkIntersection(shape, *s1, false, false)) {
                    rejected_iterators.insert(s1);
                    overlapping_trsfms.insert(*t1);
                }
                // Check intersection with other transformations
                for (; s2 != v_transformedShapes.end(); s2++, t2++)
                    if (Part::checkIntersection(*s1, *s2, false, false)) {
                        rejected_iterators.insert(s1);
                        rejected_iterators.insert(s2);
                        overlapping_trsfms.insert(*t1);
                        overlapping_trsfms.insert(*t2);
                    }
                s1++;
                s2 = s1;
                s2++;
                t1++;
                t2 = t1;
                t2++;
            }
            // Check intersection of last transformation with the original
            if (Part::checkIntersection(shape, *s1, false, false)) {
                rejected_iterators.insert(s1);
                overlapping_trsfms.insert(*t1);
            }

            for (std::set<std::vector<TopoDS_Shape>::iterator>::reverse_iterator it = rejected_iterators.rbegin();
                 it != rejected_iterators.rend(); it++)
                v_transformedShapes.erase(*it);
        }

        if (v_transformedShapes.empty())
            break; // Skip the boolean operation and go on to next original

        // Build a compound from all the valid transformations
        BRep_Builder builder;
        TopoDS_Compound transformedShapes;
        builder.MakeCompound(transformedShapes);
        for (std::vector<TopoDS_Shape>::const_iterator s = v_transformedShapes.begin(); s != v_transformedShapes.end(); s++)
            builder.Add(transformedShapes, *s);

        // Fuse/Cut the compounded transformed shapes with the support
        TopoDS_Shape result;

        if (fuse) {
            BRepAlgoAPI_Fuse mkFuse(support, transformedShapes);
            if (!mkFuse.IsDone())
                return new App::DocumentObjectExecReturn("Fusion with support failed", *o);
            // we have to get the solids (fuse sometimes creates compounds)
            result = this->getSolid(mkFuse.Shape());
            // lets check if the result is a solid
            if (result.IsNull())
                return new App::DocumentObjectExecReturn("Resulting shape is not a solid", *o);
            result = refineShapeIfActive(result);
        } else {
            BRepAlgoAPI_Cut mkCut(support, transformedShapes);
            if (!mkCut.IsDone())
                return new App::DocumentObjectExecReturn("Cut out of support failed", *o);
            result = mkCut.Shape();
            result = refineShapeIfActive(result);
        }

        support = result; // Use result of this operation for fuse/cut of next original
    }

    if (!overlapping_trsfms.empty())
        // Concentrate on overlapping shapes since they are more serious
        for (std::set<std::vector<gp_Trsf>::const_iterator>::const_iterator it = overlapping_trsfms.begin();
             it != overlapping_trsfms.end(); it++)
            rejected.push_back(**it);
    else
        for (std::set<std::vector<gp_Trsf>::const_iterator>::const_iterator it = nointersect_trsfms.begin();
             it != nointersect_trsfms.end(); it++)
            rejected.push_back(**it);

    this->Shape.setValue(support);
    if (!overlapping_trsfms.empty())
        return new App::DocumentObjectExecReturn("Transformed objects are overlapping, try using a higher length or reducing the number of occurrences");
        // Note: This limitation could be overcome by fusing the transformed features instead of
        // compounding them, probably at the expense of quite a bit of performance and complexity
        // in this code
    else
        return App::DocumentObject::StdReturn;
}

TopoDS_Shape Transformed::refineShapeIfActive(const TopoDS_Shape& oldShape) const
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/PartDesign");
    if (hGrp->GetBool("RefineModel", false)) {
        Part::BRepBuilderAPI_RefineModel mkRefine(oldShape);
        TopoDS_Shape resShape = mkRefine.Shape();
        return resShape;
    }

    return oldShape;
}

}
