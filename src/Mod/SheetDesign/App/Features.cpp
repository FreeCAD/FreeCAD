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
#include <array>

#include <BRepAdaptor_Curve.hxx>
#include <BRepBndLib.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <BRepLib_FindSurface.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepTools.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <TopoDS_Vertex.hxx>
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
const char* SpaceEnums[] = {"Folded", "Flat", "Wrapped", nullptr};
const char* HemEnums[] = {"Flat", "Open", "Teardrop", "Rolled", nullptr};
const char* CornerEnums[] = {"Circle", "Square", nullptr};
const char* BaseShapeEnums[] = {"Plate", "Angle", "Channel", "Tub", "Hat", "Box", nullptr};
const char* PositionEnums[] = {"Material Outside", "Material Inside", "Thickness Outside", nullptr};
const char* ReliefTypeEnums[] = {"Rule", "None", "Rectangle", "Round", nullptr};
const App::PropertyAngle::Constraints miterRange = {-80.0, 80.0, 1.0};
const char* CornerTypeEnums[] = {"Open", "Butt", "Overlap", "Underlap", nullptr};
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

/// Read an optional text or choice from a property of the rule object. Empty if not there.
std::string ruleText(const App::DocumentObject* rule, const char* name)
{
    App::Property* property = rule->getPropertyByName(name);
    if (auto* choice = freecad_cast<App::PropertyEnumeration*>(property)) {
        return choice->isValid() ? choice->getValueAsString() : "";
    }
    if (auto* text = freecad_cast<App::PropertyString*>(property)) {
        return text->getValue();
    }
    return {};
}

/**
 * Read the optional K-factor table of the rule: the list properties KFactorRatios (bend radius /
 * thickness) and KFactorValues (the K-factor for each ratio). Empty lists mean no table.
 */
void readKFactorTable(const App::DocumentObject* rule, SheetParams& params, bool din)
{
    auto* ratios = freecad_cast<App::PropertyFloatList*>(rule->getPropertyByName("KFactorRatios"));
    auto* values = freecad_cast<App::PropertyFloatList*>(rule->getPropertyByName("KFactorValues"));
    const std::size_t ratioCount = ratios ? ratios->getValues().size() : 0;
    const std::size_t valueCount = values ? values->getValues().size() : 0;
    if (ratioCount != valueCount) {
        throw Base::ValueError(
            "Sheet design: the K-factor table of the rule must have the same number of ratios "
            "and values"
        );
    }
    for (std::size_t index = 0; index < ratioCount; ++index) {
        const double factor = values->getValues()[index];
        params.kTable.emplace_back(ratios->getValues()[index], din ? factor / 2.0 : factor);
    }
    std::sort(params.kTable.begin(), params.kTable.end());
    for (const auto& entry : params.kTable) {
        if (entry.first < 0.0 || entry.second < 0.0 || entry.second > 1.0) {
            throw Base::ValueError("Sheet design: a value of the K-factor table is not valid");
        }
    }
}

/// The points of an open wire of straight segments, in their sequence.
std::vector<gp_Pnt> sectionPoints(const TopoDS_Wire& wire)
{
    std::vector<gp_Pnt> section;
    TopoDS_Edge last;
    for (BRepTools_WireExplorer ex(wire); ex.More(); ex.Next()) {
        if (BRepAdaptor_Curve(ex.Current()).GetType() != GeomAbs_Line) {
            throw Base::ValueError(
                "Sheet design: the profile of a contour must have only straight segments"
            );
        }
        section.push_back(BRep_Tool::Pnt(ex.CurrentVertex()));
        last = ex.Current();
    }
    if (last.IsNull()) {
        throw Base::ValueError("Sheet design: the profile of a contour has no segment");
    }
    TopoDS_Vertex first;
    TopoDS_Vertex second;
    TopExp::Vertices(last, first, second);
    const gp_Pnt end1 = BRep_Tool::Pnt(first);
    const gp_Pnt end2 = BRep_Tool::Pnt(second);
    section.push_back(end1.Distance(section.back()) > end2.Distance(section.back()) ? end1 : end2);
    return section;
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
    // A K-factor in the DIN convention is two times the value in the ANSI convention.
    const bool din = ruleText(rule, "KFactorStandard") == "DIN";
    if (din) {
        params.kFactor /= 2.0;
    }
    readKFactorTable(rule, params, din);

    // The relief is optional in the rule. It applies to each flange that does not give its own.
    const std::string type = ruleText(rule, "ReliefType");
    if (type == "Rectangle" || type == "Round") {
        params.relief.type = type == "Round" ? Relief::Round : Relief::Rectangle;
        params.relief.width = readRuleValue(rule, "ReliefWidth");
        params.relief.depth = readRuleValue(rule, "ReliefDepth");
    }
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

bool Feature::findEdge(const char* element, EdgeRef& ref)
{
    std::shared_ptr<const SheetModel> sheet = getModel();
    // The model is in the coordinates of the shape without its placement.
    TopoDS_Shape whole = Shape.getValue();
    whole.Location(TopLoc_Location());
    const TopoDS_Shape sub = Part::TopoShape(whole).getSubShape(element, true);
    if (sub.IsNull()) {
        return false;
    }
    bool found = false;
    if (sub.ShapeType() == TopAbs_EDGE) {
        found = sheet->findEdge(TopoDS::Edge(sub), ref);
    }
    else if (sub.ShapeType() == TopAbs_FACE) {
        // A side face: use its edge on the top side if there is one.
        for (TopExp_Explorer ex(sub, TopAbs_EDGE); ex.More(); ex.Next()) {
            EdgeRef candidate;
            if (sheet->findEdge(TopoDS::Edge(ex.Current()), candidate)) {
                if (!found || candidate.top) {
                    ref = candidate;
                }
                found = true;
            }
        }
    }
    return found;
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
    ADD_PROPERTY_TYPE(
        Width,
        (50.0),
        "Sheet Design",
        App::Prop_None,
        "For an open profile: the size of the part at 90 degrees to the plane of the profile"
    );
    ADD_PROPERTY_TYPE(
        Symmetric,
        (false),
        "Sheet Design",
        App::Prop_None,
        "For an open profile: the plane of the profile is in the middle of the width"
    );
    Profile.setScope(App::LinkScope::Global);
    Rule.setScope(App::LinkScope::Global);
}

std::shared_ptr<SheetModel> BasePanel::buildModel()
{
    const SheetParams params = readRule(Rule.getValue());
    const TopoDS_Shape raw = profileShape(Profile.getValue(), toLocal(Profile.getValue()));
    TopExp_Explorer wires(raw, TopAbs_WIRE);
    if (!TopExp_Explorer(raw, TopAbs_FACE).More() && wires.More()
        && !BRep_Tool::IsClosed(TopoDS::Wire(wires.Current()))) {
        // An open profile is the section of the part: one side of the sheet, with sharp corners.
        const std::vector<gp_Pnt> points = sectionPoints(TopoDS::Wire(wires.Current()));
        const double width = Width.getValue();
        if (width <= 1.0e-6) {
            throw Base::ValueError("Sheet design: an open profile needs a width larger than zero");
        }
        gp_Vec across;
        if (auto* geo = freecad_cast<App::GeoFeature*>(Profile.getValue())) {
            Base::Vector3d z;
            geo->Placement.getValue().getRotation().multVec(Base::Vector3d(0, 0, 1), z);
            across = gp_Vec(z.x, z.y, z.z).Transformed(toLocal(Profile.getValue()));
        }
        gp_Vec first(points[0], points[1]);
        const double firstLength = first.Magnitude();
        first.Normalize();
        if (across.Magnitude() < 0.5 || std::fabs(across.Dot(first)) > 1.0e-6) {
            throw Base::ValueError(
                "Sheet design: an open profile must be in the XY plane of its object"
            );
        }
        across.Normalize();
        const gp_Pnt origin = points[0].Translated(
            across * (Symmetric.getValue() ? -width / 2.0 : 0.0)
        );
        const gp_Dir up(first.Crossed(across));
        gp_Trsf placement;
        placement.SetTransformation(gp_Ax3(origin, up, gp_Dir(first)));
        const gp_Trsf toFlat = placement;
        placement.Invert();
        BRepBuilderAPI_MakePolygon outline(
            gp_Pnt(0, 0, 0),
            gp_Pnt(firstLength, 0, 0),
            gp_Pnt(firstLength, width, 0),
            gp_Pnt(0, width, 0),
            Standard_True
        );
        const TopoDS_Face face
            = BRepBuilderAPI_MakeFace(gp_Pln(gp::XOY()), outline.Wire(), Standard_True).Face();
        auto result = std::make_shared<SheetModel>(
            SheetModel::makeBase(face, placement, params, getNameInDocument())
        );
        if (points.size() > 2) {
            EdgeRef ref;
            ref.panel = 0;
            ref.a = gp_Pnt2d(firstLength, 0.0);
            ref.b = gp_Pnt2d(firstLength, width);
            std::vector<gp_Pnt> section;
            for (std::size_t i = 1; i < points.size(); ++i) {
                gp_Pnt p = points[i].Transformed(toFlat);
                p.SetY(0.0);
                section.push_back(p);
            }
            FlangeOptions options;
            options.autoMiter = false;
            result->addContour(ref, section, options, std::string(getNameInDocument()) + "_C");
        }
        return result;
    }
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
    if (lineNumber == 0) {
        throw Base::ValueError("Sheet design: the bend line object has no line");
    }
    if (lines.empty()) {
        // All scores have the angle zero: the part is the flat blank.
        return std::make_shared<SheetModel>(
            SheetModel::makeBase(face, placement, params, getNameInDocument())
        );
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
    // A sharp fold gets a bend with a radius, thus the volume is then different on purpose.
    if (result->sharpFolds == 0
        && std::fabs(source.Mass() - made.Mass()) > 1.0e-3 * std::fabs(source.Mass())) {
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
    ADD_PROPERTY_TYPE(
        Profile,
        (nullptr),
        "Sheet Design",
        App::Prop_None,
        "Optional sketch. A closed outline in the plane of the panel, with one side on the edge, "
        "gives the shape of the flange. An open line of segments in a plane at 90 degrees to the "
        "edge gives one bend and one panel for each segment"
    );
    Profile.setScope(App::LinkScope::Global);
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
    ADD_PROPERTY_TYPE(
        CornerType,
        (0L),
        "Sheet Design",
        App::Prop_None,
        "The corner between two flanges of this feature on adjacent edges, for bends of 90 "
        "degrees. Open: no change. Butt: the two flanges meet at their inner edges. Overlap: the "
        "first flange covers the end of the second. Underlap: the second covers the first"
    );
    CornerType.setEnums(CornerTypeEnums);
    ADD_PROPERTY_TYPE(
        CornerGap,
        (0.0),
        "Sheet Design",
        App::Prop_None,
        "Gap between the two flanges of a closed corner"
    );
    ADD_PROPERTY_TYPE(
        Extend1,
        (0.0),
        "Sheet Design",
        App::Prop_None,
        "Makes the panel of the flange wider than its bend at the start of the edge"
    );
    ADD_PROPERTY_TYPE(
        Extend2,
        (0.0),
        "Sheet Design",
        App::Prop_None,
        "Makes the panel of the flange wider than its bend at the end of the edge"
    );
    ADD_PROPERTY_TYPE(
        UseRuleRadius,
        (true),
        "Sheet Design",
        App::Prop_None,
        "Use the bend radius of the sheet rule"
    );
    ADD_PROPERTY_TYPE(
        Radius,
        (1.0),
        "Sheet Design",
        App::Prop_None,
        "Bend radius of this flange, if the rule radius is not used"
    );
    ADD_PROPERTY_TYPE(
        ReliefType,
        (0L),
        "Sheet Design",
        App::Prop_None,
        "A cut into the panel at each end of the bend that has a gap. It prevents a tear. "
        "Rule: the relief of the sheet rule"
    );
    ReliefType.setEnums(ReliefTypeEnums);
    ADD_PROPERTY_TYPE(
        ReliefWidth,
        (1.0),
        "Sheet Design",
        App::Prop_None,
        "Width of the relief along the edge. It is not larger than the gap"
    );
    ADD_PROPERTY_TYPE(
        ReliefDepth,
        (1.0),
        "Sheet Design",
        App::Prop_None,
        "Depth of the relief, from the bend line into the panel"
    );
    ADD_PROPERTY_TYPE(
        Miter1,
        (0.0),
        "Sheet Design",
        App::Prop_None,
        "Angle of the side of the flange at the start of the edge. A positive value makes the "
        "flange narrower to its far end"
    );
    Miter1.setConstraints(&miterRange);
    ADD_PROPERTY_TYPE(
        Miter2,
        (0.0),
        "Sheet Design",
        App::Prop_None,
        "Angle of the side of the flange at the end of the edge"
    );
    Miter2.setConstraints(&miterRange);
    ADD_PROPERTY_TYPE(
        AutoMiter,
        (true),
        "Sheet Design",
        App::Prop_None,
        "Cut this flange and a flange on an adjacent edge on the line between them, if the "
        "two overlap in the flat pattern"
    );
    ADD_PROPERTY_TYPE(
        MiterGap,
        (0.0),
        "Sheet Design",
        App::Prop_None,
        "Distance between two flanges after an automatic miter"
    );
}

std::shared_ptr<SheetModel> Flange::buildModel()
{
    const std::vector<std::string>& subs = Edge.getSubValues();
    if (!Edge.getValue() || subs.empty()) {
        throw Base::ValueError("Sheet design: select one edge of a sheet design feature");
    }
    if (subs.size() > 1 && Profile.getValue()) {
        throw Base::ValueError("Sheet design: a flange with a profile can have only one edge");
    }
    // This gives the error for a source that has a solid but no sheet model.
    sourceModel(Edge.getValue());
    auto* source = freecad_cast<Feature*>(Edge.getValue());
    if (!source) {
        throw Base::ValueError("Sheet design: select one edge of a sheet design feature");
    }
    auto result = std::make_shared<SheetModel>(*sourceModel(source));

    // All edges refer to the source. The flat space does not change when a flange is added.
    std::vector<EdgeRef> refs;
    for (const std::string& name : subs) {
        EdgeRef found;
        if (!source->findEdge(name.c_str(), found)) {
            throw Base::ValueError(
                "Sheet design: the selection is not a straight edge of a flat panel"
            );
        }
        refs.push_back(found);
    }
    const EdgeRef& ref = refs.front();

    const bool up = ref.top != Reversed.getValue();
    // The position moves the bend line against the edge. Offset adds to it.
    const double t = result->params.thickness;
    const double r = UseRuleRadius.getValue() ? result->params.radius : Radius.getValue();
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
    FlangeOptions options;
    options.gap1 = Gap1.getValue();
    options.gap2 = Gap2.getValue();
    options.offset = shift;
    if (ReliefType.getValue() == 0) {
        options.relief = result->params.relief;  // The relief of the sheet rule.
    }
    else {
        options.relief.type = static_cast<Relief::Type>(ReliefType.getValue() - 1);
        options.relief.width = ReliefWidth.getValue();
        options.relief.depth = ReliefDepth.getValue();
    }
    options.miter1 = Base::toRadians(Miter1.getValue());
    options.miter2 = Base::toRadians(Miter2.getValue());
    options.autoMiter = AutoMiter.getValue();
    options.miterGap = MiterGap.getValue();
    options.extend1 = Extend1.getValue();
    options.extend2 = Extend2.getValue();
    if (!UseRuleRadius.getValue()) {
        options.radius = r;
    }
    if (!Profile.getValue()) {
        // A closed corner: the panels of two flanges on adjacent edges become wider, until they
        // meet. The distance from an edge to the outer side of its flange gives the sizes.
        std::vector<std::array<double, 2>> closure(refs.size(), {0.0, 0.0});
        const double outer = shift + r + t;
        const long corner = CornerType.getValue();
        if (corner != 0 && std::fabs(angle - M_PI / 2.0) < 1.0e-6 && outer > 1.0e-6) {
            const double gap = CornerGap.getValue();
            for (std::size_t i = 0; i < refs.size(); ++i) {
                for (std::size_t j = i + 1; j < refs.size(); ++j) {
                    if (refs[i].panel != refs[j].panel) {
                        continue;
                    }
                    for (int endI = 0; endI < 2; ++endI) {
                        for (int endJ = 0; endJ < 2; ++endJ) {
                            const gp_Pnt2d& pointI = endI == 0 ? refs[i].a : refs[i].b;
                            const gp_Pnt2d& pointJ = endJ == 0 ? refs[j].a : refs[j].b;
                            if (pointI.Distance(pointJ) > 1.0e-6) {
                                continue;
                            }
                            // The direction of each edge to the corner, and on from it.
                            gp_Vec2d onI(endI == 0 ? refs[i].b : refs[i].a, pointI);
                            gp_Vec2d onJ(endJ == 0 ? refs[j].b : refs[j].a, pointJ);
                            onI.Normalize();
                            onJ.Normalize();
                            // An outer corner of 90 degrees: each edge goes on in the direction
                            // out of the other edge.
                            if (onI.Dot(result->edgeNormal(refs[j])) < 1.0 - 1.0e-6
                                || onJ.Dot(result->edgeNormal(refs[i])) < 1.0 - 1.0e-6) {
                                continue;
                            }
                            const double gapI = endI == 0 ? options.gap1 : options.gap2;
                            const double gapJ = endJ == 0 ? options.gap1 : options.gap2;
                            if (gapI > 1.0e-9 || gapJ > 1.0e-9) {
                                continue;
                            }
                            double first = outer - t - gap / 2.0;  // Butt
                            double second = first;
                            if (corner == 2) {  // Overlap
                                first = outer;
                                second = outer - t - gap;
                            }
                            else if (corner == 3) {  // Underlap
                                first = outer - t - gap;
                                second = outer;
                            }
                            closure[i][endI] = first;
                            closure[j][endJ] = second;
                        }
                    }
                }
            }
        }
        for (std::size_t i = 0; i < refs.size(); ++i) {
            FlangeOptions own = options;
            own.extend1 += closure[i][0];
            own.extend2 += closure[i][1];
            const std::string tag = std::string(getNameInDocument())
                + (i > 0 ? "_" + std::to_string(i + 1) : std::string());
            result->addFlange(refs[i], leg, angle, refs[i].top != Reversed.getValue(), own, tag);
        }
        return result;
    }

    // A sketch gives the flange. Move it into the flat space of the panel of the edge.
    const TopoDS_Shape global = profileShape(Profile.getValue(), toLocal(Profile.getValue()));
    BRepBuilderAPI_Transform toPanel(global, result->panels[ref.panel].folded.Inverted(), Standard_True);
    const TopoDS_Shape local = toPanel.Shape();
    TopExp_Explorer wires(local, TopAbs_WIRE);
    const bool closed = TopExp_Explorer(local, TopAbs_FACE).More()
        || (wires.More() && BRep_Tool::IsClosed(TopoDS::Wire(wires.Current())));
    if (closed) {
        // The shape of the flange: an outline in the plane of the panel.
        const std::vector<TopoDS_Face> faces = collectFaces(local);
        Bnd_Box box;
        BRepBndLib::Add(local, box);
        double xmin, ymin, zmin, xmax, ymax, zmax;
        box.Get(xmin, ymin, zmin, xmax, ymax, zmax);
        if (faces.size() != 1 || zmax - zmin > 1.0e-4 || zmin < -1.0e-4 || zmax > t + 1.0e-4) {
            throw Base::ValueError(
                "Sheet design: the profile of the flange must be one closed outline in the plane "
                "of "
                "the panel"
            );
        }
        gp_Trsf drop;
        drop.SetTranslation(gp_Vec(0.0, 0.0, -(zmin + zmax) / 2.0));
        options.profile = TopoDS::Face(
            BRepBuilderAPI_Transform(faces.front(), drop, Standard_True).Shape()
        );
        result->addFlange(ref, leg, angle, up, options, getNameInDocument());
        return result;
    }
    if (!wires.More()) {
        throw Base::ValueError("Sheet design: the profile of the flange has no line");
    }
    const std::vector<gp_Pnt> section = sectionPoints(TopoDS::Wire(wires.Current()));
    result->addContour(ref, section, options, getNameInDocument());

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
        "Folded: the profile is on a panel. Flat: the profile is on the flat pattern. Wrapped: "
        "the profile is on a panel and goes on into the bends and panels beside it"
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
    result->cut(makeCompound(faces), Space.getValue() == 1, getNameInDocument(), Space.getValue() == 2);
    return result;
}


// ------------------------------------------------------------------------------------------------

PROPERTY_SOURCE(SheetDesign::Extend, SheetDesign::Feature)

Extend::Extend()
{
    ADD_PROPERTY_TYPE(Edge, (nullptr), "Sheet Design", App::Prop_None, "Straight edge of a panel");
    ADD_PROPERTY_TYPE(
        Profile,
        (nullptr),
        "Sheet Design",
        App::Prop_None,
        "Optional closed outline in the plane of a panel. It is added to the panel that it touches"
    );
    ADD_PROPERTY_TYPE(
        Length,
        (10.0),
        "Sheet Design",
        App::Prop_None,
        "Distance that the edge moves out. A negative value moves it in"
    );
    ADD_PROPERTY_TYPE(Gap1, (0.0), "Sheet Design", App::Prop_None, "Gap at the start of the edge");
    ADD_PROPERTY_TYPE(Gap2, (0.0), "Sheet Design", App::Prop_None, "Gap at the end of the edge");
    Profile.setScope(App::LinkScope::Global);
}

std::shared_ptr<SheetModel> Extend::buildModel()
{
    auto* source = freecad_cast<Feature*>(Edge.getValue());
    auto result = std::make_shared<SheetModel>(*sourceModel(Edge.getValue()));
    if (Profile.getValue()) {
        const std::vector<TopoDS_Face> faces = collectFaces(
            profileShape(Profile.getValue(), toLocal(Profile.getValue()))
        );
        if (faces.empty()) {
            throw Base::ValueError("Sheet design: the profile of the extension has no closed outline");
        }
        result->extendBy(makeCompound(faces), getNameInDocument());
        return result;
    }
    const std::vector<std::string>& subs = Edge.getSubValues();
    EdgeRef ref;
    if (subs.size() != 1 || !source->findEdge(subs.front().c_str(), ref)) {
        throw Base::ValueError("Sheet design: the selection is not a straight edge of a flat panel");
    }
    result->extend(ref, Length.getValue(), Gap1.getValue(), Gap2.getValue(), getNameInDocument());
    return result;
}


// ------------------------------------------------------------------------------------------------

PROPERTY_SOURCE(SheetDesign::Fold, SheetDesign::Feature)

Fold::Fold()
{
    ADD_PROPERTY_TYPE(Source, (nullptr), "Sheet Design", App::Prop_None, "Feature to fold");
    ADD_PROPERTY_TYPE(
        BendLines,
        (nullptr),
        "Sheet Design",
        App::Prop_None,
        "Straight lines on panels. Each line is the center line of a bend"
    );
    ADD_PROPERTY_TYPE(Angle, (90.0), "Sheet Design", App::Prop_None, "Bend angle");
    ADD_PROPERTY_TYPE(Reversed, (false), "Sheet Design", App::Prop_None, "Bend to the bottom side");
    Angle.setConstraints(&angleRange);
    BendLines.setScope(App::LinkScope::Global);
}

std::shared_ptr<SheetModel> Fold::buildModel()
{
    auto result = std::make_shared<SheetModel>(*sourceModel(Source.getValue()));
    const TopoDS_Shape shape = profileShape(BendLines.getValue(), toLocal(BendLines.getValue()));
    // Move all lines into flat space first. A fold moves panels, but not the flat space.
    std::vector<std::pair<gp_Pnt2d, gp_Pnt2d>> lines;
    TopTools_IndexedMapOfShape edges;
    TopExp::MapShapes(shape, TopAbs_EDGE, edges);
    for (int i = 1; i <= edges.Extent(); ++i) {
        BRepAdaptor_Curve curve(TopoDS::Edge(edges(i)));
        if (curve.GetType() != GeomAbs_Line) {
            throw Base::ValueError("Sheet design: a bend line must be straight");
        }
        gp_Pnt2d a;
        gp_Pnt2d b;
        if (
            !result->toFlat(curve.Value(curve.FirstParameter()), curve.Value(curve.LastParameter()), a, b)
        ) {
            throw Base::ValueError("Sheet design: a bend line is not on a panel");
        }
        lines.emplace_back(a, b);
    }
    if (lines.empty()) {
        throw Base::ValueError("Sheet design: the fold has no bend line");
    }
    int number = 0;
    for (const auto& line : lines) {
        const std::string tag = std::string(getNameInDocument())
            + (++number > 1 ? "_L" + std::to_string(number) : std::string());
        result->fold(line.first, line.second, Base::toRadians(Angle.getValue()), !Reversed.getValue(), tag);
    }
    return result;
}


// ------------------------------------------------------------------------------------------------

PROPERTY_SOURCE(SheetDesign::Hem, SheetDesign::Feature)

Hem::Hem()
{
    ADD_PROPERTY_TYPE(Edge, (nullptr), "Sheet Design", App::Prop_None, "Straight edge of a panel");
    ADD_PROPERTY_TYPE(
        Type,
        (0L),
        "Sheet Design",
        App::Prop_None,
        "Flat: the hem is on the sheet. Open: the hem is parallel to the sheet with a gap. "
        "Teardrop: the end of the hem comes back to the sheet. Rolled: a curl with no flat part"
    );
    ADD_PROPERTY_TYPE(Length, (8.0), "Sheet Design", App::Prop_None, "Length of the flat part of the hem");
    ADD_PROPERTY_TYPE(
        Gap,
        (2.0),
        "Sheet Design",
        App::Prop_None,
        "Distance between the sheet and the hem at the bend. For a rolled hem, the inner diameter"
    );
    ADD_PROPERTY_TYPE(Reversed, (false), "Sheet Design", App::Prop_None, "Fold to the opposite side");
    Type.setEnums(HemEnums);
}

std::shared_ptr<SheetModel> Hem::buildModel()
{
    const std::vector<std::string>& subs = Edge.getSubValues();
    auto* source = freecad_cast<Feature*>(Edge.getValue());
    auto result = std::make_shared<SheetModel>(*sourceModel(Edge.getValue()));
    EdgeRef ref;
    if (subs.size() != 1 || !source->findEdge(subs.front().c_str(), ref)) {
        throw Base::ValueError("Sheet design: the selection is not a straight edge of a flat panel");
    }
    const long type = Type.getValue();
    const double gap = Gap.getValue();
    double length = Length.getValue();
    if (type != 0 && gap <= 1.0e-6) {
        throw Base::ValueError("Sheet design: the gap of this hem type must be larger than zero");
    }
    FlangeOptions options;
    options.autoMiter = false;
    options.anyAngle = true;
    options.radius = type == 0 ? 0.0 : gap / 2.0;
    double angle = M_PI;
    if (type == 2) {
        // Teardrop: the end of the hem comes down to the middle of the gap.
        if (length <= 1.0e-6) {
            throw Base::ValueError("Sheet design: the length of the hem must be larger than zero");
        }
        angle += std::asin(std::min(1.0, gap / (2.0 * length)));
    }
    else if (type == 3) {
        // Rolled: three quarters of a circle. The model needs a panel after each bend, thus the
        // curl ends with a very short flat part.
        angle = 1.5 * M_PI;
        length = result->params.thickness / 100.0;
    }
    result->addFlange(ref, length, angle, ref.top != Reversed.getValue(), options, getNameInDocument());
    return result;
}


// ------------------------------------------------------------------------------------------------

PROPERTY_SOURCE(SheetDesign::CornerRelief, SheetDesign::Feature)

CornerRelief::CornerRelief()
{
    ADD_PROPERTY_TYPE(Source, (nullptr), "Sheet Design", App::Prop_None, "Feature to cut");
    ADD_PROPERTY_TYPE(Type, (0L), "Sheet Design", App::Prop_None, "Shape of the relief");
    ADD_PROPERTY_TYPE(
        Size,
        (4.0),
        "Sheet Design",
        App::Prop_None,
        "Diameter of the circle or side of the square. A corner gets a relief if the two bends "
        "end not farther than this from the point where their bend lines meet"
    );
    Type.setEnums(CornerEnums);
}

std::shared_ptr<SheetModel> CornerRelief::buildModel()
{
    auto result = std::make_shared<SheetModel>(*sourceModel(Source.getValue()));
    result->cornerRelief(Type.getValue() == 1, Size.getValue(), getNameInDocument());
    return result;
}


// ------------------------------------------------------------------------------------------------

PROPERTY_SOURCE(SheetDesign::BaseShape, SheetDesign::Feature)

BaseShape::BaseShape()
{
    ADD_PROPERTY_TYPE(
        Type,
        (0L),
        "Sheet Design",
        App::Prop_None,
        "Plate: one panel. Angle: one wall. Channel: two walls. Tub: four walls. Hat: a channel "
        "with a flange on each wall. Box: a tub with a cover"
    );
    ADD_PROPERTY_TYPE(Length, (100.0), "Sheet Design", App::Prop_None, "Length of the bottom panel");
    ADD_PROPERTY_TYPE(Width, (60.0), "Sheet Design", App::Prop_None, "Width of the bottom panel");
    ADD_PROPERTY_TYPE(Height, (30.0), "Sheet Design", App::Prop_None, "Flat length of each wall");
    ADD_PROPERTY_TYPE(
        FlangeWidth,
        (15.0),
        "Sheet Design",
        App::Prop_None,
        "Flat length of each flange of a hat"
    );
    ADD_PROPERTY_TYPE(
        Rule,
        (nullptr),
        "Sheet Design",
        App::Prop_None,
        "Object with the properties Thickness, BendRadius, and KFactor"
    );
    Type.setEnums(BaseShapeEnums);
    Rule.setScope(App::LinkScope::Global);
}

std::shared_ptr<SheetModel> BaseShape::buildModel()
{
    const SheetParams params = readRule(Rule.getValue());
    const double length = Length.getValue();
    const double width = Width.getValue();
    if (length <= 1.0e-6 || width <= 1.0e-6) {
        throw Base::ValueError("Sheet design: the length and the width must be larger than zero");
    }
    const std::string name = getNameInDocument();
    const gp_Pnt2d corners[4]
        = {gp_Pnt2d(0, 0), gp_Pnt2d(length, 0), gp_Pnt2d(length, width), gp_Pnt2d(0, width)};
    BRepBuilderAPI_MakePolygon outline;
    for (const gp_Pnt2d& corner : corners) {
        outline.Add(gp_Pnt(corner.X(), corner.Y(), 0.0));
    }
    outline.Close();
    const TopoDS_Face face
        = BRepBuilderAPI_MakeFace(gp_Pln(gp::XOY()), outline.Wire(), Standard_True).Face();
    auto result = std::make_shared<SheetModel>(SheetModel::makeBase(face, gp_Trsf(), params, name));

    // The walls, in this sequence: the two short sides, then the two long sides.
    const long type = Type.getValue();
    const int sides[4] = {1, 3, 0, 2};
    const int wallCount = type == 0 ? 0 : type == 1 ? 1 : (type == 2 || type == 4) ? 2 : 4;
    FlangeOptions options;
    options.autoMiter = false;
    std::vector<int> walls;
    for (int i = 0; i < wallCount; ++i) {
        EdgeRef ref;
        ref.panel = 0;
        ref.a = corners[sides[i]];
        ref.b = corners[(sides[i] + 1) % 4];
        ref.top = true;
        walls.push_back(result->addFlange(
            ref,
            Height.getValue(),
            M_PI / 2.0,
            true,
            options,
            name + "_Wall" + std::to_string(i + 1)
        ));
    }
    auto farEdge = [&](int bend) {
        EdgeRef ref;
        ref.panel = result->bends[bend].childPanel;
        ref.a = result->bends[bend].farStart;
        ref.b = result->bends[bend].farEnd;
        ref.top = true;
        return ref;
    };
    if (type == 4) {
        for (std::size_t i = 0; i < walls.size(); ++i) {
            result->addFlange(
                farEdge(walls[i]),
                FlangeWidth.getValue(),
                M_PI / 2.0,
                false,
                options,
                name + "_Flange" + std::to_string(i + 1)
            );
        }
    }
    else if (type == 5) {
        result->addFlange(farEdge(walls[0]), length, M_PI / 2.0, true, options, name + "_Cover");
    }
    return result;
}


// ------------------------------------------------------------------------------------------------

PROPERTY_SOURCE(SheetDesign::Convert, SheetDesign::Feature)

Convert::Convert()
{
    ADD_PROPERTY_TYPE(
        Faces,
        (nullptr),
        "Sheet Design",
        App::Prop_None,
        "Flat faces of a solid that become panels. The first face stays in its position"
    );
    ADD_PROPERTY_TYPE(
        Rule,
        (nullptr),
        "Sheet Design",
        App::Prop_None,
        "Object with the properties Thickness, BendRadius, and KFactor"
    );
    ADD_PROPERTY_TYPE(
        Inside,
        (true),
        "Sheet Design",
        App::Prop_None,
        "The sheet is in the solid, thus the part has the outer sizes of the solid"
    );
    ADD_PROPERTY_TYPE(
        RipGap,
        (0.0),
        "Sheet Design",
        App::Prop_None,
        "Distance between two panels on an edge that has no bend"
    );
    Faces.setScope(App::LinkScope::Global);
    Rule.setScope(App::LinkScope::Global);
}

std::shared_ptr<SheetModel> Convert::buildModel()
{
    const SheetParams params = readRule(Rule.getValue());
    const std::vector<std::string>& subs = Faces.getSubValues();
    if (!Faces.getValue() || subs.empty()) {
        throw Base::ValueError("Sheet design: select the faces of the solid that become panels");
    }
    const TopoDS_Shape solid = profileShape(Faces.getValue(), toLocal(Faces.getValue()));
    std::vector<TopoDS_Face> faces;
    for (const std::string& name : subs) {
        const TopoDS_Shape sub = Part::TopoShape(solid).getSubShape(name.c_str(), true);
        if (sub.IsNull() || sub.ShapeType() != TopAbs_FACE) {
            throw Base::ValueError("Sheet design: the selection must have only faces");
        }
        faces.push_back(TopoDS::Face(sub));
    }
    return std::make_shared<SheetModel>(
        SheetModel::fromFaces(solid, faces, params, Inside.getValue(), RipGap.getValue(), getNameInDocument())
    );
}


// ------------------------------------------------------------------------------------------------

PROPERTY_SOURCE(SheetDesign::Form, SheetDesign::Feature)

Form::Form()
{
    ADD_PROPERTY_TYPE(Source, (nullptr), "Sheet Design", App::Prop_None, "Feature to form");
    ADD_PROPERTY_TYPE(
        Tool,
        (nullptr),
        "Sheet Design",
        App::Prop_None,
        "Solid of the forming tool. The first face is the base face on the sheet. More faces "
        "give openings in the formed sheet"
    );
    Tool.setScope(App::LinkScope::Global);
}

std::shared_ptr<SheetModel> Form::buildModel()
{
    auto result = std::make_shared<SheetModel>(*sourceModel(Source.getValue()));
    const std::vector<std::string>& subs = Tool.getSubValues();
    if (!Tool.getValue() || subs.empty()) {
        throw Base::ValueError("Sheet design: select the base face of the forming tool");
    }
    const TopoDS_Shape tool = profileShape(Tool.getValue(), toLocal(Tool.getValue()));
    std::vector<TopoDS_Face> faces;
    for (const std::string& name : subs) {
        const TopoDS_Shape sub = Part::TopoShape(tool).getSubShape(name.c_str(), true);
        if (sub.IsNull() || sub.ShapeType() != TopAbs_FACE) {
            throw Base::ValueError(
                "Sheet design: the selection on the forming tool must have only faces"
            );
        }
        faces.push_back(TopoDS::Face(sub));
    }
    const std::vector<TopoDS_Face> open(faces.begin() + 1, faces.end());
    result->addForm(tool, faces.front(), open, getNameInDocument());
    return result;
}


// ------------------------------------------------------------------------------------------------

PROPERTY_SOURCE(SheetDesign::Perforation, SheetDesign::Feature)

Perforation::Perforation()
{
    ADD_PROPERTY_TYPE(
        Edges,
        (nullptr),
        "Sheet Design",
        App::Prop_None,
        "Feature to cut, with edges on the bends to cut. With no edge, all bends are cut"
    );
    ADD_PROPERTY_TYPE(
        CutLength,
        (10.0),
        "Sheet Design",
        App::Prop_None,
        "Length of each cut along the bend line"
    );
    ADD_PROPERTY_TYPE(
        TabLength,
        (3.0),
        "Sheet Design",
        App::Prop_None,
        "Length of the material between two cuts"
    );
    ADD_PROPERTY_TYPE(
        Width,
        (0.0),
        "Sheet Design",
        App::Prop_None,
        "Size of each cut across the bend. Zero: the width of the bend zone"
    );
}

std::shared_ptr<SheetModel> Perforation::buildModel()
{
    auto* source = freecad_cast<Feature*>(Edges.getValue());
    auto result = std::make_shared<SheetModel>(*sourceModel(Edges.getValue()));
    std::vector<int> bends;
    for (const std::string& name : Edges.getSubValues()) {
        EdgeRef ref;
        const int bend = source->findEdge(name.c_str(), ref) ? result->bendAt(ref) : -1;
        if (bend < 0) {
            throw Base::ValueError("Sheet design: a selected edge is not on a bend");
        }
        if (std::find(bends.begin(), bends.end(), bend) == bends.end()) {
            bends.push_back(bend);
        }
    }
    result->perforate(
        bends,
        CutLength.getValue(),
        TabLength.getValue(),
        Width.getValue(),
        getNameInDocument()
    );
    return result;
}


// ------------------------------------------------------------------------------------------------

PROPERTY_SOURCE(SheetDesign::FlatPattern, Part::Feature)

FlatPattern::FlatPattern()
{
    ADD_PROPERTY_TYPE(Source, (nullptr), "Sheet Design", App::Prop_None, "Sheet design feature");
    ADD_PROPERTY_TYPE(BendLines, (true), "Sheet Design", App::Prop_None, "Add the bend center lines");
    const auto output = static_cast<App::PropertyType>(App::Prop_ReadOnly | App::Prop_Output);
    ADD_PROPERTY_TYPE(
        BendAngles,
        (0.0),
        "Sheet Design",
        output,
        "Angle of each bend in degrees, in the sequence of the bend lines. Positive: the bend "
        "goes up. Negative: it goes down"
    );
    ADD_PROPERTY_TYPE(BendRadii, (0.0), "Sheet Design", output, "Inner radius of each bend");
    ADD_PROPERTY_TYPE(
        BendOrder,
        (0L),
        "Sheet Design",
        App::Prop_None,
        "Number of each bend in the bend sequence, in the sequence of the bend lines. Empty: "
        "the numbers 1, 2, 3"
    );
    BendOrder.setValues(std::vector<long>());
    BendAngles.setValues(std::vector<double>());
    BendRadii.setValues(std::vector<double>());
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
        // The outlines of the formed regions: a marker for the export, not a cut.
        const std::vector<TopoDS_Wire> outlines = sheet->formOutlines();
        if (!outlines.empty()) {
            TopoDS_Compound marks;
            builder.MakeCompound(marks);
            for (const TopoDS_Wire& outline : outlines) {
                builder.Add(marks, outline.Moved(root));
            }
            builder.Add(compound, marks);
        }
        Shape.setValue(namedShape(compound, names, getID()));
        std::vector<double> angles;
        std::vector<double> radii;
        for (const Bend& bend : sheet->bends) {
            angles.push_back((bend.up ? 1.0 : -1.0) * Base::toDegrees(bend.angle));
            radii.push_back(bend.radius);
        }
        BendAngles.setValues(angles);
        BendRadii.setValues(radii);
    }
    catch (const Standard_Failure& e) {
        return failure(e.GetMessageString());
    }
    catch (const Base::Exception& e) {
        return failure(e.what());
    }
    return App::DocumentObject::StdReturn;
}
