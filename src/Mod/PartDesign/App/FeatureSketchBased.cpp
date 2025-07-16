/***************************************************************************
 *   Copyright (c) 2010 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
# include <Bnd_Box.hxx>
# include <BRep_Builder.hxx>
# include <BRep_Tool.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <BRepBndLib.hxx>
# include <BRepBuilderAPI_Copy.hxx>
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepExtrema_DistShapeShape.hxx>
# include <BRepGProp.hxx>
# include <BRepGProp_Face.hxx>
# include <BRepLProp_SLProps.hxx>
# include <BRepProj_Projection.hxx>
# include <Extrema_ExtCC.hxx>
# include <Extrema_POnCurv.hxx>
# include <gp_Circ.hxx>
# include <gp_Pln.hxx>
# include <GProp_GProps.hxx>
# include <ShapeAnalysis.hxx>
# include <Standard_Version.hxx>
# include <TopExp.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS_Vertex.hxx>
# include <TopoDS_Wire.hxx>
# include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
#endif

#include <App/Document.h>
#include <App/Datums.h>
#include <Base/Reader.h>
#include <Mod/Part/App/FaceMakerCheese.h>

#include "FeatureSketchBased.h"
#include "DatumLine.h"
#include "DatumPlane.h"
#include "Mod/Part/App/Geometry.h"


FC_LOG_LEVEL_INIT("PartDesign",true,true);

using namespace PartDesign;

PROPERTY_SOURCE(PartDesign::ProfileBased, PartDesign::FeatureAddSub)

ProfileBased::ProfileBased()
{
    ADD_PROPERTY_TYPE(Profile, (nullptr), "SketchBased", App::Prop_None, "Reference to sketch");
    ADD_PROPERTY_TYPE(Midplane, (0), "SketchBased", App::Prop_None, "Extrude symmetric to sketch face");
    ADD_PROPERTY_TYPE(Reversed, (0), "SketchBased", App::Prop_None, "Reverse extrusion direction");
    ADD_PROPERTY_TYPE(UpToFace, (nullptr), "SketchBased", (App::PropertyType)(App::Prop_None), "Face where feature will end");
    ADD_PROPERTY_TYPE(UpToShape, (nullptr), "SketchBased", (App::PropertyType)(App::Prop_None), "Shape where feature will end");
    ADD_PROPERTY_TYPE(AllowMultiFace, (false), "SketchBased", App::Prop_None, "Allow multiple faces in profile");
}

short ProfileBased::mustExecute() const
{
    if (Profile.isTouched() ||
        Midplane.isTouched() ||
        Reversed.isTouched() ||
        UpToFace.isTouched())
        return 1;
    return PartDesign::FeatureAddSub::mustExecute();
}

void ProfileBased::setupObject()
{
    AllowMultiFace.setValue(true);
}

void ProfileBased::positionByPrevious()
{
    Part::Feature* feat = getBaseObject(/* silent = */ true);
    if (feat) {
        this->Placement.setValue(feat->Placement.getValue());
    }
    else {
        //no base. Use either Sketch support's placement, or sketch's placement itself.
        Part::Part2DObject* sketch = getVerifiedSketch();
        App::DocumentObject* support = sketch->AttachmentSupport.getValue();
        if (support && support->isDerivedFrom<App::GeoFeature>()) {
            this->Placement.setValue(static_cast<App::GeoFeature*>(support)->Placement.getValue());
        }
        else {
            this->Placement.setValue(sketch->Placement.getValue());
        }
    }
}

void ProfileBased::transformPlacement(const Base::Placement& transform)
{
    Part::Feature* feat = getBaseObject(/* silent = */ true);
    if (feat) {
        feat->transformPlacement(transform);
    }
    else {
        Part::Part2DObject* sketch = getVerifiedSketch();
        sketch->transformPlacement(transform);
    }
    positionByPrevious();
}

Part::Part2DObject* ProfileBased::getVerifiedSketch(bool silent) const {
    App::DocumentObject* result = Profile.getValue();
    const char* err = nullptr;

    if (!result) {
        err = "No profile linked at all";
    }
    else {
        if (!result->isDerivedFrom<Part::Part2DObject>()) {
            err = "Linked object is not a Sketch or Part2DObject";
            result = nullptr;
        }
    }

    if (!silent && err) {
        throw Base::RuntimeError(err);
    }

    return static_cast<Part::Part2DObject*>(result);
}

Part::Feature* ProfileBased::getVerifiedObject(bool silent) const {

    App::DocumentObject* result = Profile.getValue();
    const char* err = nullptr;

    if (!result) {
        err = "No object linked";
    }
    else {
        if (!result->isDerivedFrom<Part::Feature>())
            err = "Linked object is not a Sketch, Part2DObject or Feature";
    }

    if (!silent && err) {
        throw Base::RuntimeError(err);
    }

    return static_cast<Part::Feature*>(result);
}
TopoShape ProfileBased::getTopoShapeVerifiedFace(bool silent,
                                                 [[maybe_unused]]bool doFit, // TODO: Remove parameter
                                                 bool allowOpen,
                                                 const App::DocumentObject* profile,
                                                 const std::vector<std::string>& _subs) const
{
    auto obj = profile ? profile : Profile.getValue();
    if (!obj || !obj->getNameInDocument()) {
        if (silent) {
            return TopoShape();
        }
        throw Base::ValueError("No profile linked");
    }
    const auto& subs = profile ? _subs : Profile.getSubValues();
    try {
        TopoShape shape;
        if (AllowMultiFace.getValue()) {
            if (subs.empty()) {
                shape = Part::Feature::getTopoShape(obj, Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform);
            }
            else {
                std::vector<TopoShape> shapes;
                for (auto& sub : subs) {
                    auto subshape = Part::Feature::getTopoShape(obj,
                                                                  Part::ShapeOption::NeedSubElement
                                                                | Part::ShapeOption::ResolveLink
                                                                | Part::ShapeOption::Transform,
                                                                sub.c_str());


                    if (subshape.isNull()) {
                        FC_THROWM(Base::CADKernelError,
                                  "Sub shape not found: " << obj->getFullName() << "." << sub);
                    }
                    shapes.push_back(subshape);
                }
                shape.makeElementCompound(shapes);
            }
        }
        else {
            std::string sub;
            if (!obj->isDerivedFrom<Part::Part2DObject>()) {
                if (!subs.empty()) {
                    sub = subs[0];
                }
            }
            shape = Part::Feature::getTopoShape(obj,
                                                  (sub.empty() ? Part::ShapeOption::NoFlag : Part::ShapeOption::NeedSubElement)
                                                | Part::ShapeOption::ResolveLink
                                                | Part::ShapeOption::Transform,
                                                sub.c_str());
        }
        if (shape.isNull()) {
            if (silent) {
                return shape;
            }
            throw Base::CADKernelError("Linked shape object is empty");
        }
        TopoShape openshape;
        if (!shape.hasSubShape(TopAbs_FACE)) {
            try {
                if (!shape.hasSubShape(TopAbs_WIRE)) {
                    shape = shape.makeElementWires();
                }
                if (shape.hasSubShape(TopAbs_WIRE)) {
                    shape.Hasher = getDocument()->getStringHasher();
                    if (allowOpen) {
                        std::vector<TopoShape> openwires;
                        std::vector<TopoShape> wires;
                        for (auto& wire : shape.getSubTopoShapes(TopAbs_WIRE)) {
                            if (!wire.isClosed()) {
                                openwires.push_back(wire);
                            }
                            else {
                                wires.push_back(wire);
                            }
                        }
                        if (openwires.size()) {
                            openshape.makeElementCompound(
                                openwires,
                                nullptr,
                                TopoShape ::SingleShapeCompoundCreationPolicy::returnShape);
                            if (wires.empty()) {
                                shape = TopoShape();
                            }
                            else {
                                shape.makeElementCompound(
                                    wires,
                                    nullptr,
                                    TopoShape ::SingleShapeCompoundCreationPolicy::returnShape);
                            }
                        }
                    }
                    if (!shape.isNull()) {
                        if (AllowMultiFace.getValue()) {
                            shape = shape.makeElementFace();  // default to use FaceMakerBullseye
                        }
                        else {
                            shape = shape.makeElementFace(nullptr, "Part::FaceMakerCheese");
                        }
                    }
                }
            }
            catch (const Base::Exception&) {
                if (silent) {
                    return TopoShape();
                }
                throw;
            }
            catch (const Standard_Failure&) {
                if (silent) {
                    return TopoShape();
                }
                throw;
            }
        }
        int count = shape.countSubShapes(TopAbs_FACE);
        if (!count && !allowOpen) {
            if (silent) {
                return TopoShape();
            }
            throw Base::CADKernelError("Cannot make face from profile");
        }

        if (!openshape.isNull()) {
            if (shape.isNull()) {
                shape = openshape;
            }
            else {
                shape.makeElementCompound({shape, openshape});
            }
        }
        if (count > 1) {
            if (AllowMultiFace.getValue()
                || obj->isDerivedFrom<Part::Part2DObject>()) {
                return shape;
            }
            FC_WARN("Found more than one face from profile");
        }
        if (!openshape.isNull()) {
            return shape;
        }
        if (count) {
            return shape.getSubTopoShape(TopAbs_FACE, 1);
        }
        return shape;
    }
    catch (Standard_Failure&) {
        if (silent) {
            return TopoShape();
        }
        throw;
    }
}

// TODO: Toponaming April 2024 Deprecated in favor of TopoShape method.  Remove when possible.
TopoDS_Shape ProfileBased::getVerifiedFace(bool silent) const {

    App::DocumentObject* result = Profile.getValue();
    const char* err = nullptr;
    std::string _err;

    if (!result) {
        err = "No profile linked";
    }
    else if (AllowMultiFace.getValue()) {
        try {
            auto shape = getProfileShape();
            if (shape.isNull())
                err = "Linked shape object is empty";
            else {
                auto faces = shape.getSubTopoShapes(TopAbs_FACE);
                if (faces.empty()) {
                    if (!shape.hasSubShape(TopAbs_WIRE))
                        shape = shape.makeWires();
                    if (shape.hasSubShape(TopAbs_WIRE))
                        shape = shape.makeFace(nullptr, "Part::FaceMakerBullseye");
                    else
                        err = "Cannot make face from profile";
                }
                else if (faces.size() == 1)
                    shape = faces.front();
                else
                    shape = TopoShape().makeCompound(faces);
            }
            if (!err)
                return shape.getShape();
        }
        catch (Standard_Failure& e) {
            _err = e.GetMessageString();
            err = _err.c_str();
        }
    }
    else {
        if (result->isDerivedFrom<Part::Part2DObject>()) {

            auto wires = getProfileWires();
            return Part::FaceMakerCheese::makeFace(wires);
        }
        else if (result->isDerivedFrom<Part::Feature>()) {
            if (Profile.getSubValues().empty())
                err = "Linked object has no subshape specified";
            else {

                const Part::TopoShape& shape = Profile.getValue<Part::Feature*>()->Shape.getShape();
                TopoDS_Shape sub = shape.getSubShape(Profile.getSubValues()[0].c_str());
                if (sub.ShapeType() == TopAbs_FACE)
                    return TopoDS::Face(sub);
                else if (sub.ShapeType() == TopAbs_WIRE) {

                    auto wire = TopoDS::Wire(sub);
                    if (!wire.Closed())
                        err = "Linked wire is not closed";
                    else {
                        BRepBuilderAPI_MakeFace mk(wire);
                        mk.Build();
                        return TopoDS::Face(mk.Shape());
                    }
                }
                else
                    err = "Linked Subshape cannot be used";
            }
        }
        else
            err = "Linked object is neither Sketch, Part2DObject or Part::Feature";
    }

    if (!silent && err) {
        throw Base::RuntimeError(err);
    }

    return TopoDS_Face();
}

TopoShape ProfileBased::getProfileShape(Part::ShapeOptions subShapeOptions) const
{
    TopoShape shape;
    const auto& subs = Profile.getSubValues();
    auto profile = Profile.getValue();
    if (subs.empty()) {
        shape = Part::Feature::getTopoShape(profile, Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform);
    }
    else {
        std::vector<TopoShape> shapes;
        for (auto& sub : subs) {
            shapes.push_back(Part::Feature::getTopoShape(profile,
                                                         subShapeOptions,
                                                         sub.c_str()));
        }
        shape = TopoShape(shape.Tag).makeElementCompound(shapes);
    }
    if (shape.isNull()) {
        throw Part::NullShapeException("Linked shape object is empty");
    }
    return shape;
}


// TODO: Toponaming April 2024 Deprecated in favor of TopoShape method.  Remove when possible.
std::vector<TopoDS_Wire> ProfileBased::getProfileWires() const {
    std::vector<TopoDS_Wire> result;

    if (!Profile.getValue() || !Profile.getValue()->isDerivedFrom<Part::Feature>())
        throw Base::TypeError("No valid profile linked");

    TopoDS_Shape shape;
    if (Profile.getValue()->isDerivedFrom<Part::Part2DObject>())
        shape = Profile.getValue<Part::Part2DObject*>()->Shape.getValue();
    else {
        if (Profile.getSubValues().empty())
            throw Base::ValueError("No valid subelement linked in Part::Feature");

        shape = Profile.getValue<Part::Feature*>()->Shape.getShape().getSubShape(Profile.getSubValues().front().c_str());
    }

    if (shape.IsNull())
        throw Base::ValueError("Linked shape object is empty");

    // this is a workaround for an obscure OCC bug which leads to empty tessellations
    // for some faces. Making an explicit copy of the linked shape seems to fix it.
    // The error almost happens when re-computing the shape but sometimes also for the
    // first time
    BRepBuilderAPI_Copy copy(shape);
    shape = copy.Shape();
    if (shape.IsNull())
        throw Base::ValueError("Linked shape object is empty");

    TopExp_Explorer ex;
    for (ex.Init(shape, TopAbs_WIRE); ex.More(); ex.Next()) {
        result.push_back(TopoDS::Wire(ex.Current()));
    }
    if (result.empty()) // there can be several wires
        throw Base::ValueError("Linked shape object is not a wire");

    return result;
}

std::vector<TopoShape> ProfileBased::getTopoShapeProfileWires() const
{
    // shape copy is a workaround for an obscure OCC bug which leads to empty
    // tessellations for some faces. Making an explicit copy of the linked
    // shape seems to fix it.  The error mostly happens when re-computing the
    // shape but sometimes also for the first time
    auto shape = getProfileShape().makeElementCopy();

    if (shape.hasSubShape(TopAbs_WIRE)) {
        return shape.getSubTopoShapes(TopAbs_WIRE);
    }

    auto wires = shape.makeElementWires().getSubTopoShapes(TopAbs_WIRE);
    if (wires.empty()) {
        throw Part::NullShapeException("Linked shape object is not a wire");
    }
    return wires;
}

// Note: We cannot return a reference, because it will become Null.
// Not clear where, because we check for IsNull() here, but as soon as it is passed out of
// this method, it becomes null!
// TODO: Toponaming April 2024 Deprecated in favor of TopoShape method.  Remove when possible.
const TopoDS_Face ProfileBased::getSupportFace() const
{
    const Part::Part2DObject* sketch = getVerifiedSketch(true);
    if (sketch) {
        return getSupportFace(sketch);
    }

    return getSupportFace(Profile);
}

TopoDS_Face ProfileBased::getSupportFace(const Part::Part2DObject* sketch) const
{
    if (sketch && sketch->MapMode.getValue() == Attacher::mmFlatFace
        && sketch->AttachmentSupport.getValue()) {
        const auto& AttachmentSupport = sketch->AttachmentSupport;
        App::DocumentObject* ref = AttachmentSupport.getValue();

        Part::Feature* part = dynamic_cast<Part::Feature*>(ref);
        if (part) {
            const std::vector<std::string>& sub = AttachmentSupport.getSubValues();
            assert(sub.size() == 1);

            if (sub.at(0).empty()) {
                // This seems to happen when sketch is on a datum plane
                return TopoDS::Face(Feature::makeShapeFromPlane(sketch));
            }

            // get the selected sub shape (a Face)
            const Part::TopoShape& shape = part->Shape.getShape();
            if (shape.getShape().IsNull()) {
                throw Base::ValueError("Sketch support shape is empty!");
            }

            TopoDS_Shape sh = shape.getSubShape(sub[0].c_str());
            if (sh.IsNull()) {
                throw Base::ValueError("Null shape in SketchBased::getSupportFace()!");
            }

            const TopoDS_Face face = TopoDS::Face(sh);
            if (face.IsNull()) {
                throw Base::ValueError("Null face in SketchBased::getSupportFace()!");
            }

            BRepAdaptor_Surface adapt(face);
            if (adapt.GetType() != GeomAbs_Plane) {
                throw Base::TypeError("No planar face in SketchBased::getSupportFace()!");
            }

            return face;
        }
    }
    return TopoDS::Face(Feature::makeShapeFromPlane(sketch));
}

TopoDS_Face ProfileBased::getSupportFace(const App::PropertyLinkSub& link) const
{
    App::DocumentObject* result = link.getValue();
    if (!result) {
        throw Base::RuntimeError("No support linked");
    }

    TopoDS_Face face;
    getFaceFromLinkSub(face, link);
    return face;
}

TopoShape ProfileBased::getTopoShapeSupportFace() const
{
    TopoShape shape;
    const Part::Part2DObject* sketch = getVerifiedSketch(true);
    if (!sketch) {
        shape = getTopoShapeVerifiedFace();
    }
    else if (sketch->MapMode.getValue() == Attacher::mmFlatFace
             && sketch->AttachmentSupport.getValue()) {
        const auto& Support = sketch->AttachmentSupport;
        App::DocumentObject* ref = Support.getValue();
        shape = Part::Feature::getTopoShape(
                        ref,
                            Part::ShapeOption::NeedSubElement
                          | Part::ShapeOption::ResolveLink
                          | Part::ShapeOption::Transform,
                        Support.getSubValues().empty() ? "" : Support.getSubValues()[0].c_str());
    }
    if (!shape.isNull()) {
        if (shape.shapeType(true) != TopAbs_FACE) {
            if (!shape.hasSubShape(TopAbs_FACE)) {
                throw Base::ValueError("Null face in SketchBased::getSupportFace()!");
            }
            shape = shape.getSubTopoShape(TopAbs_FACE, 1);
        }
        gp_Pln pln;
        if (!shape.findPlane(pln)) {
            throw Base::TypeError("No planar face in SketchBased::getSupportFace()!");
        }

        return shape;
    }
    if (!sketch) {
        throw Base::RuntimeError("No planar support");
    }
    return Feature::makeShapeFromPlane(sketch);
}

int ProfileBased::getSketchAxisCount() const
{
    Part::Part2DObject* sketch = static_cast<Part::Part2DObject*>(Profile.getValue());
    if (!sketch)
        return -1; // the link to the sketch is lost
    return sketch->getAxisCount();
}

Part::Feature* ProfileBased::getBaseObject(bool silent) const
{
    // Test the base's class feature.
    Part::Feature* rv = Feature::getBaseObject(/* silent = */ true);
    if (rv) {
        return rv;
    }

    // getVerifiedObject() may throw it's own exception if fail
    Part::Feature* obj = getVerifiedObject(silent);

    if (!obj)
        return nullptr;

    if (!obj->isDerivedFrom<Part::Part2DObject>())
        return obj;

    //due to former test we know we have a 2d object
    Part::Part2DObject* sketch = getVerifiedSketch(silent);
    const char* err = nullptr;
    App::DocumentObject* spt = sketch->AttachmentSupport.getValue();
    if (spt) {
        if (spt->isDerivedFrom<Part::Feature>()) {
            rv = static_cast<Part::Feature*>(spt);
        }
        else {
            err = "No base set, sketch support is not Part::Feature";
        }
    }
    else {
        err = "No base set, no sketch support either";
    }

    if (!silent && err) {
        throw Base::RuntimeError(err);
    }

    return rv;
}

void ProfileBased::onChanged(const App::Property* prop)
{
    if (prop == &Profile) {
        // if attached to a sketch then mark it as read-only
        this->Placement.setStatus(App::Property::ReadOnly, Profile.getValue() != nullptr);
    }

    FeatureAddSub::onChanged(prop);
}

void ProfileBased::getUpToFaceFromLinkSub(TopoShape& upToFace, const App::PropertyLinkSub& refFace)
{
    App::DocumentObject* ref = refFace.getValue();

    if (!ref) {
        throw Base::ValueError("SketchBased: No face selected");
    }

    if (ref->isDerivedFrom<App::Plane>()) {
        upToFace = makeShapeFromPlane(ref);
        return;
    }

    const auto& subs = refFace.getSubValues();
    upToFace = Part::Feature::getTopoShape(
        ref,
          Part::ShapeOption::NeedSubElement
        | Part::ShapeOption::ResolveLink
        | Part::ShapeOption::Transform,
        subs.empty() ? nullptr : subs[0].c_str());

    if (!upToFace.hasSubShape(TopAbs_FACE)) {
        throw Base::ValueError("SketchBased: Up to face: Failed to extract face");
    }
}

int ProfileBased::getUpToShapeFromLinkSubList(TopoShape& upToShape, const App::PropertyLinkSubList& refShape)
{
    int ret = 0;

    auto subSets = refShape.getSubListValues();

    std::vector<TopoShape> faceList;
    for (auto &subSet : subSets){
        auto ref = subSet.first;
        if (ref->isDerivedFrom<App::Plane>()) {
            faceList.push_back(makeTopoShapeFromPlane(ref));
            ret ++;
        } else {
            if (!ref->isDerivedFrom<Part::Feature>())
                throw Base::TypeError("SketchBased: Must be face of a feature");

            auto subStrings = subSet.second;
            if (subStrings.empty() || subStrings[0].empty()) {
                TopoShape baseShape = Part::Feature::getTopoShape(ref,
                                                                    Part::ShapeOption::NeedSubElement
                                                                  | Part::ShapeOption::ResolveLink
                                                                  | Part::ShapeOption::Transform);


                for (auto face : baseShape.getSubTopoShapes(TopAbs_FACE)){
                    faceList.push_back(face);
                    ret ++;
                }
            }
            else {
                for (auto &subString : subStrings){
                    auto shape = Part::Feature::getShape(
                                    ref, 
                                      Part::ShapeOption::NeedSubElement
                                    | Part::ShapeOption::ResolveLink
                                    | Part::ShapeOption::Transform,
                                    subString.c_str());

                    TopoShape face = shape;
                    face = face.makeElementFace();
                    if (face.isNull()) {
                        throw Base::ValueError("SketchBased: Failed to extract face");
                    }
                    faceList.push_back(face);
                    ret ++;
                }
            }
        }
    }
    if (ret == 0){
        return 0;
    }
    if (ret == 1){
        upToShape = faceList[0];
        return 1;
    }

    // create a unique shell with all selected faces
    upToShape = upToShape.makeElementCompound(faceList);
    return ret;
}

void ProfileBased::getFaceFromLinkSub(TopoDS_Face& upToFace, const App::PropertyLinkSub& refFace)
{
    App::DocumentObject* ref = refFace.getValue();
    std::vector<std::string> subStrings = refFace.getSubValues();

    if (!ref)
        throw Base::ValueError("SketchBased: No face selected");

    if (ref->isDerivedFrom<App::Plane>()) {
        upToFace = TopoDS::Face(makeShapeFromPlane(ref));
        return;
    }
    else if (ref->isDerivedFrom<PartDesign::Plane>()) {
        Part::Datum* datum = static_cast<Part::Datum*>(ref);
        upToFace = TopoDS::Face(datum->getShape());
        return;
    }

    if (!ref->isDerivedFrom<Part::Feature>())
        throw Base::TypeError("SketchBased: Must be face of a feature");
    Part::TopoShape baseShape = static_cast<Part::Feature*>(ref)->Shape.getShape();

    // Allow an empty sub here - example is a sketch reference (no sub) that creates a face.
    if (subStrings.empty() )
        throw Base::ValueError("SketchBased: No face selected");
    // TODO: Check for multiple UpToFaces?

    upToFace = TopoDS::Face(baseShape.getSubShape(subStrings[0].c_str()));
    if (upToFace.IsNull())
        throw Base::ValueError("SketchBased: Failed to extract face");
}


void ProfileBased::getUpToFace(TopoShape& upToFace,
                               const TopoShape& support,
                               const TopoShape& sketchshape,
                               const std::string& method,
                               gp_Dir& dir)
{
    if ((method == "UpToLast") || (method == "UpToFirst")) {
        std::vector<Part::cutTopoShapeFaces> cfaces =
            Part::findAllFacesCutBy(support, sketchshape, dir);
        if (cfaces.empty()) {
            throw Base::ValueError("SketchBased: No faces found in this direction");
        }

        // Find nearest/furthest face
        std::vector<Part::cutTopoShapeFaces>::const_iterator it, it_near, it_far;
        it_near = it_far = cfaces.begin();
        for (it = cfaces.begin(); it != cfaces.end(); it++) {
            if (it->distsq > it_far->distsq) {
                it_far = it;
            }
            else if (it->distsq < it_near->distsq) {
                it_near = it;
            }
        }
        upToFace = (method == "UpToLast" ? it_far->face : it_near->face);
    }
    else if (Part::findAllFacesCutBy(upToFace, sketchshape, dir).empty()) {
        dir = -dir;
    }

    if (upToFace.shapeType(true) != TopAbs_FACE) {
        if (!upToFace.hasSubShape(TopAbs_FACE)) {
            throw Base::ValueError("SketchBased: Up to face: No face found");
        }
        upToFace = upToFace.getSubTopoShape(TopAbs_FACE, 1);
    }

    TopoDS_Face face = TopoDS::Face(upToFace.getShape());

    // Check that the upToFace does not intersect the sketch face and
    // is not parallel to the extrusion direction
    BRepAdaptor_Surface adapt(face);

    if (adapt.GetType() == GeomAbs_Plane) {
        if (dir.IsNormal(adapt.Plane().Axis().Direction(), Precision::Confusion())) {
            throw Base::ValueError(
                "SketchBased: Up to face: Must not be parallel to extrusion direction!");
        }
    }

    // We must measure from sketchshape, not supportface, here
    BRepExtrema_DistShapeShape distSS(sketchshape.getShape(), face);
    if (distSS.Value() < Precision::Confusion()) {
        throw Base::ValueError("SketchBased: Up to face: Must not intersect sketch!");
    }
}

void ProfileBased::addOffsetToFace(TopoShape& upToFace, const gp_Dir& dir, double offset)
{
    // Move the face in the extrusion direction
    // TODO: For non-planar faces, we could consider offsetting the surface
    if (fabs(offset) > Precision::Confusion()) {
        gp_Trsf mov;
        mov.SetTranslation(offset * gp_Vec(dir));
        TopLoc_Location loc(mov);
        upToFace.move(loc);
    }
}

double ProfileBased::getThroughAllLength() const
{
    TopoShape profileshape;
    TopoShape base;
    profileshape = getTopoShapeVerifiedFace(true);
    base = getBaseTopoShape();
    Bnd_Box box;
    BRepBndLib::Add(base.getShape(), box);

    if (!profileshape.isNull()) {
        BRepBndLib::Add(profileshape.getShape(), box);
    }
    box.SetGap(0.0);
    // The diagonal of the bounding box, plus 1%  extra to eliminate risk of
    // co-planar issues, gives a length that is guaranteed to go through all.
    // The result is multiplied by 2 for the guarantee to work also for the midplane option.
    return 2.02 * sqrt(box.SquareExtent());
}

bool ProfileBased::checkWireInsideFace(const TopoDS_Wire& wire, const TopoDS_Face& face,
                                       const gp_Dir& dir) {
    // Project wire onto the face (face, not surface! So limits of face apply)
    // FIXME: The results of BRepProj_Projection do not seem to be very stable. Sometimes they return no result
    // even in the simplest projection case.
    // FIXME: Checking for Closed() is wrong because this has nothing to do with the wire itself being closed
    // But ShapeAnalysis_Wire::CheckClosed() doesn't give correct results either.
    BRepProj_Projection proj(wire, face, dir);
    return (proj.More() && proj.Current().Closed());
}

bool ProfileBased::checkLineCrossesFace(const gp_Lin& line, const TopoDS_Face& face)
{
    BRepBuilderAPI_MakeEdge mkEdge(line);
    TopoDS_Wire wire = ShapeAnalysis::OuterWire(face);
    BRepExtrema_DistShapeShape distss(wire, mkEdge.Shape(), Precision::Confusion());
    if (distss.IsDone()) {
        if (distss.Value() > Precision::Confusion())
            return false;
        // build up map vertex->edge
        TopTools_IndexedDataMapOfShapeListOfShape vertex2Edge;
        TopExp::MapShapesAndAncestors(wire, TopAbs_VERTEX, TopAbs_EDGE, vertex2Edge);

        for (Standard_Integer i = 1; i <= distss.NbSolution(); i++) {
            if (distss.PointOnShape1(i).Distance(distss.PointOnShape2(i)) > Precision::Confusion())
                continue;
            BRepExtrema_SupportType type = distss.SupportTypeShape1(i);
            if (type == BRepExtrema_IsOnEdge) {
                TopoDS_Edge edge = TopoDS::Edge(distss.SupportOnShape1(i));
                BRepAdaptor_Curve adapt(edge);
                // create a plane (pnt,dir) that goes through the intersection point and is built of
                // the vectors of the sketch normal and the rotation axis
                gp_Dir normal = BRepAdaptor_Surface(face).Plane().Axis().Direction();
                gp_Dir dir = line.Direction().Crossed(normal);
                gp_Pnt pnt = distss.PointOnShape1(i);

                Standard_Real t;
                distss.ParOnEdgeS1(i, t);
                gp_Pnt p_eps1 = adapt.Value(std::max<double>(adapt.FirstParameter(), t - 10 * Precision::Confusion()));
                gp_Pnt p_eps2 = adapt.Value(std::min<double>(adapt.LastParameter(), t + 10 * Precision::Confusion()));

                // now check if we get a change in the sign of the distances
                Standard_Real dist_p_eps1_pnt = gp_Vec(p_eps1, pnt).Dot(gp_Vec(dir));
                Standard_Real dist_p_eps2_pnt = gp_Vec(p_eps2, pnt).Dot(gp_Vec(dir));
                // distance to the plane must be noticeable
                if (fabs(dist_p_eps1_pnt) > 5 * Precision::Confusion() &&
                    fabs(dist_p_eps2_pnt) > 5 * Precision::Confusion()) {
                    if (dist_p_eps1_pnt * dist_p_eps2_pnt < 0)
                        return true;
                }
            }
            else if (type == BRepExtrema_IsVertex) {
                // for a vertex check the two adjacent edges if there is a change of sign
                TopoDS_Vertex vertex = TopoDS::Vertex(distss.SupportOnShape1(i));
                const TopTools_ListOfShape& edges = vertex2Edge.FindFromKey(vertex);
                if (edges.Extent() == 2) {
                    // create a plane (pnt,dir) that goes through the intersection point and is built of
                    // the vectors of the sketch normal and the rotation axis
                    BRepAdaptor_Surface adapt(face);
                    gp_Dir normal = adapt.Plane().Axis().Direction();
                    gp_Dir dir = line.Direction().Crossed(normal);
                    gp_Pnt pnt = distss.PointOnShape1(i);

                    // from the first edge get a point next to the intersection point
                    const TopoDS_Edge& edge1 = TopoDS::Edge(edges.First());
                    BRepAdaptor_Curve adapt1(edge1);
                    Standard_Real dist1 = adapt1.Value(adapt1.FirstParameter()).SquareDistance(pnt);
                    Standard_Real dist2 = adapt1.Value(adapt1.LastParameter()).SquareDistance(pnt);
                    gp_Pnt p_eps1;
                    if (dist1 < dist2)
                        p_eps1 = adapt1.Value(adapt1.FirstParameter() + 2 * Precision::Confusion());
                    else
                        p_eps1 = adapt1.Value(adapt1.LastParameter() - 2 * Precision::Confusion());

                    // from the second edge get a point next to the intersection point
                    const TopoDS_Edge& edge2 = TopoDS::Edge(edges.Last());
                    BRepAdaptor_Curve adapt2(edge2);
                    Standard_Real dist3 = adapt2.Value(adapt2.FirstParameter()).SquareDistance(pnt);
                    Standard_Real dist4 = adapt2.Value(adapt2.LastParameter()).SquareDistance(pnt);
                    gp_Pnt p_eps2;
                    if (dist3 < dist4)
                        p_eps2 = adapt2.Value(adapt2.FirstParameter() + 2 * Precision::Confusion());
                    else
                        p_eps2 = adapt2.Value(adapt2.LastParameter() - 2 * Precision::Confusion());

                    // now check if we get a change in the sign of the distances
                    Standard_Real dist_p_eps1_pnt = gp_Vec(p_eps1, pnt).Dot(gp_Vec(dir));
                    Standard_Real dist_p_eps2_pnt = gp_Vec(p_eps2, pnt).Dot(gp_Vec(dir));
                    // distance to the plane must be noticeable
                    if (fabs(dist_p_eps1_pnt) > Precision::Confusion() &&
                        fabs(dist_p_eps2_pnt) > Precision::Confusion()) {
                        if (dist_p_eps1_pnt * dist_p_eps2_pnt < 0)
                            return true;
                    }
                }
            }
        }
    }

    return false;
}

void ProfileBased::remapSupportShape(const TopoDS_Shape & newShape)
{
    (void)newShape;
    // Realthunder: with the new topological naming, I don't think this function
    // is necessary. A missing element will cause an explicitly error, and the
    // user will be force to manually select the element. Various editors, such
    // as dress up editors, can perform element guessing when activated.
}

namespace PartDesign {
    struct gp_Pnt_Less
    {
        bool operator()(const gp_Pnt& p1,
            const gp_Pnt& p2) const
        {
            if (fabs(p1.X() - p2.X()) > Precision::Confusion())
                return p1.X() < p2.X();
            if (fabs(p1.Y() - p2.Y()) > Precision::Confusion())
                return p1.Y() < p2.Y();
            if (fabs(p1.Z() - p2.Z()) > Precision::Confusion())
                return p1.Z() < p2.Z();
            return false; // points are considered to be equal
        }
    };
}

bool ProfileBased::isQuasiEqual(const TopoDS_Shape & s1, const TopoDS_Shape & s2) const
{
    if (s1.ShapeType() != s2.ShapeType())
        return false;
    TopTools_IndexedMapOfShape map1, map2;
    TopExp::MapShapes(s1, TopAbs_VERTEX, map1);
    TopExp::MapShapes(s2, TopAbs_VERTEX, map2);
    if (map1.Extent() != map2.Extent())
        return false;

    std::vector<gp_Pnt> p1;
    for (int i = 1; i <= map1.Extent(); i++) {
        const TopoDS_Vertex& v = TopoDS::Vertex(map1.FindKey(i));
        p1.push_back(BRep_Tool::Pnt(v));
    }
    std::vector<gp_Pnt> p2;
    for (int i = 1; i <= map2.Extent(); i++) {
        const TopoDS_Vertex& v = TopoDS::Vertex(map2.FindKey(i));
        p2.push_back(BRep_Tool::Pnt(v));
    }

    std::sort(p1.begin(), p1.end(), gp_Pnt_Less());
    std::sort(p2.begin(), p2.end(), gp_Pnt_Less());

    if (p1.size() != p2.size())
        return false;

    std::vector<gp_Pnt>::iterator it = p1.begin(), jt = p2.begin();
    for (; it != p1.end(); ++it, ++jt) {
        if (!(*it).IsEqual(*jt, Precision::Confusion()))
            return false;
    }

    return true;
}

bool ProfileBased::isEqualGeometry(const TopoDS_Shape & s1, const TopoDS_Shape & s2) const
{
    if (s1.ShapeType() == TopAbs_FACE && s2.ShapeType() == TopAbs_FACE) {
        BRepAdaptor_Surface a1(TopoDS::Face(s1));
        BRepAdaptor_Surface a2(TopoDS::Face(s2));
        if (a1.GetType() == GeomAbs_Plane && a2.GetType() == GeomAbs_Plane) {
            gp_Pln p1 = a1.Plane();
            gp_Pln p2 = a2.Plane();
            if (p1.Distance(p2.Location()) < Precision::Confusion()) {
                const gp_Dir& d1 = p1.Axis().Direction();
                const gp_Dir& d2 = p2.Axis().Direction();
                if (d1.IsParallel(d2, Precision::Confusion()))
                    return true;
            }
        }
    }
    else if (s1.ShapeType() == TopAbs_EDGE && s2.ShapeType() == TopAbs_EDGE) {
        // Do nothing here
    }
    else if (s1.ShapeType() == TopAbs_VERTEX && s2.ShapeType() == TopAbs_VERTEX) {
        gp_Pnt p1 = BRep_Tool::Pnt(TopoDS::Vertex(s1));
        gp_Pnt p2 = BRep_Tool::Pnt(TopoDS::Vertex(s2));
        return p1.Distance(p2) < Precision::Confusion();
    }

    return false;
}

bool ProfileBased::isParallelPlane(const TopoDS_Shape & s1, const TopoDS_Shape & s2) const
{
    if (s1.ShapeType() == TopAbs_FACE && s2.ShapeType() == TopAbs_FACE) {
        BRepAdaptor_Surface a1(TopoDS::Face(s1));
        BRepAdaptor_Surface a2(TopoDS::Face(s2));
        if (a1.GetType() == GeomAbs_Plane && a2.GetType() == GeomAbs_Plane) {
            gp_Pln p1 = a1.Plane();
            gp_Pln p2 = a2.Plane();
            const gp_Dir& d1 = p1.Axis().Direction();
            const gp_Dir& d2 = p2.Axis().Direction();
            if (d1.IsParallel(d2, Precision::Confusion()))
                return true;
        }
    }

    return false;
}

double ProfileBased::getReversedAngle(const Base::Vector3d & b, const Base::Vector3d & v) const
{
    try {
        Part::Feature* obj = getVerifiedObject();
        TopoShape sketchshape = getTopoShapeVerifiedFace();

        // get centre of gravity of the sketch face
        GProp_GProps props;
        BRepGProp::SurfaceProperties(sketchshape.getShape(), props);
        gp_Pnt cog = props.CentreOfMass();
        Base::Vector3d p_cog(cog.X(), cog.Y(), cog.Z());
        // get direction to cog from its projection on the revolve axis
        Base::Vector3d perp_dir = p_cog - p_cog.Perpendicular(b, v);
        // get cross product of projection direction with revolve axis direction
        Base::Vector3d cross = v % perp_dir;
        // get sketch vector pointing away from support material
        Base::Placement SketchPos = obj->Placement.getValue();
        Base::Rotation SketchOrientation = SketchPos.getRotation();
        Base::Vector3d SketchNormal(0, 0, 1);
        SketchOrientation.multVec(SketchNormal, SketchNormal);

        return SketchNormal * cross;
    }
    catch (...) {
        return Reversed.getValue() ? 1 : 0;
    }
}

void ProfileBased::getAxis(const App::DocumentObject * pcReferenceAxis, const std::vector<std::string> &subReferenceAxis,
                           Base::Vector3d& base, Base::Vector3d& dir, ProfileBased::ForbiddenAxis checkAxis) const
{
    auto verifyAxisFunc = [](ProfileBased::ForbiddenAxis checkAxis, const gp_Pln& sketchplane, const gp_Dir& dir) {
        switch (checkAxis) {
        case ForbiddenAxis::NotPerpendicularWithNormal:
            // If perpendicular to the normal then it's parallel to the plane
            if (sketchplane.Axis().Direction().IsNormal(dir, Precision::Angular()))
                throw Base::ValueError("Axis must not be parallel to the sketch plane");
            break;
        case ForbiddenAxis::NotParallelWithNormal:
            // If parallel with the normal then it's perpendicular to the plane
            if (sketchplane.Axis().Direction().IsParallel(dir, Precision::Angular()))
                throw Base::ValueError("Axis must not be perpendicular to the sketch plane");
            break;
        default:
            break;
        }
    };

    auto getAxisFromEdge = [](const TopoDS_Edge& refEdge, Base::Vector3d& base, Base::Vector3d& dir) {
        if (refEdge.IsNull())
            throw Base::ValueError("Failed to extract rotation edge");
        BRepAdaptor_Curve adapt(refEdge);
        gp_Pnt b;
        gp_Dir d;
        if (adapt.GetType() == GeomAbs_Line) {
            b = adapt.Line().Location();
            d = adapt.Line().Direction();
        }
        else if (adapt.GetType() == GeomAbs_Circle) {
            b = adapt.Circle().Location();
            d = adapt.Circle().Axis().Direction();
        }
        else {
            throw Base::TypeError("Edge must be a straight line, circle or arc of circle");
        }

        base = Base::Vector3d(b.X(), b.Y(), b.Z());
        dir = Base::Vector3d(d.X(), d.Y(), d.Z());
    };

    dir = Base::Vector3d(0, 0, 0); // If unchanged signals that no valid axis was found
    if (!pcReferenceAxis || subReferenceAxis.empty()) {
        return;
    }

    App::DocumentObject* profile = Profile.getValue();
    gp_Pln sketchplane;

    if (profile->isDerivedFrom<Part::Part2DObject>()) {
        Part::Part2DObject* sketch = getVerifiedSketch();
        Base::Placement SketchPlm = sketch->Placement.getValue();
        Base::Vector3d SketchVector = Base::Vector3d(0, 0, 1);
        Base::Rotation SketchOrientation = SketchPlm.getRotation();
        SketchOrientation.multVec(SketchVector, SketchVector);
        Base::Vector3d SketchPos = SketchPlm.getPosition();
        sketchplane = gp_Pln(gp_Pnt(SketchPos.x, SketchPos.y, SketchPos.z), gp_Dir(SketchVector.x, SketchVector.y, SketchVector.z));

        if (pcReferenceAxis == profile) {
            bool hasValidAxis = false;
            Base::Axis axis;
            if (subReferenceAxis[0] == "V_Axis") {
                hasValidAxis = true;
                axis = sketch->getAxis(Part::Part2DObject::V_Axis);
            }
            else if (subReferenceAxis[0] == "H_Axis") {
                hasValidAxis = true;
                axis = sketch->getAxis(Part::Part2DObject::H_Axis);
            }
            else if (subReferenceAxis[0] == "N_Axis") {
                hasValidAxis = true;
                axis = sketch->getAxis(Part::Part2DObject::N_Axis);
            }
            else if (subReferenceAxis[0].compare(0, 4, "Axis") == 0) {
                int AxId = std::atoi(subReferenceAxis[0].substr(4, 4000).c_str());
                if (AxId >= 0 && AxId < sketch->getAxisCount()) {
                    hasValidAxis = true;
                    axis = sketch->getAxis(AxId);
                }
            }
            if (hasValidAxis) {
                axis *= SketchPlm;
                base = axis.getBase();
                dir = axis.getDirection();
                return;
            } //else - an edge of the sketch was selected as an axis
        }

    }
    else if (profile->isDerivedFrom<Part::Feature>()) {
        Base::Placement SketchPlm = getVerifiedObject()->Placement.getValue();
        Base::Vector3d SketchVector = getProfileNormal();
        Base::Vector3d SketchPos = SketchPlm.getPosition();
        sketchplane = gp_Pln(gp_Pnt(SketchPos.x, SketchPos.y, SketchPos.z), gp_Dir(SketchVector.x, SketchVector.y, SketchVector.z));
    }

    // get reference axis
    if (pcReferenceAxis->isDerivedFrom<PartDesign::Line>()) {
        const PartDesign::Line* line = static_cast<const PartDesign::Line*>(pcReferenceAxis);
        base = line->getBasePoint();
        dir = line->getDirection();

        verifyAxisFunc(checkAxis, sketchplane, gp_Dir(dir.x, dir.y, dir.z));
        return;
    }

    if (pcReferenceAxis->isDerivedFrom<App::Line>()) {
        auto* line = static_cast<const App::Line*>(pcReferenceAxis);
        base = line->getBasePoint();
        dir = line->getDirection();

        verifyAxisFunc(checkAxis, sketchplane, gp_Dir(dir.x, dir.y, dir.z));
        return;
    }

    if (pcReferenceAxis->isDerivedFrom<Part::Feature>()) {
        if (subReferenceAxis.empty())
            throw Base::ValueError("No rotation axis reference specified");
        const Part::Feature* refFeature = static_cast<const Part::Feature*>(pcReferenceAxis);
        Part::TopoShape refShape = refFeature->Shape.getShape();
        TopoDS_Shape ref;
        try {
            // if an exception is raised then convert it into a FreeCAD-specific exception
            ref = refShape.getSubShape(subReferenceAxis[0].c_str());
        }
        catch (const Standard_Failure& e) {
            throw Base::RuntimeError(e.GetMessageString());
        }

        if (ref.ShapeType() == TopAbs_EDGE) {
            getAxisFromEdge(TopoDS::Edge(ref), base, dir);
            verifyAxisFunc(checkAxis, sketchplane, gp_Dir(dir.x, dir.y, dir.z));
            return;
        }
    }

    throw Base::TypeError("Unsupported geometry type to get reference axis");
}

Base::Vector3d ProfileBased::getProfileNormal() const {

    Base::Vector3d SketchVector(0, 0, 1);
    auto obj = getVerifiedObject(true);
    if (!obj) {
        return SketchVector;
    }

    // get the Sketch plane
    if (obj->isDerivedFrom<Part::Part2DObject>()) {
        Base::Placement SketchPos = obj->Placement.getValue();
        Base::Rotation SketchOrientation = SketchPos.getRotation();
        SketchOrientation.multVec(SketchVector, SketchVector);
        return SketchVector;
    }

    // For newer version, do not do fitting, as it may flip the face normal for
    // some reason.
    TopoShape shape = getTopoShapeVerifiedFace(true, true, true);  //, _ProfileBasedVersion.getValue() <= 0);

    gp_Pln pln;
    if (shape.findPlane(pln)) {
        gp_Dir dir = pln.Axis().Direction();
        return Base::Vector3d(dir.X(), dir.Y(), dir.Z());
    }

    if (shape.hasSubShape(TopAbs_EDGE)) {
        // Find the first planar face that contains the edge, and return the plane normal
        TopoShape objShape = Part::Feature::getTopoShape(obj, Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform);
        for (int idx : objShape.findAncestors(shape.getSubShape(TopAbs_EDGE, 1), TopAbs_FACE)) {
            if (objShape.getSubTopoShape(TopAbs_FACE, idx).findPlane(pln)) {
                gp_Dir dir = pln.Axis().Direction();
                return Base::Vector3d(dir.X(), dir.Y(), dir.Z());
            }
        }
    }

    // If no planar face, try to use the normal of the center of the first face.
    if (shape.hasSubShape(TopAbs_FACE)) {
        TopoDS_Face face = TopoDS::Face(shape.getSubShape(TopAbs_FACE, 1));
        BRepAdaptor_Surface adapt(face);
        double u =
            adapt.FirstUParameter() + (adapt.LastUParameter() - adapt.FirstUParameter()) / 2.;
        double v =
            adapt.FirstVParameter() + (adapt.LastVParameter() - adapt.FirstVParameter()) / 2.;
        BRepLProp_SLProps prop(adapt, u, v, 2, Precision::Confusion());
        if (prop.IsNormalDefined()) {
            gp_Pnt pnt;
            gp_Vec vec;
            // handles the orientation state of the shape
            BRepGProp_Face(face).Normal(u, v, pnt, vec);
            return Base::Vector3d(vec.X(), vec.Y(), vec.Z());
        }
    }

    if (!shape.hasSubShape(TopAbs_EDGE)) {
        return SketchVector;
    }

    // If the shape is a line, then return an arbitrary direction that is perpendicular to the line
    auto geom = Part::Geometry::fromShape(shape.getSubShape(TopAbs_EDGE, 1), true);
    auto geomLine = freecad_cast<Part::GeomLine*>(geom.get());
    if (geomLine) {
        Base::Vector3d dir = geomLine->getDir();
        double x = std::fabs(dir.x);
        double y = std::fabs(dir.y);
        double z = std::fabs(dir.z);
        if (x > y && x > z && x > 1e-7) {
            if (y + z < 1e-7) {
                return Base::Vector3d(0, 0, 1);
            }
            dir.x = -(dir.z + dir.y) / dir.x;
        }
        else if (y > x && y > z && y > 1e-7) {
            if (x + z < 1e-7) {
                return Base::Vector3d(0, 0, 1);
            }
            dir.y = -(dir.z + dir.x) / dir.y;
        }
        else if (z > 1e-7) {
            if (x + y < 1e-7) {
                return Base::Vector3d(1, 0, 0);
            }
            dir.z = -(dir.x + dir.y) / dir.z;
        }
        else {
            return SketchVector;
        }
        return dir.Normalize();
    }
    return SketchVector;
}

void ProfileBased::Restore(Base::XMLReader & reader)
{
    PartDesign::FeatureAddSub::Restore(reader);
}

void ProfileBased::handleChangedPropertyName(Base::XMLReader & reader, const char* TypeName, const char* PropName)
{
    //check if we load the old sketch property
    if ((strcmp("Sketch", PropName) == 0) && (strcmp("App::PropertyLink", TypeName) == 0)) {

        std::vector<std::string> vec;
        // read my element
        reader.readElement("Link");
        // get the value of my attribute
        std::string name = reader.getAttribute<const char*>("value");

        if (!name.empty()) {
            App::Document* document = getDocument();
            DocumentObject* object = document ? document->getObject(name.c_str()) : nullptr;
            Profile.setValue(object, vec);
        }
        else {
            Profile.setValue(nullptr, vec);
        }
    }
    else {
        PartDesign::FeatureAddSub::handleChangedPropertyName(reader, TypeName, PropName);
    }
}
