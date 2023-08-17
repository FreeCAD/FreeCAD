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
# include <Bnd_Box.hxx>
# include <BRep_Builder.hxx>
# include <BRepAlgoAPI_Cut.hxx>
# include <BRepAlgoAPI_Fuse.hxx>
# include <BRepBndLib.hxx>
# include <BRepBuilderAPI_Copy.hxx>
# include <BRepBuilderAPI_Transform.hxx>
# include <Precision.hxx>
# include <TopExp_Explorer.hxx>
#endif

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <Base/Reader.h>
#include <Mod/Part/App/modelRefine.h>

#include "FeatureTransformed.h"
#include "Body.h"
#include "FeatureAddSub.h"
#include "FeatureMirrored.h"
#include "FeatureLinearPattern.h"
#include "FeaturePolarPattern.h"
#include "FeatureSketchBased.h"


using namespace PartDesign;

namespace PartDesign {

PROPERTY_SOURCE(PartDesign::Transformed, PartDesign::Feature)

Transformed::Transformed()
{
    ADD_PROPERTY(Originals,(nullptr));
    Originals.setSize(0);
    Placement.setStatus(App::Property::ReadOnly, true);

    ADD_PROPERTY_TYPE(Refine,(0),"Part Design",(App::PropertyType)(App::Prop_None),"Refine shape (clean up redundant edges) after adding/subtracting");

    //init Refine property
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/PartDesign");
    this->Refine.setValue(hGrp->GetBool("RefineModel", false));
}

void Transformed::positionBySupport()
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
    App::DocumentObject* firstOriginal = originals.empty() ? nullptr : originals.front();
    if (firstOriginal) {
        if(firstOriginal->isDerivedFrom(Part::Feature::getClassTypeId())) {
            rv = static_cast<Part::Feature*>(firstOriginal);
        } else {
            err = QT_TRANSLATE_NOOP("Exception", "Transformation feature Linked object is not a Part object");
        }
    } else {
        err = QT_TRANSLATE_NOOP("Exception", "No originals linked to the transformed feature.");
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
        return nullptr;
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
        return nullptr;
    }
}

void Transformed::Restore(Base::XMLReader &reader)
{
    PartDesign::Feature::Restore(reader);
}

void Transformed::handleChangedPropertyType(Base::XMLReader &reader, const char * TypeName, App::Property * prop)
{
    // The property 'Angle' of PolarPattern has changed from PropertyFloat
    // to PropertyAngle and the property 'Length' has changed to PropertyLength.
    Base::Type inputType = Base::Type::fromName(TypeName);
    if (prop->getTypeId().isDerivedFrom(App::PropertyFloat::getClassTypeId()) &&
        inputType.isDerivedFrom(App::PropertyFloat::getClassTypeId())) {
        // Do not directly call the property's Restore method in case the implementation
        // has changed. So, create a temporary PropertyFloat object and assign the value.
        App::PropertyFloat floatProp;
        floatProp.Restore(reader);
        static_cast<App::PropertyFloat*>(prop)->setValue(floatProp.getValue());
    }
    else {
        PartDesign::Feature::handleChangedPropertyType(reader, TypeName, prop);
    }
}

short Transformed::mustExecute() const
{
    if (Originals.isTouched())
        return 1;
    return PartDesign::Feature::mustExecute();
}

App::DocumentObjectExecReturn *Transformed::execute()
{
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
        return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Cannot transform invalid support shape"));

    // create an untransformed copy of the support shape
    Part::TopoShape supportShape(supportTopShape);

    gp_Trsf trsfInv = supportShape.getShape().Location().Transformation().Inverted();

    supportShape.setTransform(Base::Matrix4D());
    TopoDS_Shape support = supportShape.getShape();

    auto getTransformedCompShape = [&](const auto& origShape)
    {
        TopTools_ListOfShape shapeTools;
        std::vector<TopoDS_Shape> shapes;

        std::vector<gp_Trsf>::const_iterator transformIter = transformations.begin();

        // First transformation is skipped since it should not be part of the toolShape.
        ++transformIter;

        for (; transformIter != transformations.end(); ++transformIter) {
            // Make an explicit copy of the shape because the "true" parameter to BRepBuilderAPI_Transform
            // seems to be pretty broken
            BRepBuilderAPI_Copy copy(origShape);

            TopoDS_Shape shape = copy.Shape();

            BRepBuilderAPI_Transform mkTrf(shape, *transformIter, false); // No need to copy, now
            if (!mkTrf.IsDone()) {
                throw Base::CADKernelError(QT_TRANSLATE_NOOP("Exception", "Transformation failed"));
            }
            shape = mkTrf.Shape();

            shapes.emplace_back(shape);
        }

        for (const auto& shape : shapes)
            shapeTools.Append(shape);

        return shapeTools;
    };

    // NOTE: It would be possible to build a compound from all original addShapes/subShapes and then
    // transform the compounds as a whole. But we choose to apply the transformations to each
    // Original separately. This way it is easier to discover what feature causes a fuse/cut
    // to fail. The downside is that performance suffers when there are many originals. But it seems
    // safe to assume that in most cases there are few originals and many transformations
    for (auto original : originals)
    {
        // Extract the original shape and determine whether to cut or to fuse
        Part::TopoShape fuseShape;
        Part::TopoShape cutShape;

        if (original->getTypeId().isDerivedFrom(PartDesign::FeatureAddSub::getClassTypeId())) {
            PartDesign::FeatureAddSub* feature = static_cast<PartDesign::FeatureAddSub*>(original);
            feature->getAddSubShape(fuseShape, cutShape);
            if (fuseShape.isNull() && cutShape.isNull())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Shape of additive/subtractive feature is empty"));
            gp_Trsf trsf = feature->getLocation().Transformation().Multiplied(trsfInv);
            if (!fuseShape.isNull())
                fuseShape = fuseShape.makeTransform(trsf);
            if (!cutShape.isNull())
                cutShape = cutShape.makeTransform(trsf);
        }
        else {
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Only additive and subtractive features can be transformed"));
        }

        TopoDS_Shape current = support;
        if (!fuseShape.isNull()) {
            TopTools_ListOfShape shapeArguments;
            shapeArguments.Append(current);
            TopTools_ListOfShape shapeTools = getTransformedCompShape(fuseShape.getShape());
            if (!shapeTools.IsEmpty()) {
                std::unique_ptr<BRepAlgoAPI_BooleanOperation> mkBool(new BRepAlgoAPI_Fuse());
                mkBool->SetArguments(shapeArguments);
                mkBool->SetTools(shapeTools);
                mkBool->Build();
                if (!mkBool->IsDone()) {
                    return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Boolean operation failed"));
                }
                current = mkBool->Shape();
            }
        }
        if (!cutShape.isNull()) {
            TopTools_ListOfShape shapeArguments;
            shapeArguments.Append(current);
            TopTools_ListOfShape shapeTools = getTransformedCompShape(cutShape.getShape());
            if (!shapeTools.IsEmpty()) {
                std::unique_ptr<BRepAlgoAPI_BooleanOperation> mkBool(new BRepAlgoAPI_Cut());
                mkBool->SetArguments(shapeArguments);
                mkBool->SetTools(shapeTools);
                mkBool->Build();
                if (!mkBool->IsDone()) {
                    return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Boolean operation failed"));
                }
                current = mkBool->Shape();
            }
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
    using ShapeBoundPair = std::pair<TopoDS_Shape, Bnd_Box>;
    using PairList = std::list<ShapeBoundPair>;
    using PairVector = std::vector<ShapeBoundPair>;

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
