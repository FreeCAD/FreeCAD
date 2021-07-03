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

#ifndef FC_DEBUG
#include <ctime>
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

const char* Transformed::OverlapEnums[] = { "Detect", "Overlap mode", "Non-overlap mode", NULL};

PROPERTY_SOURCE(PartDesign::Transformed, PartDesign::Feature)

Transformed::Transformed()
{
    ADD_PROPERTY(Originals,(0));
    Originals.setSize(0);
    Placement.setStatus(App::Property::ReadOnly, true);

    ADD_PROPERTY_TYPE(Refine,(0),"Part Design",(App::PropertyType)(App::Prop_None),"Refine shape (clean up redundant edges) after adding/subtracting");
    ADD_PROPERTY_TYPE(Overlap, (0L), "Transform", App::Prop_None, "Feature overlapping behaviour");
    Overlap.setEnums(OverlapEnums);

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
    std::string overlapMode = Overlap.getValueAsString();
    bool overlapDetectionMode = overlapMode == "Detect";

#ifndef FC_DEBUG
    std::clock_t start0;
    start0 = std::clock();
#endif

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

    gp_Trsf trsfInv = supportShape.getShape().Location().Transformation().Inverted();

    supportShape.setTransform(Base::Matrix4D());
    TopoDS_Shape support = supportShape.getShape();

    // NOTE: It would be possible to build a compound from all original addShapes/subShapes and then
    // transform the compounds as a whole. But we choose to apply the transformations to each
    // Original separately. This way it is easier to discover what feature causes a fuse/cut
    // to fail. The downside is that performance suffers when there are many originals. But it seems
    // safe to assume that in most cases there are few originals and many transformations
    for (std::vector<App::DocumentObject*>::const_iterator o = originals.begin(); o != originals.end(); ++o)
    {
        // Extract the original shape and determine whether to cut or to fuse
        TopoDS_Shape shape;
        Part::TopoShape fuseShape;
        Part::TopoShape cutShape;

        if ((*o)->getTypeId().isDerivedFrom(PartDesign::FeatureAddSub::getClassTypeId())) {
            PartDesign::FeatureAddSub* feature = static_cast<PartDesign::FeatureAddSub*>(*o);
            feature->getAddSubShape(fuseShape, cutShape);
            if (fuseShape.isNull() && cutShape.isNull())
                return new App::DocumentObjectExecReturn("Shape of addsub feature is empty");
            gp_Trsf trsf = feature->getLocation().Transformation().Multiplied(trsfInv);
            if (!fuseShape.isNull())
                fuseShape = fuseShape.makETransform(trsf);
            if (!cutShape.isNull())
                cutShape = cutShape.makETransform(trsf);
        }
        else {
            return new App::DocumentObjectExecReturn("Only additive and subtractive features can be transformed");
        }
        TopoDS_Shape origShape = fuseShape.isNull()?cutShape.getShape():fuseShape.getShape();

        TopoDS_Shape current = support;

        BRep_Builder builder;
        TopoDS_Compound compShape;
        builder.MakeCompound(compShape);
        std::vector<TopoDS_Shape> shapes;
        bool overlapping = false;

        std::vector<gp_Trsf>::const_iterator t = transformations.begin();
        bool first = true;
        for (; t != transformations.end(); ++t) {
            // Make an explicit copy of the shape because the "true" parameter to BRepBuilderAPI_Transform
            // seems to be pretty broken
            BRepBuilderAPI_Copy copy(origShape);

            shape = copy.Shape();

            BRepBuilderAPI_Transform mkTrf(shape, *t, false); // No need to copy, now
            if (!mkTrf.IsDone())
                return new App::DocumentObjectExecReturn("Transformation failed", (*o));
            shape = mkTrf.Shape();

            shapes.emplace_back(shape);
            builder.Add(compShape, shape);

            if (overlapDetectionMode && !first)
                overlapping =  overlapping || (countSolids(TopoShape(origShape).fuse(shape))==1);

            if (first)
                first = false;
        }

        TopoDS_Shape toolShape;

#ifndef FC_DEBUG
        if (overlapping || overlapMode == "Overlap mode")
            Base::Console().Message("Transformed: Overlapping feature mode (fusing tool shapes)\n");
        else
            Base::Console().Message("Transformed: Non-Overlapping feature mode (compound of tool shapes)\n");
#endif

        if (overlapping || overlapMode == "Overlap mode")
            toolShape = TopoShape(origShape).fuse(shapes, Precision::Confusion());
        else
            toolShape = compShape;

        if (!fuseShape.isNull()) {
            std::unique_ptr<BRepAlgoAPI_BooleanOperation> mkBool(new BRepAlgoAPI_Fuse(current, toolShape));
            if (!mkBool->IsDone()) {
                std::stringstream error;
                error << "Boolean operation failed";
                return new App::DocumentObjectExecReturn(error.str());
            }
            current = mkBool->Shape();
        } else {
            std::unique_ptr<BRepAlgoAPI_BooleanOperation> mkBool(new BRepAlgoAPI_Cut(current, toolShape));
            if (!mkBool->IsDone()) {
                std::stringstream error;
                error << "Boolean operation failed";
                return new App::DocumentObjectExecReturn(error.str());
            }
            current = mkBool->Shape();
        }

        support = current; // Use result of this operation for fuse/cut of next original
    }

    support = refineShapeIfActive(support);

    int solidCount = countSolids(support);
    if (solidCount > 1) {
        Base::Console().Warning("Transformed: Result has multiple solids. Only keeping the first.\n");
    }

    this->Shape.setValue(getSolid(support));  // picking the first solid
    rejected = getRemainingSolids(support);

#ifndef FC_DEBUG
    Base::Console().Message("Transformed: Elapsed CPU time: %f s\n", (std::clock() - start0  ) / (double)(CLOCKS_PER_SEC));
#endif

    return App::DocumentObject::StdReturn;
}

TopoDS_Shape Transformed::refineShapeIfActive(const TopoDS_Shape& oldShape) const
{
    if (this->Refine.getValue()) {
        try {
            Part::BRepBuilderAPI_RefineModel mkRefine(oldShape);
            TopoDS_Shape resShape = mkRefine.Shape();
            if (!TopoShape(resShape).isClosed()) {
                return oldShape;
            }
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

TopoDS_Shape Transformed::getRemainingSolids(const TopoDS_Shape& shape)
{
    BRep_Builder builder;
    TopoDS_Compound compShape;
    builder.MakeCompound(compShape);

    if (shape.IsNull())
        Standard_Failure::Raise("Shape is null");
    TopExp_Explorer xp;
    xp.Init(shape,TopAbs_SOLID);
    xp.Next();  // skip the first

    for (; xp.More(); xp.Next()) {
        builder.Add(compShape, xp.Current());
    }

    return TopoDS_Shape(std::move(compShape));
}

}
