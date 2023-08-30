/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <BRepAdaptor_Curve.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <BRepBuilderAPI_Copy.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <BRepLib_FindSurface.hxx>
# include <BRepPrimAPI_MakePrism.hxx>
# include <gp_Pln.hxx>
# include <gp_Trsf.hxx>
# include <Precision.hxx>
# include <TopExp.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
#endif

#include <Base/Exception.h>

#include "FeatureExtrusion.h"
#include "ExtrusionHelper.h"
#include "Part2DObject.h"


using namespace Part;

PROPERTY_SOURCE(Part::Extrusion, Part::Feature)

const char* Extrusion::eDirModeStrings[] = {
    "Custom",
    "Edge",
    "Normal",
    nullptr };

Extrusion::Extrusion()
{
    ADD_PROPERTY_TYPE(Base, (nullptr), "Extrude", App::Prop_None, "Shape to extrude");
    ADD_PROPERTY_TYPE(Dir, (Base::Vector3d(0.0, 0.0, 1.0)), "Extrude", App::Prop_None, "Direction of extrusion (also magnitude, if both lengths are zero).");
    ADD_PROPERTY_TYPE(DirMode, (dmCustom), "Extrude", App::Prop_None, "Sets, how Dir is updated.");
    DirMode.setEnums(eDirModeStrings);
    ADD_PROPERTY_TYPE(DirLink, (nullptr), "Extrude", App::Prop_None, "Link to edge defining extrusion direction.");
    ADD_PROPERTY_TYPE(LengthFwd, (0.0), "Extrude", App::Prop_None, "Length of extrusion along direction. If both LengthFwd and LengthRev are zero, magnitude of Dir is used.");
    ADD_PROPERTY_TYPE(LengthRev, (0.0), "Extrude", App::Prop_None, "Length of additional extrusion, against direction.");
    ADD_PROPERTY_TYPE(Solid, (false), "Extrude", App::Prop_None, "If true, extruding a wire yields a solid. If false, a shell.");
    ADD_PROPERTY_TYPE(Reversed, (false), "Extrude", App::Prop_None, "Set to true to swap the direction of extrusion.");
    ADD_PROPERTY_TYPE(Symmetric, (false), "Extrude", App::Prop_None, "If true, extrusion is done in both directions to a total of LengthFwd. LengthRev is ignored.");
    ADD_PROPERTY_TYPE(TaperAngle, (0.0), "Extrude", App::Prop_None, "Sets the angle of slope (draft) to apply to the sides. The angle is for outward taper; negative value yields inward tapering.");
    ADD_PROPERTY_TYPE(TaperAngleRev, (0.0), "Extrude", App::Prop_None, "Taper angle of reverse part of extrusion.");
    ADD_PROPERTY_TYPE(FaceMakerClass, ("Part::FaceMakerExtrusion"), "Extrude", App::Prop_None, "If Solid is true, this sets the facemaker class to use when converting wires to faces. Otherwise, ignored."); //default for old documents. See setupObject for default for new extrusions.
}

short Extrusion::mustExecute() const
{
    if (Base.isTouched() ||
        Dir.isTouched() ||
        DirMode.isTouched() ||
        DirLink.isTouched() ||
        LengthFwd.isTouched() ||
        LengthRev.isTouched() ||
        Solid.isTouched() ||
        Reversed.isTouched() ||
        Symmetric.isTouched() ||
        TaperAngle.isTouched() ||
        TaperAngleRev.isTouched() ||
        FaceMakerClass.isTouched())
        return 1;
    return 0;
}

bool Extrusion::fetchAxisLink(const App::PropertyLinkSub& axisLink, Base::Vector3d& basepoint, Base::Vector3d& dir)
{
    if (!axisLink.getValue())
        return false;

    auto linked = axisLink.getValue();

    TopoDS_Shape axEdge;
    if (!axisLink.getSubValues().empty() && axisLink.getSubValues()[0].length() > 0) {
        axEdge = Feature::getTopoShape(linked, axisLink.getSubValues()[0].c_str(), true /*need element*/).getShape();
    }
    else {
        axEdge = Feature::getShape(linked);
    }

    if (axEdge.IsNull())
        throw Base::ValueError("DirLink shape is null");
    if (axEdge.ShapeType() != TopAbs_EDGE)
        throw Base::TypeError("DirLink shape is not an edge");

    BRepAdaptor_Curve crv(TopoDS::Edge(axEdge));
    gp_Pnt startpoint;
    gp_Pnt endpoint;
    if (crv.GetType() == GeomAbs_Line) {
        startpoint = crv.Value(crv.FirstParameter());
        endpoint = crv.Value(crv.LastParameter());
        if (axEdge.Orientation() == TopAbs_REVERSED)
            std::swap(startpoint, endpoint);
    }
    else {
        throw Base::TypeError("DirLink edge is not a line.");
    }
    basepoint.Set(startpoint.X(), startpoint.Y(), startpoint.Z());
    gp_Vec vec = gp_Vec(startpoint, endpoint);
    dir.Set(vec.X(), vec.Y(), vec.Z());
    return true;
}

Extrusion::ExtrusionParameters Extrusion::computeFinalParameters()
{
    Extrusion::ExtrusionParameters result;
    Base::Vector3d dir;
    switch (this->DirMode.getValue()) {
    case dmCustom:
        dir = this->Dir.getValue();
        break;
    case dmEdge: {
        bool fetched;
        Base::Vector3d base;
        fetched = fetchAxisLink(this->DirLink, base, dir);
        if (!fetched)
            throw Base::ValueError("DirMode is set to use edge, but no edge is linked.");
        this->Dir.setValue(dir);
    } break;
    case dmNormal:
        dir = calculateShapeNormal(this->Base);
        this->Dir.setValue(dir);
        break;
    default:
        throw Base::ValueError("Unexpected enum value");
    }
    if (dir.Length() < Precision::Confusion())
        throw Base::ValueError("Direction is zero-length");
    result.dir = gp_Dir(dir.x, dir.y, dir.z);
    if (this->Reversed.getValue())
        result.dir.Reverse();

    result.lengthFwd = this->LengthFwd.getValue();
    result.lengthRev = this->LengthRev.getValue();
    if (fabs(result.lengthFwd) < Precision::Confusion()
        && fabs(result.lengthRev) < Precision::Confusion()) {
        result.lengthFwd = dir.Length();
    }

    if (this->Symmetric.getValue()) {
        result.lengthRev = result.lengthFwd * 0.5;
        result.lengthFwd = result.lengthFwd * 0.5;
    }

    if (fabs(result.lengthFwd + result.lengthRev) < Precision::Confusion())
        throw Base::ValueError("Total length of extrusion is zero.");

    result.solid = this->Solid.getValue();

    result.taperAngleFwd = this->TaperAngle.getValue() * M_PI / 180.0;
    if (fabs(result.taperAngleFwd) > M_PI * 0.5 - Precision::Angular())
        throw Base::ValueError("Magnitude of taper angle matches or exceeds 90 degrees. That is too much.");
    result.taperAngleRev = this->TaperAngleRev.getValue() * M_PI / 180.0;
    if (fabs(result.taperAngleRev) > M_PI * 0.5 - Precision::Angular())
        throw Base::ValueError("Magnitude of taper angle matches or exceeds 90 degrees. That is too much.");

    result.faceMakerClass = this->FaceMakerClass.getValue();

    return result;
}

Base::Vector3d Extrusion::calculateShapeNormal(const App::PropertyLink& shapeLink)
{
    App::DocumentObject* docobj = nullptr;
    Base::Matrix4D mat;
    TopoDS_Shape sh = Feature::getShape(shapeLink.getValue(), nullptr, false, &mat, &docobj);

    if (!docobj)
        throw Base::ValueError("calculateShapeNormal: link is empty");

    //special case for sketches and the like: no matter what shape they have, use their local Z axis.
    if (docobj->isDerivedFrom(Part::Part2DObject::getClassTypeId())) {
        Base::Vector3d OZ(0.0, 0.0, 1.0);
        Base::Vector3d result;
        Base::Rotation(mat).multVec(OZ, result);
        return result;
    }

    if (sh.IsNull())
        throw NullShapeException("calculateShapeNormal: link points to a valid object, but its shape is null.");

    //find plane
    BRepLib_FindSurface planeFinder(sh, -1, /*OnlyPlane=*/true);
    if (!planeFinder.Found())
        throw Base::ValueError("Can't find normal direction, because the shape is not on a plane.");

    //find plane normal and return result.
    GeomAdaptor_Surface surf(planeFinder.Surface());
    gp_Dir normal = surf.Plane().Axis().Direction();

    //now we know the plane. But if there are faces, the
    //plane normal direction is not dependent on face orientation (because findPlane only uses edges).
    //let's fix that.
    TopExp_Explorer ex(sh, TopAbs_FACE);
    if (ex.More()) {
        BRepAdaptor_Surface surf(TopoDS::Face(ex.Current()));
        normal = surf.Plane().Axis().Direction();
        if (ex.Current().Orientation() == TopAbs_REVERSED) {
            normal.Reverse();
        }
    }

    return Base::Vector3d(normal.X(), normal.Y(), normal.Z());
}

TopoShape Extrusion::extrudeShape(const TopoShape& source, const Extrusion::ExtrusionParameters& params)
{
    TopoDS_Shape result;
    gp_Vec vec = gp_Vec(params.dir).Multiplied(params.lengthFwd + params.lengthRev);//total vector of extrusion

    if (std::fabs(params.taperAngleFwd) >= Precision::Angular() ||
        std::fabs(params.taperAngleRev) >= Precision::Angular()) {
        //Tapered extrusion!
#if defined(__GNUC__) && defined (FC_OS_LINUX)
        Base::SignalException se;
#endif
        TopoDS_Shape myShape = source.getShape();
        if (myShape.IsNull())
            Standard_Failure::Raise("Cannot extrude empty shape");
        // #0000910: Circles Extrude Only Surfaces, thus use BRepBuilderAPI_Copy
        myShape = BRepBuilderAPI_Copy(myShape).Shape();

        std::list<TopoDS_Shape> drafts;
        bool isPartDesign = false; // there is an OCC bug with single-edge wires (circles) we need to treat differently for PD and Part
        ExtrusionHelper::makeDraft(myShape, params.dir, params.lengthFwd, params.lengthRev,
                                   params.taperAngleFwd, params.taperAngleRev, params.solid, drafts, isPartDesign);
        if (drafts.empty()) {
            Standard_Failure::Raise("Drafting shape failed");
        }
        else if (drafts.size() == 1) {
            result = drafts.front();
        }
        else {
            TopoDS_Compound comp;
            BRep_Builder builder;
            builder.MakeCompound(comp);
            for (const auto & draft : drafts)
                builder.Add(comp, draft);
            result = comp;
        }
    }
    else {
        //Regular (non-tapered) extrusion!
        TopoDS_Shape myShape = source.getShape();
        if (myShape.IsNull())
            Standard_Failure::Raise("Cannot extrude empty shape");

        // #0000910: Circles Extrude Only Surfaces, thus use BRepBuilderAPI_Copy
        myShape = BRepBuilderAPI_Copy(myShape).Shape();

        //apply reverse part of extrusion by shifting the source shape
        if (fabs(params.lengthRev) > Precision::Confusion()) {
            gp_Trsf mov;
            mov.SetTranslation(gp_Vec(params.dir) * (-params.lengthRev));
            TopLoc_Location loc(mov);
            myShape.Move(loc);
        }

        //make faces from wires
        if (params.solid) {
            //test if we need to make faces from wires. If there are faces - we don't.
            TopExp_Explorer xp(myShape, TopAbs_FACE);
            if (xp.More()) {
                //source shape has faces. Just extrude as-is.
            }
            else {
                std::unique_ptr<FaceMaker> mkFace = FaceMaker::ConstructFromType(params.faceMakerClass.c_str());

                if (myShape.ShapeType() == TopAbs_COMPOUND)
                    mkFace->useCompound(TopoDS::Compound(myShape));
                else
                    mkFace->addShape(myShape);
                mkFace->Build();
                myShape = mkFace->Shape();
            }
        }

        //extrude!
        BRepPrimAPI_MakePrism mkPrism(myShape, vec);
        result = mkPrism.Shape();
    }

    if (result.IsNull())
        throw NullShapeException("Result of extrusion is null shape.");
    return TopoShape(result);
}

App::DocumentObjectExecReturn* Extrusion::execute()
{
    App::DocumentObject* link = Base.getValue();
    if (!link)
        return new App::DocumentObjectExecReturn("No object linked");

    try {
        Extrusion::ExtrusionParameters params = computeFinalParameters();
        TopoShape result = extrudeShape(Feature::getShape(link), params);
        this->Shape.setValue(result);
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
}

//----------------------------------------------------------------

TYPESYSTEM_SOURCE(Part::FaceMakerExtrusion, Part::FaceMakerCheese)

std::string FaceMakerExtrusion::getUserFriendlyName() const
{
    return {QT_TRANSLATE_NOOP("Part_FaceMaker", "Part Extrude facemaker")};
}

std::string FaceMakerExtrusion::getBriefExplanation() const
{
    return {QT_TRANSLATE_NOOP("Part_FaceMaker", "Supports making faces with holes, does not support nesting.")};
}

#if OCC_VERSION_HEX >= 0x070600
void FaceMakerExtrusion::Build(const Message_ProgressRange&)
#else
void FaceMakerExtrusion::Build()
#endif
{
    this->NotDone();
    this->myGenerated.Clear();
    this->myShapesToReturn.clear();
    this->myShape = TopoDS_Shape();
    TopoDS_Shape inputShape;
    if (mySourceShapes.empty())
        throw Base::ValueError("No input shapes!");
    if (mySourceShapes.size() == 1) {
        inputShape = mySourceShapes[0];
    }
    else {
        TopoDS_Builder builder;
        TopoDS_Compound cmp;
        builder.MakeCompound(cmp);
        for (const TopoDS_Shape& sh : mySourceShapes) {
            builder.Add(cmp, sh);
        }
        inputShape = cmp;
    }

    std::vector<TopoDS_Wire> wires;
    TopTools_IndexedMapOfShape mapOfWires;
    TopExp::MapShapes(inputShape, TopAbs_WIRE, mapOfWires);

    // if there are no wires then check also for edges
    if (mapOfWires.IsEmpty()) {
        TopTools_IndexedMapOfShape mapOfEdges;
        TopExp::MapShapes(inputShape, TopAbs_EDGE, mapOfEdges);
        for (int i = 1; i <= mapOfEdges.Extent(); i++) {
            BRepBuilderAPI_MakeWire mkWire(TopoDS::Edge(mapOfEdges.FindKey(i)));
            wires.push_back(mkWire.Wire());
        }
    }
    else {
        wires.reserve(mapOfWires.Extent());
        for (int i = 1; i <= mapOfWires.Extent(); i++) {
            wires.push_back(TopoDS::Wire(mapOfWires.FindKey(i)));
        }
    }

    if (!wires.empty()) {
        //try {
        TopoDS_Shape res = FaceMakerCheese::makeFace(wires);
        if (!res.IsNull())
            this->myShape = res;
        //}
        //catch (...) {

        //}
    }

    this->Done();

}

void Part::Extrusion::setupObject()
{
    Part::Feature::setupObject();
    this->FaceMakerClass.setValue("Part::FaceMakerBullseye"); //default for newly created features
}
