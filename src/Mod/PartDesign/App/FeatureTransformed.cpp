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
#include <App/Document.h>
#include <Mod/Part/App/modelRefine.h>

using namespace PartDesign;
using namespace Part;

namespace PartDesign {

PROPERTY_SOURCE(PartDesign::Transformed, PartDesign::Feature)

Transformed::Transformed()
{
    ADD_PROPERTY(Originals,(0));
    Originals.setSize(0);
    ADD_PROPERTY(OriginalSubs,(0));
    Placement.setStatus(App::Property::ReadOnly, true);

    ADD_PROPERTY_TYPE(Refine,(0),"Base",(App::PropertyType)(App::Prop_None),"Refine shape (clean up redundant edges) after adding/subtracting");

    ADD_PROPERTY_TYPE(SubTransform,(true),"Base",(App::PropertyType)(App::Prop_None),
        "Transform sub feature instead of the solid if is an additive or substractive feature (e.g. Pad, Pocket)");

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
    const std::vector<App::DocumentObject*> & originals = OriginalSubs.getValues();
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
    std::vector<DocumentObject*> originals = OriginalSubs.getValues();
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

void Transformed::handleChangedPropertyType(
        Base::XMLReader &reader, const char * TypeName, App::Property * prop) 
{
    Base::Type inputType = Base::Type::fromName(TypeName);
    // The property 'Angle' of PolarPattern has changed from PropertyFloat
    // to PropertyAngle and the property 'Length' has changed to PropertyLength.
    if (prop->getTypeId().isDerivedFrom(App::PropertyFloat::getClassTypeId()) &&
            inputType.isDerivedFrom(App::PropertyFloat::getClassTypeId())) {
        // Do not directly call the property's Restore method in case the implementation
        // has changed. So, create a temporary PropertyFloat object and assign the value.
        App::PropertyFloat floatProp;
        floatProp.Restore(reader);
        static_cast<App::PropertyFloat*>(prop)->setValue(floatProp.getValue());
    }
}

short Transformed::mustExecute() const
{
    if (OriginalSubs.isTouched())
        return 1;
    return PartDesign::Feature::mustExecute();
}

App::DocumentObjectExecReturn *Transformed::execute(void)
{
    rejected.clear();

    auto originals = OriginalSubs.getSubListValues(true);
    if (originals.empty()) {
        if(!BaseFeature.getValue()) {
            // typically InsideMultiTransform
            return App::DocumentObject::StdReturn;
        }
        std::vector<std::string> subs;
        subs.emplace_back("");
        originals.emplace_back(BaseFeature.getValue(),subs);
    }

    if(!this->BaseFeature.getValue()) {
        auto body = getFeatureBody();
        if(body) {
            body->setBaseProperty(this);
        }
    }

    this->positionBySupport();

    TopTools_IndexedMapOfShape shapeMap;
    std::vector<TopoShape> originalShapes;
    std::vector<std::string> originalSubs;
    std::vector<bool> fuses;
    for(auto &v : originals) {
        auto obj = Base::freecad_dynamic_cast<PartDesign::Feature>(v.first);
        if(!obj) 
            continue;

        if (SubTransform.getValue() 
                && obj->isDerivedFrom(PartDesign::FeatureAddSub::getClassTypeId())) 
        {
            PartDesign::FeatureAddSub* feature = static_cast<PartDesign::FeatureAddSub*>(obj);
            auto shape = feature->AddSubShape.getShape();
            if (shape.isNull())
                return new App::DocumentObjectExecReturn("Shape of additive feature is empty");
            int count = shapeMap.Extent();
            if(shapeMap.Add(shape.getShape())<=count)
                continue;
            shape.Tag = -getID();
            originalShapes.push_back(shape);
            originalSubs.push_back(feature->getFullName());
            fuses.push_back((feature->getAddSubType() == FeatureAddSub::Additive) ? true : false);
            continue;
        } 

        for(auto &sub : v.second) {
            TopoShape baseShape = obj->Shape.getShape();
            std::vector<TopoShape> shapes;
            if(sub.empty()) 
                shapes = baseShape.getSubTopoShapes(TopAbs_SOLID);
            else {
                TopoDS_Shape subShape = baseShape.getSubShape(sub.c_str());
                if(subShape.IsNull())
                    return new App::DocumentObjectExecReturn("Shape of source feature is empty");
                int idx = baseShape.findAncestor(subShape,TopAbs_SOLID);
                if(idx)
                    shapes.push_back(baseShape.getSubTopoShape(TopAbs_SOLID,idx));
            }
            if(shapes.empty())
                return new App::DocumentObjectExecReturn("Non solid source feature");
            for(auto &s : shapes) {
                int count = shapeMap.Extent();
                if(shapeMap.Add(s.getShape())<=count)
                    continue;
                originalShapes.push_back(s);
                if(sub.size())
                    originalSubs.push_back(obj->getFullName() + '.' + sub);
                else
                    originalSubs.push_back(obj->getFullName());
                fuses.push_back(true);
            }
        }
    }

    // get transformations from subclass by calling virtual method
    std::vector<gp_Trsf> transformations;
    try {
        std::list<gp_Trsf> t_list = getTransformations(originalShapes);
        transformations.insert(transformations.end(), t_list.begin(), t_list.end());
    } catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    if (transformations.empty())
        return App::DocumentObject::StdReturn; // No transformations defined, exit silently

    // Get the support
    auto support = getBaseShape();
    if (support.isNull())
        return new App::DocumentObjectExecReturn("Cannot transform invalid support shape");

    // create an untransformed copy of the support shape
    support.setTransform(Base::Matrix4D());
    if(!support.Hasher)
        support.Hasher = getDocument()->getStringHasher();

    std::ostringstream ss;

    TopoShape result;

    // NOTE: It would be possible to build a compound from all original addShapes/subShapes and then
    // transform the compounds as a whole. But we choose to apply the transformations to each
    // Original separately. This way it is easier to discover what feature causes a fuse/cut
    // to fail. The downside is that performance suffers when there are many originals. But it seems
    // safe to assume that in most cases there are few originals and many transformations
    int i=0;
    for (TopoShape &shape : originalShapes) {
        auto &sub = originalSubs[i];
        bool fuse = fuses[i++];
        bool failed = false;

        // Transform the add/subshape and collect the resulting shapes for overlap testing
        /*typedef std::vector<std::vector<gp_Trsf>::const_iterator> trsf_it_vec;
        trsf_it_vec v_transformations;
        std::vector<TopoDS_Shape> v_transformedShapes;*/

        std::vector<gp_Trsf>::const_iterator t = transformations.begin();
        ++t; // Skip first transformation, which is always the identity transformation
        for (int idx=1; t != transformations.end(); ++t,++idx) {
            // Make an explicit copy of the shape because the "true" parameter to BRepBuilderAPI_Transform
            // seems to be pretty broken
            ss.str("");
            ss << 'I' << idx;
            auto shapeCopy = shape.makECopy(ss.str().c_str());
            if (shapeCopy.isNull())
                return new App::DocumentObjectExecReturn("Transformed: Linked shape object is empty");

            try {
                shapeCopy = shapeCopy.makETransform(*t);
            }catch(Standard_Failure &) {
                std::string msg("Transformation failed ");
                msg += sub;
                return new App::DocumentObjectExecReturn(msg.c_str());
            }

            // Check for intersection with support
            try {

                if (!allowMultiSolid()
                        && !Part::checkIntersection(support.getShape(), shapeCopy.getShape(), false, true)) {
#ifdef FC_DEBUG // do not write this in release mode because a message appears already in the task view
                    Base::Console().Warning("Transformed shape does not intersect support %s: Removed\n", sub.c_str());
#endif
                    if(!failed) {
                        failed = true;
                        rejected.emplace_back(shape,std::vector<gp_Trsf>());
                    }
                    rejected.back().second.push_back(*t);
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
                    
                    if (fuse) {
                        result.makEFuse({support,shapeCopy});
                        // we have to get the solids (fuse sometimes creates compounds)
                        support = this->getSolid(result);
                        // lets check if the result is a solid
                        if (support.isNull()) {
                            std::string msg("Resulting shape is not a solid: ");
                            msg += sub;
                            return new App::DocumentObjectExecReturn(msg.c_str());
                        }

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
                        result.makECut({support,shapeCopy});
                        support = result;
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
    result = refineShapeIfActive(result);

    this->Shape.setValue(getSolid(result));

    if (rejected.size() > 0) {
        return new App::DocumentObjectExecReturn("Transformation failed");
    }

    return App::DocumentObject::StdReturn;
}

TopoShape Transformed::refineShapeIfActive(const TopoShape& oldShape) const
{
    if (this->Refine.getValue()) 
        return oldShape.makERefine();
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

void Transformed::onDocumentRestored() {
    if(OriginalSubs.getValues().empty() && Originals.getSize()) {
        std::vector<std::string> subs(Originals.getSize());
        OriginalSubs.setValues(Originals.getValues(),subs);
    }
    PartDesign::Feature::onDocumentRestored();
}

void Transformed::onChanged(const App::Property *prop) {
    if (!this->isRestoring() 
            && this->getDocument()
            && !this->getDocument()->isPerformingTransaction()
            && !prop->testStatus(App::Property::User3))
    {
        if(prop == &Originals) {
            std::map<App::DocumentObject*,std::vector<std::string> > subMap;
            const auto &originals = OriginalSubs.getSubListValues();
            for(auto &v : originals) {
                auto &subs = subMap[v.first];
                subs.insert(subs.end(),v.second.begin(),v.second.end());
            }
            std::vector<App::PropertyLinkSubList::SubSet> subset;
            std::set<App::DocumentObject*> objSet;
            bool touched = false;
            for(auto obj : Originals.getValues()) {
                if(!objSet.insert(obj).second)
                    continue;
                auto it = subMap.find(obj);
                if(it == subMap.end()) {
                    touched = true;
                    subset.emplace_back(obj,std::vector<std::string>());
                    continue;
                }
                subset.emplace_back(it->first,std::move(it->second));
                subMap.erase(it);
            }
            if(subMap.size() || touched || originals!=subset) {
                OriginalSubs.setStatus(App::Property::User3,true);
                OriginalSubs.setSubListValues(subset);
                OriginalSubs.setStatus(App::Property::User3,false);
            }
        }else if(prop == &OriginalSubs) {
            std::set<App::DocumentObject*> objSet;
            std::vector<App::DocumentObject *> objs;
            for(auto obj : OriginalSubs.getValues()) {
                if(objSet.insert(obj).second)
                    objs.push_back(obj);
            }
            if(objs != Originals.getValues()) {
                Originals.setStatus(App::Property::User3,true);
                Originals.setValues(objs);
                Originals.setStatus(App::Property::User3,false);
            }
        }
    }
    PartDesign::Feature::onChanged(prop);
}
}
