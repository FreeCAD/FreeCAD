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
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <gp_Ax3.hxx>
#include <gp_Pln.hxx>

#include <App/ElementMap.h>
#include <App/GeoFeature.h>
#include <App/IndexedName.h>
#include <App/MappedName.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Tools.h>
#include <Mod/Part/App/FaceMaker.h>
#include <Mod/Part/App/TopoShape.h>

#include "Features.h"


using namespace SheetDesign;

namespace
{

const double tol = 1.0e-6;
const App::PropertyAngle::Constraints angleRange = {0.0, 180.0, 1.0};
const char* SpaceEnums[] = {"Folded", "Flat", nullptr};
const char* PositionEnums[] = {"Material Outside", "Material Inside", "Thickness Outside", nullptr};
const char* LengthTypeEnums[] = {"Leg", "Outer Sharp", "Inner Sharp", "Tangential", nullptr};

/**
 * Make one face from an outer wire and holes that are apart from each other.
 *
 * This is the usual profile of a sheet part. The general face maker of the Part module
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
        throw Base::ValueError("Sheet design: the profile has no closed wire");
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

TopoDS_Shape profileShape(App::DocumentObject* object, const gp_Trsf& toLocal)
{
    if (!object) {
        throw Base::ValueError("Sheet design: no profile is set");
    }
    TopoDS_Shape shape = Part::Feature::getShape(
        object,
        Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform
    );
    if (shape.IsNull()) {
        throw Base::ValueError("Sheet design: the profile has no shape");
    }
    // The shape is in the coordinates of its container. Move it into those of the feature.
    shape.Move(TopLoc_Location(toLocal));
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
        throw Base::ValueError(std::string("Sheet design: the rule has no numeric property ") + name);
    }
    return property->getValue();
}

/// Read the sheet values from the rule object. The usual rule object is an App::VarSet.
SheetParams readRule(const App::DocumentObject* rule)
{
    if (!rule) {
        throw Base::ValueError("Sheet design: no rule is set");
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
TopoDS_Face flatProfile(App::DocumentObject* object, gp_Trsf& placement, const gp_Trsf& toLocal)
{
    const TopoDS_Shape shape = profileShape(object, toLocal);

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
            throw Base::ValueError("Sheet design: the profile is not planar");
        }
        const gp_Pln plane = Handle(Geom_Plane)::DownCast(finder.Surface())->Pln();
        const gp_Ax3 system(plane.Location(), plane.Axis().Direction(), plane.XAxis().Direction());
        placement.SetTransformation(system);
        placement.Invert();
    }

    BRepBuilderAPI_Transform toFlat(shape, placement.Inverted(), Standard_True);
    const std::vector<TopoDS_Face> faces = collectFaces(toFlat.Shape());
    if (faces.size() != 1) {
        throw Base::ValueError("Sheet design: the profile must make one face");
    }
    return faces.front();
}

/// Make a Part shape with stable element names, thus links to its edges and faces can follow
/// a change of the topology.
Part::TopoShape namedShape(const TopoDS_Shape& shape, const ElementNames& names, long tag)
{
    Part::TopoShape result(tag);
    result.setShape(shape);
    result.resetElementMap(std::make_shared<Data::ElementMap>());
    for (const auto& edge : names.edges) {
        result.setElementName(
            Data::IndexedName::fromConst("Edge", edge.first),
            Data::MappedName(edge.second),
            tag
        );
    }
    for (const auto& face : names.faces) {
        result.setElementName(
            Data::IndexedName::fromConst("Face", face.first),
            Data::MappedName(face.second),
            tag
        );
    }
    return result;
}

App::DocumentObjectExecReturn* failure(const char* message)
{
    return new App::DocumentObjectExecReturn(message);
}

}  // namespace


// ------------------------------------------------------------------------------------------------

PROPERTY_SOURCE(SheetDesign::Feature, PartDesign::Feature)

Feature::Feature()
{
    ADD_PROPERTY_TYPE(
        Refine,
        (true),
        "Sheet Design",
        App::Prop_None,
        "Merge the faces of the folded solid that are on the same surface"
    );
    ADD_PROPERTY_TYPE(
        Overlap,
        (0.0),
        "Sheet Design",
        static_cast<App::PropertyType>(App::Prop_ReadOnly | App::Prop_Output),
        "Volume of material that two or more panels have in the same space. "
        "Zero for a part that can exist"
    );
}

std::shared_ptr<SheetModel> Feature::buildModel()
{
    throw Base::NotImplementedError("Sheet design: this feature has no model");
}

gp_Trsf Feature::toLocal(const App::DocumentObject* object) const
{
    // The placement of an object is in the coordinates of its container. The shape of a
    // profile in a different container must go through global coordinates.
    auto containerOf = [](const App::DocumentObject* item) {
        auto* geo = freecad_cast<const App::GeoFeature*>(item);
        return geo ? geo->globalPlacement() * geo->Placement.getValue().inverse() : Base::Placement();
    };
    const Base::Placement relative = containerOf(this).inverse() * containerOf(object);
    return Part::TopoShape::convert(relative.toMatrix());
}

std::shared_ptr<const SheetModel> Feature::getModel()
{
    // A suppressed feature gives the model of the feature before it.
    if (Suppressed.getValue() && modelSource()) {
        return sourceModel(modelSource());
    }
    if (!model) {
        model = buildModel();
    }
    return model;
}

std::shared_ptr<const SheetModel> Feature::sourceModel(App::DocumentObject* object)
{
    auto* feature = freecad_cast<Feature*>(object);
    if (!feature) {
        throw Base::ValueError(
            "Sheet design: the source is not a sheet design feature. Use Unfold to continue from "
            "the solid of a different feature"
        );
    }
    return feature->getModel();
}

App::DocumentObjectExecReturn* Feature::execute()
{
    model.reset();
    try {
        std::shared_ptr<SheetModel> result = buildModel();
        model = result;
        double overlap = 0.0;
        const TopoDS_Shape folded = result->makeFolded(true, Refine.getValue(), &overlap);
        if (!TopExp_Explorer(folded, TopAbs_SOLID).More()) {
            throw Base::CADKernelError("Sheet design: the model gives no solid");
        }
        Shape.setValue(namedShape(folded, result->nameFolded(folded), getID()));
        Overlap.setValue(overlap);
        if (overlap > 0.0) {
            Base::Console().warning(
                "Sheet design: {}: {:.1f} mm^3 of material is in the same space as other material. "
                "Panels of the folded part collide.\n",
                getNameInDocument(),
                overlap
            );
        }
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

PROPERTY_SOURCE(SheetDesign::BasePanel, SheetDesign::Feature)

BasePanel::BasePanel()
{
    ADD_PROPERTY_TYPE(Profile, (nullptr), "Sheet Design", App::Prop_None, "Closed planar profile");
    ADD_PROPERTY_TYPE(
        Rule,
        (nullptr),
        "Sheet Design",
        App::Prop_None,
        "Object with the properties Thickness, BendRadius, and KFactor"
    );
    Profile.setScope(App::LinkScope::Global);
    Rule.setScope(App::LinkScope::Global);
}

std::shared_ptr<SheetModel> BasePanel::buildModel()
{
    const SheetParams params = readRule(Rule.getValue());
    gp_Trsf placement;
    const TopoDS_Face face = flatProfile(Profile.getValue(), placement, toLocal(Profile.getValue()));
    return std::make_shared<SheetModel>(
        SheetModel::makeBase(face, placement, params, getNameInDocument())
    );
}


// ------------------------------------------------------------------------------------------------

PROPERTY_SOURCE(SheetDesign::Refold, SheetDesign::Feature)

Refold::Refold()
{
    ADD_PROPERTY_TYPE(Profile, (nullptr), "Sheet Design", App::Prop_None, "Outline of the flat pattern");
    ADD_PROPERTY_TYPE(
        BendLines,
        (nullptr),
        "Sheet Design",
        App::Prop_None,
        "Straight lines in the plane of the profile: the center lines of the bends"
    );
    ADD_PROPERTY_TYPE(
        Angles,
        (90.0),
        "Sheet Design",
        App::Prop_None,
        "Bend angle of each line in degrees. A negative value bends to the opposite side. "
        "A line without a value uses the last value"
    );
    ADD_PROPERTY_TYPE(
        Rule,
        (nullptr),
        "Sheet Design",
        App::Prop_None,
        "Object with the properties Thickness, BendRadius, and KFactor"
    );
    Profile.setScope(App::LinkScope::Global);
    BendLines.setScope(App::LinkScope::Global);
    Rule.setScope(App::LinkScope::Global);
}

std::shared_ptr<SheetModel> Refold::buildModel()
{
    const SheetParams params = readRule(Rule.getValue());
    gp_Trsf placement;
    const TopoDS_Face face = flatProfile(Profile.getValue(), placement, toLocal(Profile.getValue()));

    BRepBuilderAPI_Transform toFlat(
        profileShape(BendLines.getValue(), toLocal(BendLines.getValue())),
        placement.Inverted(),
        Standard_True
    );
    const std::vector<double>& angles = Angles.getValues();
    std::vector<BendLine> lines;
    std::size_t lineNumber = 0;
    for (TopExp_Explorer ex(toFlat.Shape(), TopAbs_EDGE); ex.More(); ex.Next()) {
        BRepAdaptor_Curve curve(TopoDS::Edge(ex.Current()));
        const gp_Pnt p = curve.Value(curve.FirstParameter());
        const gp_Pnt q = curve.Value(curve.LastParameter());
        if (curve.GetType() != GeomAbs_Line || std::fabs(p.Z()) > 1.0e-5
            || std::fabs(q.Z()) > 1.0e-5) {
            throw Base::ValueError(
                "Sheet design: a bend line must be a straight line in the plane of the profile"
            );
        }
        double degrees = 90.0;
        if (!angles.empty()) {
            degrees = angles[std::min(lineNumber, angles.size() - 1)];
        }
        ++lineNumber;
        if (std::fabs(degrees) < 1.0e-9) {
            // A score with the angle zero is flat in this state of the part.
            continue;
        }
        BendLine line;
        line.a = gp_Pnt2d(p.X(), p.Y());
        line.b = gp_Pnt2d(q.X(), q.Y());
        line.angle = Base::toRadians(std::fabs(degrees));
        line.up = degrees >= 0.0;
        lines.push_back(line);
    }
    if (lines.empty()) {
        throw Base::ValueError("Sheet design: the bend line object has no line");
    }
    return std::make_shared<SheetModel>(
        SheetModel::fromFlat(face, lines, placement, params, getNameInDocument())
    );
}


// ------------------------------------------------------------------------------------------------

PROPERTY_SOURCE(SheetDesign::Unfold, SheetDesign::Feature)

Unfold::Unfold()
{
    ADD_PROPERTY_TYPE(Face, (nullptr), "Sheet Design", App::Prop_None, "Flat face of the folded solid");
    ADD_PROPERTY_TYPE(
        Rule,
        (nullptr),
        "Sheet Design",
        App::Prop_None,
        "Object with the properties Thickness, BendRadius, and KFactor. "
        "The thickness comes from the solid"
    );
    Face.setScope(App::LinkScope::Global);
    Rule.setScope(App::LinkScope::Global);
}

std::shared_ptr<SheetModel> Unfold::buildModel()
{
    const SheetParams params = readRule(Rule.getValue());
    const std::vector<std::string>& subs = Face.getSubValues();
    if (!Face.getValue() || subs.size() != 1) {
        throw Base::ValueError("Sheet design: select one flat face of a solid");
    }
    const TopoDS_Shape solid = profileShape(Face.getValue(), toLocal(Face.getValue()));
    const TopoDS_Shape sub = Part::TopoShape(solid).getSubShape(subs.front().c_str());
    if (sub.IsNull() || sub.ShapeType() != TopAbs_FACE) {
        throw Base::ValueError("Sheet design: select one flat face of a solid");
    }
    auto result = std::make_shared<SheetModel>(
        SheetModel::fromSolid(solid, TopoDS::Face(sub), params, getNameInDocument())
    );

    // The model has only the panels and the bends that the function can read. A difference in
    // volume shows that the solid has more than that.
    GProp_GProps source;
    GProp_GProps made;
    BRepGProp::VolumeProperties(solid, source);
    BRepGProp::VolumeProperties(result->makeFolded(false, false), made);
    if (std::fabs(source.Mass() - made.Mass()) > 1.0e-3 * std::fabs(source.Mass())) {
        Base::Console().warning(
            "Sheet design: {}: the volume of the model ({:.3f}) is different from the volume of "
            "the "
            "solid ({:.3f}). The solid has geometry that this version cannot read.\n",
            getNameInDocument(),
            made.Mass(),
            source.Mass()
        );
    }
    return result;
}


// ------------------------------------------------------------------------------------------------

PROPERTY_SOURCE(SheetDesign::Flange, SheetDesign::Feature)

Flange::Flange()
{
    ADD_PROPERTY_TYPE(Edge, (nullptr), "Sheet Design", App::Prop_None, "Straight edge for the bend");
    ADD_PROPERTY_TYPE(Length, (10.0), "Sheet Design", App::Prop_None, "Length of the flat flange");
    ADD_PROPERTY_TYPE(Angle, (90.0), "Sheet Design", App::Prop_None, "Bend angle");
    ADD_PROPERTY_TYPE(Reversed, (false), "Sheet Design", App::Prop_None, "Bend to the opposite side");
    ADD_PROPERTY_TYPE(Gap1, (0.0), "Sheet Design", App::Prop_None, "Gap at the start of the edge");
    ADD_PROPERTY_TYPE(Gap2, (0.0), "Sheet Design", App::Prop_None, "Gap at the end of the edge");
    ADD_PROPERTY_TYPE(
        Offset,
        (0.0),
        "Sheet Design",
        App::Prop_None,
        "Moves the bend line out of the panel before the bend. Use one thickness for a flap "
        "that folds across a second flap"
    );
    Angle.setConstraints(&angleRange);
    ADD_PROPERTY_TYPE(
        Position,
        (0L),
        "Sheet Design",
        App::Prop_None,
        "Where the flange is against the selected edge. Material Outside: the bend starts at "
        "the edge. Material Inside: the outer face of the flange is at the edge. Thickness "
        "Outside: the inner face of the flange is at the edge"
    );
    Position.setEnums(PositionEnums);
    ADD_PROPERTY_TYPE(
        LengthType,
        (0L),
        "Sheet Design",
        App::Prop_None,
        "How Length is measured. Leg: the flat part of the flange. Outer Sharp, Inner Sharp: "
        "from the point where the outer or the inner faces of the two panels meet. Tangential: "
        "from the plane that touches the outer side of the bend"
    );
    LengthType.setEnums(LengthTypeEnums);
}

std::shared_ptr<SheetModel> Flange::buildModel()
{
    const std::vector<std::string>& subs = Edge.getSubValues();
    if (!Edge.getValue() || subs.size() != 1) {
        throw Base::ValueError("Sheet design: select one edge of a sheet design feature");
    }
    // This gives the error for a source that has a solid but no sheet model.
    sourceModel(Edge.getValue());
    auto* source = freecad_cast<Feature*>(Edge.getValue());
    if (!source) {
        throw Base::ValueError("Sheet design: select one edge of a sheet design feature");
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
        throw Base::ValueError("Sheet design: the selection is not a straight edge of a flat panel");
    }

    const bool up = ref.top != Reversed.getValue();
    // The position moves the bend line against the edge. Offset adds to it.
    const double t = result->params.thickness;
    const double r = result->params.radius;
    const double angle = Base::toRadians(Angle.getValue());
    double shift = Offset.getValue();
    if (Position.getValue() == 1) {
        shift -= r + t;  // Material Inside
    }
    else if (Position.getValue() == 2) {
        shift -= r;  // Thickness Outside
    }
    // The length types give the flat leg of the flange.
    double leg = Length.getValue();
    switch (LengthType.getValue()) {
        case 1:  // Outer Sharp
            leg -= (r + t) * std::tan(angle / 2.0);
            break;
        case 2:  // Inner Sharp
            leg -= r * std::tan(angle / 2.0);
            break;
        case 3:  // Tangential
            leg -= angle >= M_PI / 2.0 ? r + t : (r + t) * std::tan(angle / 2.0);
            break;
        default:
            break;
    }
    if (leg <= 1.0e-6) {
        throw Base::ValueError(
            "Sheet design: the length is too small for this length type. The bend uses all of it"
        );
    }
    result->addFlange(ref, leg, angle, up, Gap1.getValue(), Gap2.getValue(), shift, getNameInDocument());

    return result;
}


// ------------------------------------------------------------------------------------------------

PROPERTY_SOURCE(SheetDesign::Cut, SheetDesign::Feature)

Cut::Cut()
{
    ADD_PROPERTY_TYPE(Source, (nullptr), "Sheet Design", App::Prop_None, "Feature to cut");
    ADD_PROPERTY_TYPE(Profile, (nullptr), "Sheet Design", App::Prop_None, "Closed planar profile");
    ADD_PROPERTY_TYPE(
        Space,
        (0L),
        "Sheet Design",
        App::Prop_None,
        "Folded: the profile is on a panel. Flat: the profile is on the flat pattern"
    );
    Space.setEnums(SpaceEnums);
    Profile.setScope(App::LinkScope::Global);
}

std::shared_ptr<SheetModel> Cut::buildModel()
{
    auto result = std::make_shared<SheetModel>(*sourceModel(Source.getValue()));
    const std::vector<TopoDS_Face> faces = collectFaces(
        profileShape(Profile.getValue(), toLocal(Profile.getValue()))
    );
    result->cut(makeCompound(faces), Space.getValue() == 1, getNameInDocument());
    return result;
}


// ------------------------------------------------------------------------------------------------

PROPERTY_SOURCE(SheetDesign::FlatPattern, Part::Feature)

FlatPattern::FlatPattern()
{
    ADD_PROPERTY_TYPE(Source, (nullptr), "Sheet Design", App::Prop_None, "Sheet design feature");
    ADD_PROPERTY_TYPE(BendLines, (true), "Sheet Design", App::Prop_None, "Add the bend center lines");
    Source.setScope(App::LinkScope::Global);
}

App::DocumentObjectExecReturn* FlatPattern::execute()
{
    auto* source = freecad_cast<SheetDesign::Feature*>(Source.getValue());
    if (!source) {
        return failure("Sheet design: the source is not a sheet design feature");
    }
    try {
        std::shared_ptr<const SheetModel> sheet = source->getModel();
        BRep_Builder builder;
        TopoDS_Compound compound;
        builder.MakeCompound(compound);
        // Put the flat pattern in the plane of the root panel. The compound keeps an identity
        // location, because a Part feature replaces that location with its placement.
        // The model is in the coordinates of the container of its feature, for example its
        // Body. The flat pattern uses the same coordinates. It does not follow a later move of
        // the Body, because a move of a Body does not recompute its features.
        const TopLoc_Location root(sheet->rootPlacement());
        const TopoDS_Shape flat = sheet->makeFlat();
        builder.Add(compound, flat.Moved(root));
        // The edges of the face are the first edges of the compound, in the same sequence.
        ElementNames names = sheet->nameFlat(flat);
        TopTools_IndexedMapOfShape flatEdges;
        TopExp::MapShapes(flat, TopAbs_EDGE, flatEdges);
        if (BendLines.getValue()) {
            const std::vector<TopoDS_Edge> lines = sheet->bendLines();
            for (std::size_t i = 0; i < lines.size(); ++i) {
                builder.Add(compound, lines[i].Moved(root));
                names.edges.emplace_back(
                    flatEdges.Extent() + static_cast<int>(i) + 1,
                    sheet->bendLineName(i)
                );
            }
        }
        Shape.setValue(namedShape(compound, names, getID()));
    }
    catch (const Standard_Failure& e) {
        return failure(e.GetMessageString());
    }
    catch (const Base::Exception& e) {
        return failure(e.what());
    }
    return App::DocumentObject::StdReturn;
}
