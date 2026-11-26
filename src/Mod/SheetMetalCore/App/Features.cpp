// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include <cmath>
#include <string>

#include <algorithm>

#include <BRepAdaptor_Curve.hxx>
#include <BRepBndLib.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <BRepLib_FindSurface.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepTools.hxx>
#include <BRep_Tool.hxx>
#include <Geom_Surface.hxx>
#include <TopoDS_Wire.hxx>
#include <BRep_Builder.hxx>
#include <Bnd_Box.hxx>
#include <Geom_Plane.hxx>
#include <Standard_Failure.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <gp_Ax3.hxx>
#include <gp_Pln.hxx>

#include <App/GeoFeature.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Tools.h>
#include <Mod/Part/App/FaceMaker.h>
#include <Mod/Part/App/TopoShape.h>

#include "Features.h"


using namespace SheetMetalCore;

namespace
{

const double tol = 1.0e-6;
const App::PropertyAngle::Constraints angleRange = {0.0, 180.0, 1.0};
const char* SpaceEnums[] = {"Folded", "Flat", nullptr};

/**
 * Make one face from an outer wire and holes that are apart from each other.
 *
 * This is the usual profile of a sheet metal part. The general face maker of the Part module
 * needs much more time for a profile with many holes. Returns false if the wires do not have
 * this simple arrangement.
 */
bool makeSimpleFace(const std::vector<TopoDS_Wire>& wires, TopoDS_Face& face)
{
    std::vector<Bnd_Box> boxes(wires.size());
    for (std::size_t i = 0; i < wires.size(); ++i) {
        if (!wires[i].Closed()) {
            return false;
        }
        BRepBndLib::Add(wires[i], boxes[i]);
    }
    // The outer wire has the box with the largest diagonal.
    std::size_t outer = 0;
    for (std::size_t i = 1; i < wires.size(); ++i) {
        if (boxes[i].SquareExtent() > boxes[outer].SquareExtent()) {
            outer = i;
        }
    }
    for (std::size_t i = 0; i < wires.size(); ++i) {
        if (i == outer) {
            continue;
        }
        Bnd_Box both = boxes[outer];
        both.Add(boxes[i]);
        if (both.SquareExtent() > boxes[outer].SquareExtent() + tol) {
            return false;  // Not fully in the box of the outer wire.
        }
        for (std::size_t j = i + 1; j < wires.size(); ++j) {
            if (j != outer && !boxes[i].IsOut(boxes[j])) {
                return false;  // Two holes can touch or one can be in the other.
            }
        }
    }
    BRepBuilderAPI_MakeFace maker(wires[outer], Standard_True);
    if (!maker.IsDone()) {
        return false;
    }
    const Handle(Geom_Surface) surface = BRep_Tool::Surface(maker.Face());
    for (std::size_t i = 0; i < wires.size(); ++i) {
        if (i == outer) {
            continue;
        }
        // A face from the hole wire gives the wire the same turn direction as the outer wire.
        BRepBuilderAPI_MakeFace hole(surface, wires[i], Standard_True);
        if (!hole.IsDone()) {
            return false;
        }
        maker.Add(TopoDS::Wire(BRepTools::OuterWire(hole.Face()).Reversed()));
    }
    face = maker.Face();
    return true;
}

/// Collect the faces of a shape. If the shape has no face, make faces from its closed wires.
std::vector<TopoDS_Face> collectFaces(const TopoDS_Shape& shape)
{
    std::vector<TopoDS_Face> faces;
    for (TopExp_Explorer ex(shape, TopAbs_FACE); ex.More(); ex.Next()) {
        faces.push_back(TopoDS::Face(ex.Current()));
    }
    if (!faces.empty()) {
        return faces;
    }
    std::vector<TopoDS_Wire> wires;
    for (TopExp_Explorer ex(shape, TopAbs_WIRE); ex.More(); ex.Next()) {
        wires.push_back(TopoDS::Wire(ex.Current()));
    }
    if (wires.empty()) {
        throw Base::ValueError("Sheet metal: the profile has no closed wire");
    }
    TopoDS_Face simple;
    if (makeSimpleFace(wires, simple)) {
        faces.push_back(simple);
        return faces;
    }
    auto maker = Part::FaceMaker::ConstructFromType("Part::FaceMakerBullseye");
    for (const TopoDS_Wire& wire : wires) {
        maker->addWire(wire);
    }
    maker->Build();
    for (TopExp_Explorer ex(maker->Shape(), TopAbs_FACE); ex.More(); ex.Next()) {
        faces.push_back(TopoDS::Face(ex.Current()));
    }
    return faces;
}

TopoDS_Shape profileShape(App::DocumentObject* object)
{
    if (!object) {
        throw Base::ValueError("Sheet metal: no profile is set");
    }
    TopoDS_Shape shape = Part::Feature::getShape(
        object,
        Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform
    );
    if (shape.IsNull()) {
        throw Base::ValueError("Sheet metal: the profile has no shape");
    }
    return shape;
}

TopoDS_Shape makeCompound(const std::vector<TopoDS_Face>& faces)
{
    BRep_Builder builder;
    TopoDS_Compound compound;
    builder.MakeCompound(compound);
    for (const TopoDS_Face& face : faces) {
        builder.Add(compound, face);
    }
    return compound;
}

bool isInFlatPlane(const TopoDS_Shape& shape)
{
    Bnd_Box box;
    BRepBndLib::Add(shape, box);
    if (box.IsVoid()) {
        return false;
    }
    double xmin, ymin, zmin, xmax, ymax, zmax;
    box.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    // The box has a gap around the shape, thus use a tolerance that is not tight.
    return std::fabs(zmin) < 1.0e-4 && std::fabs(zmax) < 1.0e-4;
}

/// Read a number from a property of the rule object. A quantity gives its value in mm.
double readRuleValue(const App::DocumentObject* rule, const char* name)
{
    auto* property = freecad_cast<App::PropertyFloat*>(rule->getPropertyByName(name));
    if (!property) {
        throw Base::ValueError(std::string("Sheet metal: the rule has no numeric property ") + name);
    }
    return property->getValue();
}

/// Read the sheet values from the rule object. The usual rule object is an App::VarSet.
SheetParams readRule(const App::DocumentObject* rule)
{
    if (!rule) {
        throw Base::ValueError("Sheet metal: no rule is set");
    }
    SheetParams params;
    params.thickness = readRuleValue(rule, "Thickness");
    params.radius = readRuleValue(rule, "BendRadius");
    params.kFactor = readRuleValue(rule, "KFactor");
    return params;
}

/**
 * Get the one face of a planar profile object in flat space (the plane z = 0).
 *
 * @param placement  Receives the transformation from flat space to the position of the profile.
 */
TopoDS_Face flatProfile(App::DocumentObject* object, gp_Trsf& placement)
{
    const TopoDS_Shape shape = profileShape(object);

    // Use the placement of the profile object if the profile is in its XY plane. This keeps the
    // flat coordinates equal to the sketch coordinates.
    bool found = false;
    if (auto* geo = freecad_cast<App::GeoFeature*>(object)) {
        placement = Part::TopoShape::convert(geo->Placement.getValue().toMatrix());
        BRepBuilderAPI_Transform local(shape, placement.Inverted(), Standard_True);
        found = isInFlatPlane(local.Shape());
    }
    if (!found) {
        BRepLib_FindSurface finder(shape, tol, Standard_True);
        if (!finder.Found()) {
            throw Base::ValueError("Sheet metal: the profile is not planar");
        }
        const gp_Pln plane = Handle(Geom_Plane)::DownCast(finder.Surface())->Pln();
        const gp_Ax3 system(plane.Location(), plane.Axis().Direction(), plane.XAxis().Direction());
        placement.SetTransformation(system);
        placement.Invert();
    }

    BRepBuilderAPI_Transform toFlat(shape, placement.Inverted(), Standard_True);
    const std::vector<TopoDS_Face> faces = collectFaces(toFlat.Shape());
    if (faces.size() != 1) {
        throw Base::ValueError("Sheet metal: the profile must make one face");
    }
    return faces.front();
}

App::DocumentObjectExecReturn* failure(const char* message)
{
    return new App::DocumentObjectExecReturn(message);
}

}  // namespace


// ------------------------------------------------------------------------------------------------

PROPERTY_SOURCE(SheetMetalCore::Feature, Part::Feature)

Feature::Feature()
{
    ADD_PROPERTY_TYPE(
        Refine,
        (true),
        "Sheet Metal",
        App::Prop_None,
        "Merge the faces of the folded solid that are on the same surface"
    );
}

std::shared_ptr<SheetModel> Feature::buildModel()
{
    throw Base::NotImplementedError("Sheet metal: this feature has no model");
}

std::shared_ptr<const SheetModel> Feature::getModel()
{
    if (!model) {
        model = buildModel();
    }
    return model;
}

std::shared_ptr<const SheetModel> Feature::sourceModel(App::DocumentObject* object)
{
    auto* feature = freecad_cast<Feature*>(object);
    if (!feature) {
        throw Base::ValueError("Sheet metal: the source is not a sheet metal feature");
    }
    return feature->getModel();
}

App::DocumentObjectExecReturn* Feature::execute()
{
    model.reset();
    try {
        std::shared_ptr<SheetModel> result = buildModel();
        model = result;
        Shape.setValue(result->makeFolded(true, Refine.getValue()));
    }
    catch (const Standard_Failure& e) {
        return failure(e.GetMessageString());
    }
    catch (const Base::Exception& e) {
        return failure(e.what());
    }
    return App::DocumentObject::StdReturn;
}


// ------------------------------------------------------------------------------------------------

PROPERTY_SOURCE(SheetMetalCore::BasePanel, SheetMetalCore::Feature)

BasePanel::BasePanel()
{
    ADD_PROPERTY_TYPE(Profile, (nullptr), "Sheet Metal", App::Prop_None, "Closed planar profile");
    ADD_PROPERTY_TYPE(
        Rule,
        (nullptr),
        "Sheet Metal",
        App::Prop_None,
        "Object with the properties Thickness, BendRadius, and KFactor"
    );
}

std::shared_ptr<SheetModel> BasePanel::buildModel()
{
    const SheetParams params = readRule(Rule.getValue());
    gp_Trsf placement;
    const TopoDS_Face face = flatProfile(Profile.getValue(), placement);
    return std::make_shared<SheetModel>(SheetModel::makeBase(face, placement, params));
}


// ------------------------------------------------------------------------------------------------

PROPERTY_SOURCE(SheetMetalCore::FromFlat, SheetMetalCore::Feature)

FromFlat::FromFlat()
{
    ADD_PROPERTY_TYPE(Profile, (nullptr), "Sheet Metal", App::Prop_None, "Outline of the flat pattern");
    ADD_PROPERTY_TYPE(
        BendLines,
        (nullptr),
        "Sheet Metal",
        App::Prop_None,
        "Straight lines in the plane of the profile: the center lines of the bends"
    );
    ADD_PROPERTY_TYPE(
        Angles,
        (90.0),
        "Sheet Metal",
        App::Prop_None,
        "Bend angle of each line in degrees. A negative value bends to the opposite side. "
        "A line without a value uses the last value"
    );
    ADD_PROPERTY_TYPE(
        Rule,
        (nullptr),
        "Sheet Metal",
        App::Prop_None,
        "Object with the properties Thickness, BendRadius, and KFactor"
    );
}

std::shared_ptr<SheetModel> FromFlat::buildModel()
{
    const SheetParams params = readRule(Rule.getValue());
    gp_Trsf placement;
    const TopoDS_Face face = flatProfile(Profile.getValue(), placement);

    BRepBuilderAPI_Transform toFlat(
        profileShape(BendLines.getValue()),
        placement.Inverted(),
        Standard_True
    );
    const std::vector<double>& angles = Angles.getValues();
    std::vector<BendLine> lines;
    for (TopExp_Explorer ex(toFlat.Shape(), TopAbs_EDGE); ex.More(); ex.Next()) {
        BRepAdaptor_Curve curve(TopoDS::Edge(ex.Current()));
        const gp_Pnt p = curve.Value(curve.FirstParameter());
        const gp_Pnt q = curve.Value(curve.LastParameter());
        if (curve.GetType() != GeomAbs_Line || std::fabs(p.Z()) > 1.0e-5
            || std::fabs(q.Z()) > 1.0e-5) {
            throw Base::ValueError(
                "Sheet metal: a bend line must be a straight line in the plane of the profile"
            );
        }
        double degrees = 90.0;
        if (!angles.empty()) {
            degrees = angles[std::min(lines.size(), angles.size() - 1)];
        }
        BendLine line;
        line.a = gp_Pnt2d(p.X(), p.Y());
        line.b = gp_Pnt2d(q.X(), q.Y());
        line.angle = Base::toRadians(std::fabs(degrees));
        line.up = degrees >= 0.0;
        lines.push_back(line);
    }
    if (lines.empty()) {
        throw Base::ValueError("Sheet metal: the bend line object has no line");
    }
    return std::make_shared<SheetModel>(SheetModel::fromFlat(face, lines, placement, params));
}


// ------------------------------------------------------------------------------------------------

PROPERTY_SOURCE(SheetMetalCore::FromSolid, SheetMetalCore::Feature)

FromSolid::FromSolid()
{
    ADD_PROPERTY_TYPE(Face, (nullptr), "Sheet Metal", App::Prop_None, "Flat face of the folded solid");
    ADD_PROPERTY_TYPE(
        Rule,
        (nullptr),
        "Sheet Metal",
        App::Prop_None,
        "Object with the properties Thickness, BendRadius, and KFactor. "
        "The thickness comes from the solid"
    );
}

std::shared_ptr<SheetModel> FromSolid::buildModel()
{
    const SheetParams params = readRule(Rule.getValue());
    const std::vector<std::string>& subs = Face.getSubValues();
    if (!Face.getValue() || subs.size() != 1) {
        throw Base::ValueError("Sheet metal: select one flat face of a solid");
    }
    const TopoDS_Shape solid = profileShape(Face.getValue());
    const TopoDS_Shape sub = Part::TopoShape(solid).getSubShape(subs.front().c_str());
    if (sub.IsNull() || sub.ShapeType() != TopAbs_FACE) {
        throw Base::ValueError("Sheet metal: select one flat face of a solid");
    }
    auto result = std::make_shared<SheetModel>(SheetModel::fromSolid(solid, TopoDS::Face(sub), params));

    // The model has only the panels and the bends that the function can read. A difference in
    // volume shows that the solid has more than that.
    GProp_GProps source;
    GProp_GProps made;
    BRepGProp::VolumeProperties(solid, source);
    BRepGProp::VolumeProperties(result->makeFolded(false, false), made);
    if (std::fabs(source.Mass() - made.Mass()) > 1.0e-3 * std::fabs(source.Mass())) {
        Base::Console().warning(
            "Sheet metal: %s: the volume of the model (%.3f) is different from the volume of the "
            "solid (%.3f). The solid has geometry that this version cannot read.\n",
            getNameInDocument(),
            made.Mass(),
            source.Mass()
        );
    }
    return result;
}


// ------------------------------------------------------------------------------------------------

PROPERTY_SOURCE(SheetMetalCore::Flange, SheetMetalCore::Feature)

Flange::Flange()
{
    ADD_PROPERTY_TYPE(Edge, (nullptr), "Sheet Metal", App::Prop_None, "Straight edge for the bend");
    ADD_PROPERTY_TYPE(Length, (10.0), "Sheet Metal", App::Prop_None, "Length of the flat flange");
    ADD_PROPERTY_TYPE(Angle, (90.0), "Sheet Metal", App::Prop_None, "Bend angle");
    ADD_PROPERTY_TYPE(Reversed, (false), "Sheet Metal", App::Prop_None, "Bend to the opposite side");
    ADD_PROPERTY_TYPE(Gap1, (0.0), "Sheet Metal", App::Prop_None, "Gap at the start of the edge");
    ADD_PROPERTY_TYPE(Gap2, (0.0), "Sheet Metal", App::Prop_None, "Gap at the end of the edge");
    Angle.setConstraints(&angleRange);
}

std::shared_ptr<SheetModel> Flange::buildModel()
{
    auto* source = freecad_cast<Feature*>(Edge.getValue());
    const std::vector<std::string>& subs = Edge.getSubValues();
    if (!source || subs.size() != 1) {
        throw Base::ValueError("Sheet metal: select one edge of a sheet metal feature");
    }
    auto result = std::make_shared<SheetModel>(*sourceModel(source));

    // The model is in the coordinates of the source shape without its placement.
    TopoDS_Shape whole = source->Shape.getValue();
    whole.Location(TopLoc_Location());
    const TopoDS_Shape sub = Part::TopoShape(whole).getSubShape(subs.front().c_str());

    EdgeRef ref;
    bool found = false;
    if (sub.ShapeType() == TopAbs_EDGE) {
        found = result->findEdge(TopoDS::Edge(sub), ref);
    }
    else if (sub.ShapeType() == TopAbs_FACE) {
        // A side face: use its edge on the top side if there is one.
        for (TopExp_Explorer ex(sub, TopAbs_EDGE); ex.More(); ex.Next()) {
            EdgeRef candidate;
            if (result->findEdge(TopoDS::Edge(ex.Current()), candidate)) {
                if (!found || candidate.top) {
                    ref = candidate;
                }
                found = true;
            }
        }
    }
    if (!found) {
        throw Base::ValueError("Sheet metal: the selection is not a straight edge of a flat panel");
    }

    const bool up = ref.top != Reversed.getValue();
    result->addFlange(
        ref,
        Length.getValue(),
        Base::toRadians(Angle.getValue()),
        up,
        Gap1.getValue(),
        Gap2.getValue()
    );
    return result;
}


// ------------------------------------------------------------------------------------------------

PROPERTY_SOURCE(SheetMetalCore::Cut, SheetMetalCore::Feature)

Cut::Cut()
{
    ADD_PROPERTY_TYPE(Source, (nullptr), "Sheet Metal", App::Prop_None, "Feature to cut");
    ADD_PROPERTY_TYPE(Profile, (nullptr), "Sheet Metal", App::Prop_None, "Closed planar profile");
    ADD_PROPERTY_TYPE(
        Space,
        (0L),
        "Sheet Metal",
        App::Prop_None,
        "Folded: the profile is on a panel. Flat: the profile is on the flat pattern"
    );
    Space.setEnums(SpaceEnums);
}

std::shared_ptr<SheetModel> Cut::buildModel()
{
    auto result = std::make_shared<SheetModel>(*sourceModel(Source.getValue()));
    const std::vector<TopoDS_Face> faces = collectFaces(profileShape(Profile.getValue()));
    result->cut(makeCompound(faces), Space.getValue() == 1);
    return result;
}


// ------------------------------------------------------------------------------------------------

PROPERTY_SOURCE(SheetMetalCore::FlatPattern, Part::Feature)

FlatPattern::FlatPattern()
{
    ADD_PROPERTY_TYPE(Source, (nullptr), "Sheet Metal", App::Prop_None, "Sheet metal feature");
    ADD_PROPERTY_TYPE(BendLines, (true), "Sheet Metal", App::Prop_None, "Add the bend center lines");
}

App::DocumentObjectExecReturn* FlatPattern::execute()
{
    auto* source = freecad_cast<SheetMetalCore::Feature*>(Source.getValue());
    if (!source) {
        return failure("Sheet metal: the source is not a sheet metal feature");
    }
    try {
        std::shared_ptr<const SheetModel> sheet = source->getModel();
        BRep_Builder builder;
        TopoDS_Compound compound;
        builder.MakeCompound(compound);
        // Put the flat pattern in the plane of the root panel. The compound keeps an identity
        // location, because a Part feature replaces that location with its placement.
        const TopLoc_Location root(sheet->rootPlacement());
        builder.Add(compound, sheet->makeFlat().Moved(root));
        if (BendLines.getValue()) {
            for (const TopoDS_Edge& line : sheet->bendLines()) {
                builder.Add(compound, line.Moved(root));
            }
        }
        Shape.setValue(compound);
    }
    catch (const Standard_Failure& e) {
        return failure(e.GetMessageString());
    }
    catch (const Base::Exception& e) {
        return failure(e.what());
    }
    return App::DocumentObject::StdReturn;
}
