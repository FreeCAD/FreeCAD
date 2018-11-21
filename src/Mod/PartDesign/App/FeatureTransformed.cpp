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
# include <BRepBndLib.hxx>
# include <Bnd_Box.hxx>
#endif


#include "FeatureTransformed.h"
#include "FeatureMultiTransform.h"
#include "FeatureAddSub.h"
#include "FeatureMirrored.h"
#include "FeatureLinearPattern.h"
#include "FeaturePolarPattern.h"
#include "FeatureSketchBased.h"
#include "Body.h"

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Base/Reader.h>
#include <App/Application.h>
#include <Mod/Part/App/modelRefine.h>

using namespace PartDesign;

namespace PartDesign {

PROPERTY_SOURCE(PartDesign::Transformed, PartDesign::Feature)

Transformed::Transformed()
{
    ADD_PROPERTY(Originals,(0));
    Originals.setSize(0);
    Placement.setStatus(App::Property::ReadOnly, true);

    ADD_PROPERTY_TYPE(Refine,(0),"SketchBased",(App::PropertyType)(App::Prop_None),"Refine shape (clean up redundant edges) after adding/subtracting");

    //init Refine property
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/PartDesign");
    this->Refine.setValue(hGrp->GetBool("RefineModel", false));
}

void Transformed::positionBySupport(void)
{
    // TODO May be here better to throw exception (silent=false) (2015-07-27, Fat-Zer)
    Part::Feature *support = getBaseObject(/* silent =*/ true);
    if (support)
        this->Placement.setValue(support->Placement.getValue());
}

Part::Feature* Transformed::getBaseObject(bool silent) const {
    Part::Feature *rv = Feature::getBaseObject(/* silent = */ true);
    if (rv) {
        return rv;
    }

    const char* err = nullptr;
    const std::vector<App::DocumentObject*> & originals = Originals.getValues();
    // NOTE: may be here supposed to be last origin but in order to keep the old behaviour keep here first 
    App::DocumentObject* firstOriginal = originals.empty() ? NULL : originals.front();
    if (firstOriginal) {
        if(firstOriginal->isDerivedFrom(Part::Feature::getClassTypeId())) {
            rv = static_cast<Part::Feature*>(firstOriginal);
        } else {
            err = "Transformation feature Linked object is not a Part object";
        }
    } else {
        err = "No originals linked to the transformed feature.";
    }

    if (!silent && err) {
        throw Base::RuntimeError(err);
    }

    return rv;
}

App::DocumentObject* Transformed::getSketchObject() const
{
    std::vector<DocumentObject*> originals = Originals.getValues();
    if (!originals.empty() && originals.front()->getTypeId().isDerivedFrom(PartDesign::ProfileBased::getClassTypeId())) {
        return (static_cast<PartDesign::ProfileBased*>(originals.front()))->getVerifiedSketch(true);
    }
    else if (!originals.empty() && originals.front()->getTypeId().isDerivedFrom(PartDesign::FeatureAddSub::getClassTypeId())) {
        return NULL;
    }
    else if (this->getTypeId().isDerivedFrom(LinearPattern::getClassTypeId())) {
        // if Originals is empty then try the linear pattern's Direction property
        const LinearPattern* pattern = static_cast<const LinearPattern*>(this);
        return pattern->Direction.getValue();
    }
    else if (this->getTypeId().isDerivedFrom(PolarPattern::getClassTypeId())) {
        // if Originals is empty then try the polar pattern's Axis property
        const PolarPattern* pattern = static_cast<const PolarPattern*>(this);
        return pattern->Axis.getValue();
    }
    else if (this->getTypeId().isDerivedFrom(Mirrored::getClassTypeId())) {
        // if Originals is empty then try the mirror pattern's MirrorPlane property
        const Mirrored* pattern = static_cast<const Mirrored*>(this);
        return pattern->MirrorPlane.getValue();
    }
    else {
        return 0;
    }
}

void Transformed::Restore(Base::XMLReader &reader)
{
    reader.readElement("Properties");
    int Cnt = reader.getAttributeAsInteger("Count");

    for (int i=0 ;i<Cnt ;i++) {
        reader.readElement("Property");
        const char* PropName = reader.getAttribute("name");
        const char* TypeName = reader.getAttribute("type");
        App::Property* prop = getPropertyByName(PropName);

        // The property 'Angle' of PolarPattern has changed from PropertyFloat
        // to PropertyAngle and the property 'Length' has changed to PropertyLength.
        try {
            if (prop && strcmp(prop->getTypeId().getName(), TypeName) == 0) {
                prop->Restore(reader);
            }
            else if (prop) {
                Base::Type inputType = Base::Type::fromName(TypeName);
                if (prop->getTypeId().isDerivedFrom(App::PropertyFloat::getClassTypeId()) &&
                    inputType.isDerivedFrom(App::PropertyFloat::getClassTypeId())) {
                    // Do not directly call the property's Restore method in case the implementation
                    // has changed. So, create a temporary PropertyFloat object and assign the value.
                    App::PropertyFloat floatProp;
                    floatProp.Restore(reader);
                    static_cast<App::PropertyFloat*>(prop)->setValue(floatProp.getValue());
                }
            }
        }
        catch (const Base::XMLParseException&) {
            throw; // re-throw
        }
        catch (const Base::Exception &e) {
            Base::Console().Error("%s\n", e.what());
        }
        catch (const std::exception &e) {
            Base::Console().Error("%s\n", e.what());
        }
        catch (const char* e) {
            Base::Console().Error("%s\n", e);
        }
#ifndef FC_DEBUG
        catch (...) {
            Base::Console().Error("Primitive::Restore: Unknown C++ exception thrown\n");
        }
#endif

        reader.readEndElement("Property");
    }
    reader.readEndElement("Properties");
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

    if(!this->BaseFeature.getValue()) {
        auto body = getFeatureBody();
        if(body) {
            body->setBaseProperty(this);
        }
    }

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
    Part::Feature* supportFeature;

    try {
        supportFeature = getBaseObject();
    } catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    const Part::TopoShape& supportTopShape = supportFeature->Shape.getShape();
    if (supportTopShape.getShape().IsNull())
        return new App::DocumentObjectExecReturn("Cannot transform invalid support shape");

    // create an untransformed copy of the support shape
    Part::TopoShape supportShape(supportTopShape);
    supportShape.setTransform(Base::Matrix4D());
    TopoDS_Shape support = supportShape.getShape();

    typedef std::set<std::vector<gp_Trsf>::const_iterator> trsf_it;
    typedef std::map<App::DocumentObject*,  trsf_it> rej_it_map;
    rej_it_map nointersect_trsfms;

    // NOTE: It would be possible to build a compound from all original addShapes/subShapes and then
    // transform the compounds as a whole. But we choose to apply the transformations to each
    // Original separately. This way it is easier to discover what feature causes a fuse/cut
    // to fail. The downside is that performance suffers when there are many originals. But it seems
    // safe to assume that in most cases there are few originals and many transformations
    for (std::vector<App::DocumentObject*>::const_iterator o = originals.begin(); o != originals.end(); ++o)
    {
        // Extract the original shape and determine whether to cut or to fuse
        TopoDS_Shape shape;
        bool fuse;

        if ((*o)->getTypeId().isDerivedFrom(PartDesign::FeatureAddSub::getClassTypeId())) {
            PartDesign::FeatureAddSub* feature = static_cast<PartDesign::FeatureAddSub*>(*o);
            shape = feature->AddSubShape.getShape().getShape();
            if (shape.IsNull())
                return new App::DocumentObjectExecReturn("Shape of additive feature is empty");
            
            fuse = (feature->getAddSubType() == FeatureAddSub::Additive) ? true : false;
        } 
        else {
            return new App::DocumentObjectExecReturn("Only additive and subtractive features can be transformed");
        }

        // Transform the add/subshape and collect the resulting shapes for overlap testing
        /*typedef std::vector<std::vector<gp_Trsf>::const_iterator> trsf_it_vec;
        trsf_it_vec v_transformations;
        std::vector<TopoDS_Shape> v_transformedShapes;*/

        std::vector<gp_Trsf>::const_iterator t = transformations.begin();
        ++t; // Skip first transformation, which is always the identity transformation
        for (; t != transformations.end(); ++t) {
            // Make an explicit copy of the shape because the "true" parameter to BRepBuilderAPI_Transform
            // seems to be pretty broken
            BRepBuilderAPI_Copy copy(shape);
            shape = copy.Shape();
            if (shape.IsNull())
                return new App::DocumentObjectExecReturn("Transformed: Linked shape object is empty");

            BRepBuilderAPI_Transform mkTrf(shape, *t, false); // No need to copy, now
            if (!mkTrf.IsDone())
                return new App::DocumentObjectExecReturn("Transformation failed", (*o));

            // Check for intersection with support
            try {

                if (!Part::checkIntersection(support, mkTrf.Shape(), false, true)) {
#ifdef FC_DEBUG // do not write this in release mode because a message appears already in the task view
                    Base::Console().Warning("Transformed shape does not intersect support %s: Removed\n", (*o)->getNameInDocument());
#endif
                    nointersect_trsfms[*o].insert(t);
                } else {
                    // We cannot wait to fuse a transformation with the support until all the transformations are done,
                    // because the "support" potentially changes with every transformation, basically when checking intersection
                    // above you need:
                    // 1. The original support
                    // 2. Any extra support gained by any previous transformation of any previous feature (multi-feature transform)
                    // 3. Any extra support gained by any previous transformation of this feature (feature multi-trasform)
                    //
                    // Therefore, if the transformation succeeded, then we fuse it with the support now, before checking the intersection
                    // of the next transformation.
                    
                    /*v_transformations.push_back(t);
                    v_transformedShapes.push_back(mkTrf.Shape());*/

                    // Note: Transformations that do not intersect the support are ignored in the overlap tests
                    
                    //insert scheme here.
                    /*TopoDS_Compound compoundTool;
                    std::vector<TopoDS_Shape> individualTools;
                    divideTools(v_transformedShapes, individualTools, compoundTool);*/
                    
                    // Fuse/Cut the compounded transformed shapes with the support
                    //TopoDS_Shape result;
                    TopoDS_Shape current = support;
                    
                    if (fuse) {
                        BRepAlgoAPI_Fuse mkFuse(current, mkTrf.Shape());
                        if (!mkFuse.IsDone())
                            return new App::DocumentObjectExecReturn("Fusion with support failed", *o);
                        // we have to get the solids (fuse sometimes creates compounds)
                        current = this->getSolid(mkFuse.Shape());
                        // lets check if the result is a solid
                        if (current.IsNull())
                            return new App::DocumentObjectExecReturn("Resulting shape is not a solid", *o);

                        /*std::vector<TopoDS_Shape>::const_iterator individualIt;
                        for (individualIt = individualTools.begin(); individualIt != individualTools.end(); ++individualIt)
                        {
                            BRepAlgoAPI_Fuse mkFuse2(current, *individualIt);
                            if (!mkFuse2.IsDone())
                                return new App::DocumentObjectExecReturn("Fusion with support failed", *o);
                            // we have to get the solids (fuse sometimes creates compounds)
                            current = this->getSolid(mkFuse2.Shape());
                            // lets check if the result is a solid
                            if (current.IsNull())
                                return new App::DocumentObjectExecReturn("Resulting shape is not a solid", *o);
                        }*/
                    } else {
                        BRepAlgoAPI_Cut mkCut(current, mkTrf.Shape());
                        if (!mkCut.IsDone())
                            return new App::DocumentObjectExecReturn("Cut out of support failed", *o);
                        current = mkCut.Shape();
                        /*std::vector<TopoDS_Shape>::const_iterator individualIt;
                        for (individualIt = individualTools.begin(); individualIt != individualTools.end(); ++individualIt)
                        {
                            BRepAlgoAPI_Cut mkCut2(current, *individualIt);
                            if (!mkCut2.IsDone())
                                return new App::DocumentObjectExecReturn("Cut out of support failed", *o);
                            current = this->getSolid(mkCut2.Shape());
                            if (current.IsNull())
                                return new App::DocumentObjectExecReturn("Resulting shape is not a solid", *o);
                        }*/
                    }
                    support = current; // Use result of this operation for fuse/cut of next original
                }
            } catch (Standard_Failure& e) {
                // Note: Ignoring this failure is probably pointless because if the intersection check fails, the later
                // fuse operation of the transformation result will also fail
        
                std::string msg("Transformation: Intersection check failed");
                if (e.GetMessageString() != NULL)
                    msg += std::string(": '") + e.GetMessageString() + "'";
                return new App::DocumentObjectExecReturn(msg.c_str());
            }
        }
    }
    support = refineShapeIfActive(support);

    for (rej_it_map::const_iterator it = nointersect_trsfms.begin(); it != nointersect_trsfms.end(); ++it)
        for (trsf_it::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
            rejected[it->first].push_back(**it2);

    int solidCount = countSolids(support);
    if (solidCount > 1) {
        return new App::DocumentObjectExecReturn("Transformed: Result has multiple solids. This is not supported at this time.");
    }

    this->Shape.setValue(getSolid(support));

    if (rejected.size() > 0) {
        return new App::DocumentObjectExecReturn("Transformation failed");
    }

    return App::DocumentObject::StdReturn;
}

TopoDS_Shape Transformed::refineShapeIfActive(const TopoDS_Shape& oldShape) const
{
    if (this->Refine.getValue()) {
        try {
            Part::BRepBuilderAPI_RefineModel mkRefine(oldShape);
            TopoDS_Shape resShape = mkRefine.Shape();
            return resShape;
        }
        catch (Standard_Failure&) {
            return oldShape;
        }
    }

    return oldShape;
}

void Transformed::divideTools(const std::vector<TopoDS_Shape> &toolsIn, std::vector<TopoDS_Shape> &individualsOut,
                              TopoDS_Compound &compoundOut) const
{
    typedef std::pair<TopoDS_Shape, Bnd_Box> ShapeBoundPair;
    typedef std::list<ShapeBoundPair> PairList;
    typedef std::vector<ShapeBoundPair> PairVector;

    PairList pairList;

    std::vector<TopoDS_Shape>::const_iterator it;
    for (it = toolsIn.begin(); it != toolsIn.end(); ++it) {
        Bnd_Box bound;
        BRepBndLib::Add(*it, bound);
        bound.SetGap(0.0);
        ShapeBoundPair temp = std::make_pair(*it, bound);
        pairList.push_back(temp);
    }

    BRep_Builder builder;
    builder.MakeCompound(compoundOut);

    while(!pairList.empty()) {
        PairVector currentGroup;
        currentGroup.push_back(pairList.front());
        pairList.pop_front();
        PairList::iterator it = pairList.begin();
        while(it != pairList.end()) {
            PairVector::const_iterator groupIt;
            bool found(false);
            for (groupIt = currentGroup.begin(); groupIt != currentGroup.end(); ++groupIt) {
                if (!(*it).second.IsOut((*groupIt).second)) {//touching means is out.
                    found = true;
                    break;
                }
            }
            if (found) {
                currentGroup.push_back(*it);
                pairList.erase(it);
                it=pairList.begin();
                continue;
            }
            ++it;
        }

        if (currentGroup.size() == 1) {
            builder.Add(compoundOut, currentGroup.front().first);
        }
        else {
            PairVector::const_iterator groupIt;
            for (groupIt = currentGroup.begin(); groupIt != currentGroup.end(); ++groupIt)
                individualsOut.push_back((*groupIt).first);
        }
    }
}

}
