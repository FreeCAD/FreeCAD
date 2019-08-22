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
# include <cmath>
# include <gp_Pln.hxx>
# include <gp_Trsf.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <BRepOffsetAPI_MakeOffset.hxx>
# include <BRepBuilderAPI_Copy.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <BRepOffsetAPI_ThruSections.hxx>
# include <BRepPrimAPI_MakePrism.hxx>
# include <Precision.hxx>
# include <ShapeAnalysis.hxx>
# include <ShapeFix_Wire.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Iterator.hxx>
# include <TopExp.hxx>
# include <TopExp_Explorer.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <BRepLib_FindSurface.hxx>
#endif

#include "FeatureExtrusion.h"
#include <Base/Tools.h>
#include <Base/Exception.h>
#include "Part2DObject.h"



using namespace Part;


PROPERTY_SOURCE(Part::Extrusion, Part::Feature)

const char* Extrusion::eDirModeStrings[]= {
    "Custom",
    "Edge",
    "Normal",
    NULL};

Extrusion::Extrusion()
{
    ADD_PROPERTY_TYPE(Base,(0), "Extrude", App::Prop_None, "Shape to extrude");
    ADD_PROPERTY_TYPE(Dir,(Base::Vector3d(0.0,0.0,1.0)), "Extrude", App::Prop_None, "Direction of extrusion (also magnitude, if both lengths are zero).");
    ADD_PROPERTY_TYPE(DirMode, (dmCustom), "Extrude", App::Prop_None, "Sets, how Dir is updated.");
    DirMode.setEnums(eDirModeStrings);
    ADD_PROPERTY_TYPE(DirLink,(nullptr), "Extrude", App::Prop_None, "Link to edge defining extrusion direction.");
    ADD_PROPERTY_TYPE(LengthFwd,(0.0), "Extrude", App::Prop_None, "Length of extrusion along direction. If both LengthFwd and LengthRev are zero, magnitude of Dir is used.");
    ADD_PROPERTY_TYPE(LengthRev,(0.0), "Extrude", App::Prop_None, "Length of additional extrusion, against direction.");
    ADD_PROPERTY_TYPE(Solid,(false), "Extrude", App::Prop_None, "If true, extruding a wire yields a solid. If false, a shell.");
    ADD_PROPERTY_TYPE(Reversed,(false), "Extrude", App::Prop_None, "Set to true to swap the direction of extrusion.");
    ADD_PROPERTY_TYPE(Symmetric,(false), "Extrude", App::Prop_None, "If true, extrusion is done in both directions to a total of LengthFwd. LengthRev is ignored.");
    ADD_PROPERTY_TYPE(TaperAngle,(0.0), "Extrude", App::Prop_None, "Sets the angle of slope (draft) to apply to the sides. The angle is for outward taper; negative value yields inward tapering.");
    ADD_PROPERTY_TYPE(TaperAngleRev,(0.0), "Extrude", App::Prop_None, "Taper angle of reverse part of extrusion.");
    ADD_PROPERTY_TYPE(FaceMakerClass,("Part::FaceMakerExtrusion"), "Extrude", App::Prop_None, "If Solid is true, this sets the facemaker class to use when converting wires to faces. Otherwise, ignored."); //default for old documents. See setupObject for default for new extrusions.
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

    if (!axisLink.getValue()->isDerivedFrom(Part::Feature::getClassTypeId()))
        throw Base::TypeError("AxisLink has no OCC shape");

    Part::Feature* linked = static_cast<Part::Feature*>(axisLink.getValue());

    TopoDS_Shape axEdge;
    if (axisLink.getSubValues().size() > 0  &&  axisLink.getSubValues()[0].length() > 0){
        axEdge = linked->Shape.getShape().getSubShape(axisLink.getSubValues()[0].c_str());
    } else {
        axEdge = linked->Shape.getValue();
    }

    if (axEdge.IsNull())
        throw Base::ValueError("DirLink shape is null");
    if (axEdge.ShapeType() != TopAbs_EDGE)
        throw Base::TypeError("DirLink shape is not an edge");

    BRepAdaptor_Curve crv(TopoDS::Edge(axEdge));
    gp_Pnt startpoint;
    gp_Pnt endpoint;
    if (crv.GetType() == GeomAbs_Line){
        startpoint = crv.Value(crv.FirstParameter());
        endpoint = crv.Value(crv.LastParameter());
        if (axEdge.Orientation() == TopAbs_REVERSED)
            std::swap(startpoint, endpoint);
    } else {
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
    switch(this->DirMode.getValue()){
        case dmCustom:
            dir = this->Dir.getValue();
        break;
        case dmEdge:{
            bool fetched;
            Base::Vector3d base;
            fetched = fetchAxisLink(this->DirLink, base, dir);
            if (!fetched)
                throw Base::ValueError("DirMode is set to use edge, but no edge is linked.");
            this->Dir.setValue(dir);
        }break;
        case dmNormal:
            dir = calculateShapeNormal(this->Base);
            this->Dir.setValue(dir);
        break;
        default:
            throw Base::ValueError("Unexpected enum value");
    }
    if(dir.Length() < Precision::Confusion())
        throw Base::ValueError("Direction is zero-length");
    result.dir = gp_Dir(dir.x, dir.y, dir.z);
    if (this->Reversed.getValue())
        result.dir.Reverse();

    result.lengthFwd = this->LengthFwd.getValue();
    result.lengthRev = this->LengthRev.getValue();
    if(fabs(result.lengthFwd) < Precision::Confusion()
            && fabs(result.lengthRev) < Precision::Confusion() ){
        result.lengthFwd = dir.Length();
    }

    if (this->Symmetric.getValue()){
        result.lengthRev = result.lengthFwd * 0.5;
        result.lengthFwd = result.lengthFwd * 0.5;
    }

    if (fabs(result.lengthFwd + result.lengthRev) < Precision::Confusion())
        throw Base::ValueError("Total length of extrusion is zero.");

    result.solid = this->Solid.getValue();

    result.taperAngleFwd = this->TaperAngle.getValue() * M_PI / 180.0;
    if (fabs(result.taperAngleFwd) > M_PI * 0.5 - Precision::Angular() )
        throw Base::ValueError("Magnitude of taper angle matches or exceeds 90 degrees. That is too much.");
    result.taperAngleRev = this->TaperAngleRev.getValue() * M_PI / 180.0;
    if (fabs(result.taperAngleRev) > M_PI * 0.5 - Precision::Angular() )
        throw Base::ValueError("Magnitude of taper angle matches or exceeds 90 degrees. That is too much.");

    result.faceMakerClass = this->FaceMakerClass.getValue();

    return result;
}

Base::Vector3d Extrusion::calculateShapeNormal(const App::PropertyLink& shapeLink)
{
    if (!shapeLink.getValue())
        throw Base::ValueError("calculateShapeNormal: link is empty");
    const App::DocumentObject* docobj = shapeLink.getValue();

    //special case for sketches and the like: no matter what shape they have, use their local Z axis.
    if (docobj->isDerivedFrom(Part::Part2DObject::getClassTypeId())){
        const Part::Part2DObject* p2do = static_cast<const Part::Part2DObject*>(docobj);
        Base::Vector3d OZ (0.0, 0.0, 1.0);
        Base::Vector3d result;
        p2do->Placement.getValue().getRotation().multVec(OZ, result);
        return result;
    }

    //extract the shape
    if (! docobj->isDerivedFrom(Part::Feature::getClassTypeId()))
        throw Base::TypeError("Linked object doesn't have shape.");

    const TopoShape &tsh = static_cast<const Part::Feature*>(docobj)->Shape.getShape();
    TopoDS_Shape sh = tsh.getShape();
    if (sh.IsNull())
        throw NullShapeException("calculateShapeNormal: link points to a valid object, but its shape is null.");

    //find plane
    BRepLib_FindSurface planeFinder(sh, -1, /*OnlyPlane=*/true);
    if (! planeFinder.Found())
        throw Base::ValueError("Can't find normal direction, because the shape is not on a plane.");

    //find plane normal and return result.
    GeomAdaptor_Surface surf(planeFinder.Surface());
    gp_Dir normal = surf.Plane().Axis().Direction();

    //now se know the plane. But if there are faces, the
    //plane normal direction is not dependent on face orientation (because findPlane only uses edges).
    //let's fix that.
    TopExp_Explorer ex(sh, TopAbs_FACE);
    if(ex.More()) {
        BRepAdaptor_Surface surf(TopoDS::Face(ex.Current()));
        normal = surf.Plane().Axis().Direction();
        if (ex.Current().Orientation() == TopAbs_REVERSED){
            normal.Reverse();
        }
    }

    return Base::Vector3d(normal.X(), normal.Y(), normal.Z());
}

TopoShape Extrusion::extrudeShape(const TopoShape source, Extrusion::ExtrusionParameters params)
{
    TopoDS_Shape result;
    gp_Vec vec = gp_Vec(params.dir).Multiplied(params.lengthFwd+params.lengthRev);//total vector of extrusion

    if (std::fabs(params.taperAngleFwd) >= Precision::Angular() ||
        std::fabs(params.taperAngleRev) >= Precision::Angular() ) {
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
        makeDraft(params, myShape, drafts);
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
            for (std::list<TopoDS_Shape>::iterator it = drafts.begin(); it != drafts.end(); ++it)
                builder.Add(comp, *it);
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
        if (fabs(params.lengthRev)>Precision::Confusion() ){
            gp_Trsf mov;
            mov.SetTranslation(gp_Vec(params.dir)*(-params.lengthRev));
            TopLoc_Location loc(mov);
            myShape.Move(loc);
        }

        //make faces from wires
        if (params.solid) {
            //test if we need to make faces from wires. If there are faces - we don't.
            TopExp_Explorer xp(myShape, TopAbs_FACE);
            if (xp.More()){
                //source shape has faces. Just extrude as-is.
            } else {
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

App::DocumentObjectExecReturn *Extrusion::execute(void)
{
    App::DocumentObject* link = Base.getValue();
    if (!link)
        return new App::DocumentObjectExecReturn("No object linked");
    if (!link->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        return new App::DocumentObjectExecReturn("Linked object is not a Part object");
    Part::Feature *base = static_cast<Part::Feature*>(Base.getValue());

    try {
        Extrusion::ExtrusionParameters params = computeFinalParameters();
        TopoShape result = extrudeShape(base->Shape.getShape(),params);
        this->Shape.setValue(result);
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
}

void Extrusion::makeDraft(ExtrusionParameters params, const TopoDS_Shape& shape, std::list<TopoDS_Shape>& drafts)
{
    double distanceFwd = tan(params.taperAngleFwd)*params.lengthFwd;
    double distanceRev = tan(params.taperAngleRev)*params.lengthRev;

    gp_Vec vecFwd = gp_Vec(params.dir)*params.lengthFwd;
    gp_Vec vecRev = gp_Vec(params.dir.Reversed())*params.lengthRev;

    bool bFwd = fabs(params.lengthFwd) > Precision::Confusion();
    bool bRev = fabs(params.lengthRev) > Precision::Confusion();
    bool bMid = !bFwd || !bRev || params.lengthFwd*params.lengthRev > 0.0; //include the source shape as loft section?

    TopoDS_Wire sourceWire;
    if (shape.IsNull())
        Standard_Failure::Raise("Not a valid shape");
    if (shape.ShapeType() == TopAbs_WIRE) {
        ShapeFix_Wire aFix;
        aFix.Load(TopoDS::Wire(shape));
        aFix.FixReorder();
        aFix.FixConnected();
        aFix.FixClosed();
        sourceWire = aFix.Wire();
    }
    else if (shape.ShapeType() == TopAbs_FACE) {
        TopoDS_Wire outerWire = ShapeAnalysis::OuterWire(TopoDS::Face(shape));
        sourceWire = outerWire;
    }
    else if (shape.ShapeType() == TopAbs_COMPOUND) {
        TopoDS_Iterator it(shape);
        for (; it.More(); it.Next()) {
            makeDraft(params, it.Value(), drafts);
        }
    }
    else {
        Standard_Failure::Raise("Only a wire or a face is supported");
    }

    if (!sourceWire.IsNull()) {
        std::list<TopoDS_Wire> list_of_sections;

        // if the wire consists of a single edge which has applied a placement
        // then this placement must be reset because otherwise the
        // BRepOffsetAPI_MakeOffset shows weird behaviour by applying the placement
        // twice on the output shape
        //
        // count all edges of the wire
        int numEdges = 0;
        TopExp_Explorer xp(sourceWire, TopAbs_EDGE);
        while (xp.More()) {
            numEdges++;
            xp.Next();
        }

        auto makeOffset = [&numEdges,&sourceWire](const gp_Vec& translation, double offset) -> TopoDS_Shape {
            BRepOffsetAPI_MakeOffset mkOffset;
#if OCC_VERSION_HEX >= 0x060800
            mkOffset.Init(GeomAbs_Arc);
#endif
#if OCC_VERSION_HEX >= 0x070000
            mkOffset.Init(GeomAbs_Intersection);
#endif
            gp_Trsf mat;
            mat.SetTranslation(translation);
            TopLoc_Location loc(mat);
            TopoDS_Wire movedSourceWire = TopoDS::Wire(sourceWire.Moved(loc));

            TopoDS_Shape offsetShape;
            if (fabs(offset)>Precision::Confusion()) {
                TopLoc_Location wireLocation;
                TopLoc_Location edgeLocation;
                if (numEdges == 1) {
                    wireLocation = movedSourceWire.Location();

                    BRepBuilderAPI_MakeWire mkWire;
                    TopExp_Explorer xp(sourceWire, TopAbs_EDGE);
                    while (xp.More()) {
                        TopoDS_Edge edge = TopoDS::Edge(xp.Current());
                        edgeLocation = edge.Location();
                        edge.Location(TopLoc_Location());
                        mkWire.Add(edge);
                        xp.Next();
                    }
                    movedSourceWire = mkWire.Wire();
                }
                mkOffset.AddWire(movedSourceWire);
                mkOffset.Perform(offset);

                offsetShape = mkOffset.Shape();
                offsetShape.Move(edgeLocation);
                offsetShape.Move(wireLocation);
            }
            else {
                //stupid OCC doesn't understand, what to do when offset value is zero =/
                offsetShape = movedSourceWire;
            }

            return offsetShape;
        };

        //first. add wire for reversed part of extrusion
        if (bRev){
            TopoDS_Shape offsetShape = makeOffset(vecRev, distanceRev);
            if (offsetShape.IsNull())
                Standard_Failure::Raise("Tapered shape is empty");
            TopAbs_ShapeEnum type = offsetShape.ShapeType();
            if (type == TopAbs_WIRE) {
                list_of_sections.push_back(TopoDS::Wire(offsetShape));
            }
            else if (type == TopAbs_EDGE) {
                BRepBuilderAPI_MakeWire mkWire(TopoDS::Edge(offsetShape));
                list_of_sections.push_back(mkWire.Wire());
            }
            else {
                Standard_Failure::Raise("Tapered shape type is not supported");
            }
        }

        //next. Add source wire as middle section. Order is important.
        if (bMid){
            list_of_sections.push_back(sourceWire);
        }

        //finally. Forward extrusion offset wire.
        if (bFwd){
            TopoDS_Shape offsetShape = makeOffset(vecFwd, distanceFwd);
            if (offsetShape.IsNull())
                Standard_Failure::Raise("Tapered shape is empty");
            TopAbs_ShapeEnum type = offsetShape.ShapeType();
            if (type == TopAbs_WIRE) {
                list_of_sections.push_back(TopoDS::Wire(offsetShape));
            }
            else if (type == TopAbs_EDGE) {
                BRepBuilderAPI_MakeWire mkWire(TopoDS::Edge(offsetShape));
                list_of_sections.push_back(mkWire.Wire());
            }
            else {
                Standard_Failure::Raise("Tapered shape type is not supported");
            }
        }

        //make loft
        BRepOffsetAPI_ThruSections mkGenerator(params.solid ? Standard_True : Standard_False, /*ruled=*/Standard_True);
        for (std::list<TopoDS_Wire>::const_iterator it = list_of_sections.begin(); it != list_of_sections.end(); ++it) {
            const TopoDS_Wire &wire = *it;
            mkGenerator.AddWire(wire);
        }

        try {
#if defined(__GNUC__) && defined (FC_OS_LINUX)
            Base::SignalException se;
#endif
            mkGenerator.Build();
            drafts.push_back(mkGenerator.Shape());
        }
        catch (Standard_Failure &){
            throw;
        }
        catch (...) {
            throw Base::CADKernelError("Unknown exception from BRepOffsetAPI_ThruSections");
        }
    }
}

//----------------------------------------------------------------

TYPESYSTEM_SOURCE(Part::FaceMakerExtrusion, Part::FaceMakerCheese)

std::string FaceMakerExtrusion::getUserFriendlyName() const
{
    return std::string(QT_TRANSLATE_NOOP("Part_FaceMaker","Part Extrude facemaker"));
}

std::string FaceMakerExtrusion::getBriefExplanation() const
{
    return std::string(QT_TRANSLATE_NOOP("Part_FaceMaker","Supports making faces with holes, does not support nesting."));
}

void FaceMakerExtrusion::Build()
{
    this->NotDone();
    this->myGenerated.Clear();
    this->myShapesToReturn.clear();
    this->myShape = TopoDS_Shape();
    TopoDS_Shape inputShape;
    if (mySourceShapes.empty())
        throw Base::ValueError("No input shapes!");
    if (mySourceShapes.size() == 1){
        inputShape = mySourceShapes[0];
    } else {
        TopoDS_Builder builder;
        TopoDS_Compound cmp;
        builder.MakeCompound(cmp);
        for (const TopoDS_Shape& sh: mySourceShapes){
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
        for (int i=1; i<=mapOfEdges.Extent(); i++) {
            BRepBuilderAPI_MakeWire mkWire(TopoDS::Edge(mapOfEdges.FindKey(i)));
            wires.push_back(mkWire.Wire());
        }
    }
    else {
        wires.reserve(mapOfWires.Extent());
        for (int i=1; i<=mapOfWires.Extent(); i++) {
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
