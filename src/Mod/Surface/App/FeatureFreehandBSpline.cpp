// SPDX-License-Identifier: LGPL-2.1-or-later

#include <cmath>
#include <algorithm>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRep_Tool.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <TopoDS.hxx>
#include <TopExp_Explorer.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <App/GeoFeature.h>
#include <App/Document.h>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <GeomAPI_Interpolate.hxx>
#include <GeomConvert.hxx>
#include <GeomConvert_CompCurveToBSplineCurve.hxx>
#include <Geom_BezierCurve.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <Precision.hxx>
#include <TColgp_HArray1OfPnt.hxx>
#include <TColgp_Array1OfVec.hxx>
#include <TColStd_HArray1OfReal.hxx>
#include <TColStd_HArray1OfBoolean.hxx>
#include <Base/Exception.h>

#include "FeatureFreehandBSpline.h"

using namespace Surface;
PROPERTY_SOURCE(Surface::FreehandBSpline, Part::Spline)

FreehandBSpline::FreehandBSpline()
{
    ADD_PROPERTY_TYPE(
        Points,
        (Base::Vector3d()),
        "Curve",
        App::Prop_None,
        "Interpolation points in local coordinates"
    );
    ADD_PROPERTY_TYPE(
        Support,
        (nullptr),
        "Attachment",
        App::Prop_None,
        "Vertex, edge or face supporting each locked point"
    );
    ADD_PROPERTY_TYPE(
        SupportPointIndices,
        (0),
        "Attachment",
        App::Prop_ReadOnly,
        "Point indices corresponding to Support"
    );
    ADD_PROPERTY_TYPE(
        SupportParameters,
        (Base::Vector3d()),
        "Attachment",
        App::Prop_ReadOnly,
        "Edge fractions or face UV coordinates of locked points"
    );
    ADD_PROPERTY_TYPE(
        TangentSupport,
        (nullptr),
        "Attachment",
        App::Prop_None,
        "Edge or face tangent reference for each constrained point"
    );
    ADD_PROPERTY_TYPE(
        TangentPointIndices,
        (0),
        "Attachment",
        App::Prop_ReadOnly,
        "Point indices corresponding to TangentSupport"
    );
    TangentSupport.setScope(App::LinkScope::Global);
    TangentPointIndices.setValues(std::vector<long>());
    Support.setScope(App::LinkScope::Global);
    SupportPointIndices.setValues(std::vector<long>());
    SupportParameters.setValues(std::vector<Base::Vector3d>());
    ADD_PROPERTY_TYPE(Periodic, (false), "Curve", App::Prop_None, "Close the curve smoothly");
    ADD_PROPERTY_TYPE(
        Parameterization,
        (0.5),
        "Curve",
        App::Prop_None,
        "0: uniform; 0.5: centripetal; 1: chord length"
    );
    static const App::PropertyFloatConstraint::Constraints range {0.0, 1.0, 0.1};
    Parameterization.setConstraints(&range);
    ADD_PROPERTY_TYPE(
        LinearSegments,
        (false),
        "Curve",
        App::Prop_None,
        "Interpolate marked spans as straight lines"
    );
}

short FreehandBSpline::mustExecute() const
{
    return TangentSupport.isTouched() || TangentPointIndices.isTouched() || Support.isTouched()
            || SupportPointIndices.isTouched() || SupportParameters.isTouched() || Points.isTouched()
            || Periodic.isTouched() || Parameterization.isTouched() || LinearSegments.isTouched()
        ? 1
        : Part::Spline::mustExecute();
}

Handle(Geom_BSplineCurve) FreehandBSpline::interpolate() const
{
    return interpolate(Points.getValues());
}

Handle(Geom_BSplineCurve) FreehandBSpline::interpolate(const std::vector<Base::Vector3d>& points) const
{
    const int count = static_cast<int>(points.size());
    const bool closed = Periodic.getValue();
    if (count < (closed ? 3 : 2)) {
        throw Base::ValueError(
            closed ? "A closed curve needs at least three points." : "A curve needs at least two points."
        );
    }
    const int spans = closed ? count : count - 1;
    Handle(TColgp_HArray1OfPnt) data = new TColgp_HArray1OfPnt(1, count);
    Handle(TColStd_HArray1OfReal) parameters = new TColStd_HArray1OfReal(1, spans + 1);
    parameters->SetValue(1, 0.0);
    for (int i = 0; i < count; ++i) {
        const auto& p = points[i];
        if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z)) {
            throw Base::ValueError("Point coordinates must be finite.");
        }
        data->SetValue(i + 1, gp_Pnt(p.x, p.y, p.z));
    }
    for (int i = 0; i < spans; ++i) {
        const double length = (points[(i + 1) % count] - points[i]).Length();
        if (length <= Precision::Confusion()) {
            throw Base::ValueError("Consecutive interpolation points must be different.");
        }
        parameters->SetValue(
            i + 2,
            parameters->Value(i + 1) + std::pow(length, Parameterization.getValue())
        );
    }
    GeomAPI_Interpolate builder(data, parameters, closed, Precision::Confusion());
    const auto& linear = LinearSegments.getValues();
    TColgp_Array1OfVec tangents(1, count);
    Handle(TColStd_HArray1OfBoolean) flags = new TColStd_HArray1OfBoolean(1, count, false);
    bool anyLinear = false;
    for (int i = 0; i < spans; ++i) {
        if (i < static_cast<int>(linear.size()) && linear[i]) {
            anyLinear = true;
            const int next = (i + 1) % count;
            gp_Vec tangent(data->Value(i + 1), data->Value(next + 1));
            tangents.SetValue(i + 1, tangent);
            tangents.SetValue(next + 1, tangent);
            flags->SetValue(i + 1, true);
            flags->SetValue(next + 1, true);
        }
    }
    const auto& tangentIndices = TangentPointIndices.getValues();
    if (tangentIndices.size() != TangentSupport.getValues().size()) {
        throw Base::ValueError("Inconsistent tangent references.");
    }
    for (size_t j = 0; j < tangentIndices.size(); ++j) {
        const int i = static_cast<int>(tangentIndices[j]);
        if (i < 0 || i >= count) {
            throw Base::ValueError("Invalid tangent point index.");
        }
        const auto& locks = SupportPointIndices.getValues();
        if (std::find(locks.begin(), locks.end(), i) == locks.end()) {
            throw Base::ValueError("A tangent point must be locked to a reference.");
        }
        const auto direction = tangentDirection(
            i,
            points,
            TangentSupport.getValues()[j],
            TangentSupport.getSubValues()[j]
        );
        gp_Vec tangent(direction.x, direction.y, direction.z);
        if (flags->Value(i + 1) && !tangents.Value(i + 1).IsParallel(tangent, 1e-6)) {
            throw Base::ValueError("The tangent conflicts with an adjacent linear segment.");
        }
        tangents.SetValue(i + 1, tangent);
        flags->SetValue(i + 1, true);
    }
    if (count == 2 && tangentIndices.empty()) {
        return GeomConvert::CurveToBSplineCurve(new Geom_BezierCurve(data->Array1()));
    }
    if (anyLinear || !tangentIndices.empty()) {
        builder.Load(tangents, flags, true);
    }
    builder.Perform();
    if (!builder.IsDone()) {
        throw Base::ValueError("B-spline interpolation failed.");
    }
    if (!anyLinear) {
        return builder.Curve();
    }
    GeomConvert_CompCurveToBSplineCurve joined;
    for (int i = 0; i < spans; ++i) {
        Handle(Geom_BoundedCurve) segment;
        if (i < static_cast<int>(linear.size()) && linear[i]) {
            TColgp_Array1OfPnt poles(1, 2);
            poles.SetValue(1, data->Value(i + 1));
            poles.SetValue(2, data->Value((i + 1) % count + 1));
            segment = new Geom_BezierCurve(poles);
        }
        else {
            segment = new Geom_TrimmedCurve(
                builder.Curve(),
                parameters->Value(i + 1),
                parameters->Value(i + 2)
            );
        }
        if (!joined.Add(segment, Precision::Confusion(), true)) {
            throw Base::ValueError("Could not join interpolation spans.");
        }
    }
    return joined.BSplineCurve();
}

namespace
{
TopoDS_Shape supportShape(App::DocumentObject* object, const std::string& name)
{
    if (!object) {
        throw Base::ValueError("A locked point has a missing reference.");
    }
    auto shape = Part::Feature::getShape(
        object,
        Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform
            | Part::ShapeOption::NeedSubElement,
        name.c_str()
    );
    if (!shape.IsNull() && (shape.ShapeType() == TopAbs_WIRE || shape.ShapeType() == TopAbs_COMPOUND)
        && !TopExp_Explorer(shape, TopAbs_FACE).More()) {
        TopExp_Explorer edges(shape, TopAbs_EDGE);
        if (edges.More()) {
            const auto edge = edges.Current();
            edges.Next();
            if (!edges.More()) {
                shape = edge;
            }
        }
    }
    if (shape.IsNull()
        || (shape.ShapeType() != TopAbs_VERTEX && shape.ShapeType() != TopAbs_EDGE
            && shape.ShapeType() != TopAbs_FACE)) {
        throw Base::ValueError("Select a vertex, edge or face for the point reference.");
    }
    return shape;
}
}  // namespace

std::vector<std::string> FreehandBSpline::tangentChoices(int index) const
{
    std::vector<std::string> choices;
    const auto& indices = SupportPointIndices.getValues();
    auto found = std::find(indices.begin(), indices.end(), index);
    if (found == indices.end()) {
        return choices;
    }
    const auto slot = std::distance(indices.begin(), found);
    auto object = Support.getValues()[slot];
    // Resolve both the reference and its neighbours from the same topology, before transforms.
    const auto topology = Part::Feature::getTopoShape(object, Part::ShapeOption::ResolveLink);
    const auto& name = Support.getSubValues()[slot];
    auto whole = topology.getShape();
    auto locked = name.empty() ? whole : topology.getSubShape(name.c_str());
    if (locked.ShapeType() == TopAbs_WIRE || locked.ShapeType() == TopAbs_COMPOUND) {
        TopExp_Explorer edge(locked, TopAbs_EDGE);
        if (edge.More()) {
            locked = edge.Current();
        }
    }
    TopTools_IndexedMapOfShape elements;
    const auto type = locked.ShapeType() == TopAbs_FACE ? TopAbs_FACE : TopAbs_EDGE;
    TopExp::MapShapes(whole, type, elements);
    for (int i = 1; i <= elements.Extent(); ++i) {
        bool match = elements(i).IsSame(locked);
        if (locked.ShapeType() == TopAbs_VERTEX) {
            for (TopExp_Explorer v(elements(i), TopAbs_VERTEX); v.More(); v.Next()) {
                match = match || v.Current().IsSame(locked);
            }
        }
        if (match) {
            choices.push_back(std::string(type == TopAbs_FACE ? "Face" : "Edge") + std::to_string(i));
        }
    }
    return choices;
}

void FreehandBSpline::setPointTangent(int index, const std::string& subname)
{
    auto objects = TangentSupport.getValues();
    auto names = TangentSupport.getSubValues();
    auto indices = TangentPointIndices.getValues();
    App::DocumentObject* object = nullptr;
    if (!subname.empty()) {
        const auto choices = tangentChoices(index);
        if (std::find(choices.begin(), choices.end(), subname) == choices.end()) {
            throw Base::ValueError("Choose an edge or face belonging to the locked reference.");
        }
        const auto& locks = SupportPointIndices.getValues();
        object
            = Support.getValues()[std::distance(locks.begin(), std::find(locks.begin(), locks.end(), index))];
    }
    for (size_t i = 0; i < indices.size(); ++i) {
        if (indices[i] == index) {
            objects.erase(objects.begin() + i);
            names.erase(names.begin() + i);
            indices.erase(indices.begin() + i);
            break;
        }
    }
    if (object) {
        objects.push_back(object);
        names.push_back(subname);
        indices.push_back(index);
    }
    TangentSupport.setValues(objects, names);
    TangentPointIndices.setValues(indices);
}

Base::Vector3d FreehandBSpline::tangentDirection(
    int index,
    const std::vector<Base::Vector3d>& points,
    App::DocumentObject* object,
    const std::string& name
) const
{
    const int count = static_cast<int>(points.size());
    const int previous = index > 0 ? index - 1 : Periodic.getValue() ? count - 1 : 0;
    const int next = index + 1 < count ? index + 1 : Periodic.getValue() ? 0 : count - 1;
    auto direction = points[next] - points[previous];
    const auto placement = App::GeoFeature::getGlobalPlacement(this);
    Base::Vector3d world, forward;
    placement.multVec(points[index], world);
    placement.getRotation().multVec(direction, forward);
    const auto shape = supportShape(object, name);
    gp_Pnt point(world.x, world.y, world.z);
    gp_Vec tangent;
    if (shape.ShapeType() == TopAbs_EDGE) {
        double first, last;
        auto curve = BRep_Tool::Curve(TopoDS::Edge(shape), first, last);
        GeomAPI_ProjectPointOnCurve projection(point, curve, first, last);
        if (!projection.NbPoints()) {
            throw Base::ValueError("Cannot evaluate tangent on the reference edge.");
        }
        curve->D1(projection.LowerDistanceParameter(), point, tangent);
    }
    else if (shape.ShapeType() == TopAbs_FACE) {
        auto surface = BRep_Tool::Surface(TopoDS::Face(shape));
        GeomAPI_ProjectPointOnSurf projection(point, surface);
        if (!projection.IsDone() || !projection.NbPoints()) {
            throw Base::ValueError("Cannot evaluate tangent on the reference face.");
        }
        double u, v;
        projection.LowerDistanceParameters(u, v);
        gp_Vec du, dv;
        surface->D1(u, v, point, du, dv);
        auto normal = du.Crossed(dv);
        if (normal.SquareMagnitude() <= Precision::SquareConfusion()) {
            throw Base::ValueError("The reference face has no defined tangent plane here.");
        }
        normal.Normalize();
        tangent = gp_Vec(forward.x, forward.y, forward.z);
        tangent -= normal * tangent.Dot(normal);
        if (tangent.SquareMagnitude() <= Precision::SquareConfusion()) {
            tangent = du;
        }
    }
    else {
        throw Base::ValueError("A tangent reference must be an edge or face.");
    }
    if (tangent.SquareMagnitude() <= Precision::SquareConfusion()) {
        throw Base::ValueError("The reference has no defined tangent here.");
    }
    tangent.Normalize();
    if (tangent.Dot(gp_Vec(forward.x, forward.y, forward.z)) < 0) {
        tangent.Reverse();
    }
    placement.getRotation().inverse().multVec(
        Base::Vector3d(tangent.X(), tangent.Y(), tangent.Z()),
        direction
    );
    return direction;
}

void FreehandBSpline::onBeforeChange(const App::Property* property)
{
    Part::Spline::onBeforeChange(property);
    if (property != &Support && property != &TangentSupport) {
        return;
    }
    auto& removed = property == &Support ? removedSupportSlots : removedTangentSlots;
    removed.clear();
    if (!getDocument() || getDocument()->isPerformingTransaction()) {
        return;
    }
    const auto& objects = property == &Support ? Support.getValues() : TangentSupport.getValues();
    for (size_t i = 0; i < objects.size(); ++i) {
        if (objects[i] && objects[i]->isRemoving()) {
            removed.push_back(i);
        }
    }
}

void FreehandBSpline::onChanged(const App::Property* property)
{
    if (property == &Support || property == &TangentSupport) {
        auto& removed = property == &Support ? removedSupportSlots : removedTangentSlots;
        auto slots = std::move(removed);
        removed.clear();
        if (!slots.empty()) {
            auto indices = property == &Support ? SupportPointIndices.getValues()
                                                : TangentPointIndices.getValues();
            auto parameters = SupportParameters.getValues();
            for (auto it = slots.rbegin(); it != slots.rend(); ++it) {
                if (*it < indices.size()) {
                    indices.erase(indices.begin() + *it);
                }
                if (property == &Support && *it < parameters.size()) {
                    parameters.erase(parameters.begin() + *it);
                }
            }
            if (property == &Support) {
                SupportPointIndices.setValues(indices);
                SupportParameters.setValues(parameters);
            }
            else {
                TangentPointIndices.setValues(indices);
            }
        }
    }
    Part::Spline::onChanged(property);
}

void FreehandBSpline::sanitizeReferences()
{
    // A saved file from an older version may already have lost the slot mapping.
    // Unlock ambiguous entries rather than attaching a surviving reference to the wrong point.
    const auto clean =
        [this](App::PropertyLinkSubList& links, App::PropertyIntegerList& pointIndices, bool tangent) {
            const auto& oldObjects = links.getValues();
            const auto& oldNames = links.getSubValues();
            const auto& oldIndices = pointIndices.getValues();
            const auto& oldParameters = SupportParameters.getValues();
            std::vector<App::DocumentObject*> objects;
            std::vector<std::string> names;
            std::vector<long> indices;
            std::vector<Base::Vector3d> parameters;
            const bool consistent = oldObjects.size() == oldIndices.size()
                && oldNames.size() == oldIndices.size()
                && (tangent || oldParameters.size() == oldIndices.size());
            if (consistent) {
                for (size_t i = 0; i < oldIndices.size(); ++i) {
                    const auto index = oldIndices[i];
                    if (index < 0 || index >= Points.getSize() || !oldObjects[i]
                        || oldObjects[i]->isRemoving()) {
                        continue;
                    }
                    if (tangent) {
                        const auto& locks = SupportPointIndices.getValues();
                        if (std::find(locks.begin(), locks.end(), index) == locks.end()) {
                            continue;
                        }
                    }
                    try {
                        const auto shape = supportShape(oldObjects[i], oldNames[i]);
                        if (tangent && shape.ShapeType() != TopAbs_EDGE
                            && shape.ShapeType() != TopAbs_FACE) {
                            continue;
                        }
                    }
                    catch (const Base::Exception&) {
                        continue;
                    }
                    catch (const Standard_Failure&) {
                        continue;
                    }
                    objects.push_back(oldObjects[i]);
                    names.push_back(oldNames[i]);
                    indices.push_back(index);
                    if (!tangent) {
                        parameters.push_back(oldParameters[i]);
                    }
                }
            }
            if (!consistent || objects.size() != oldObjects.size()) {
                links.setValues(objects, names);
                pointIndices.setValues(indices);
                if (!tangent) {
                    SupportParameters.setValues(parameters);
                }
            }
        };
    clean(Support, SupportPointIndices, false);
    clean(TangentSupport, TangentPointIndices, true);
}

void FreehandBSpline::setPointSupport(int index, App::DocumentObject* object, const std::string& subname)
{
    sanitizeReferences();
    if (index < 0 || index >= Points.getSize()) {
        throw Base::ValueError("Invalid point index.");
    }
    if (object) {
        const auto dependents = getInListRecursive();
        if (object == this || object->getDocument() != getDocument()
            || std::find(dependents.begin(), dependents.end(), object) != dependents.end()) {
            throw Base::ValueError(
                "The reference would create a circular or cross-document dependency."
            );
        }
        supportShape(object, subname);
    }
    const auto oldObjects = Support.getValues();
    const auto oldNames = Support.getSubValues();
    const auto oldIndices = SupportPointIndices.getValues();
    const auto oldParameters = SupportParameters.getValues();
    auto objects = oldObjects;
    auto names = oldNames;
    auto indices = oldIndices;
    auto parameters = oldParameters;
    if (objects.size() != indices.size() || names.size() != indices.size()
        || parameters.size() != indices.size()) {
        throw Base::ValueError("Inconsistent point references.");
    }
    for (size_t i = 0; i < indices.size(); ++i) {
        if (indices[i] == index) {
            objects.erase(objects.begin() + i);
            names.erase(names.begin() + i);
            indices.erase(indices.begin() + i);
            parameters.erase(parameters.begin() + i);
            break;
        }
    }
    if (object) {
        objects.push_back(object);
        names.push_back(subname);
        indices.push_back(index);
        parameters.emplace_back();
    }
    Support.setValues(objects, names);
    SupportPointIndices.setValues(indices);
    SupportParameters.setValues(parameters);
    try {
        updateSupportedPoints(true);
    }
    catch (...) {
        Support.setValues(oldObjects, oldNames);
        SupportPointIndices.setValues(oldIndices);
        SupportParameters.setValues(oldParameters);
        throw;
    }
    setPointTangent(index, {});
}

void FreehandBSpline::remapSupports(const std::vector<int>& retained)
{
    sanitizeReferences();
    std::vector<App::DocumentObject*> objects;
    std::vector<std::string> names;
    std::vector<long> indices;
    std::vector<Base::Vector3d> parameters;
    const auto oldIndices = SupportPointIndices.getValues();
    for (size_t i = 0; i < oldIndices.size(); ++i) {
        auto found = std::find(retained.begin(), retained.end(), oldIndices[i]);
        if (found != retained.end()) {
            objects.push_back(Support.getValues()[i]);
            names.push_back(Support.getSubValues()[i]);
            indices.push_back(std::distance(retained.begin(), found));
            parameters.push_back(SupportParameters.getValues()[i]);
        }
    }
    Support.setValues(objects, names);
    SupportPointIndices.setValues(indices);
    SupportParameters.setValues(parameters);
    objects.clear();
    names.clear();
    indices.clear();
    const auto& tangentIndices = TangentPointIndices.getValues();
    for (size_t i = 0; i < tangentIndices.size(); ++i) {
        const auto found = std::find(retained.begin(), retained.end(), tangentIndices[i]);
        if (found != retained.end()) {
            objects.push_back(TangentSupport.getValues()[i]);
            names.push_back(TangentSupport.getSubValues()[i]);
            indices.push_back(std::distance(retained.begin(), found));
        }
    }
    TangentSupport.setValues(objects, names);
    TangentPointIndices.setValues(indices);
}

void FreehandBSpline::updateSupportedPoints(bool project)
{
    sanitizeReferences();
    const auto objects = Support.getValues();
    const auto names = Support.getSubValues();
    const auto indices = SupportPointIndices.getValues();
    auto parameters = SupportParameters.getValues();
    auto points = Points.getValues();
    if (objects.size() != indices.size() || names.size() != indices.size()
        || parameters.size() != indices.size()) {
        throw Base::ValueError("Inconsistent point references.");
    }
    const auto placement = App::GeoFeature::getGlobalPlacement(this);
    const auto inverse = placement.inverse();
    for (size_t i = 0; i < indices.size(); ++i) {
        const auto index = indices[i];
        if (index < 0 || index >= static_cast<long>(points.size())) {
            throw Base::ValueError("A point reference has an invalid index.");
        }
        const auto shape = supportShape(objects[i], names[i]);
        gp_Pnt point;
        if (shape.ShapeType() == TopAbs_VERTEX) {
            point = BRep_Tool::Pnt(TopoDS::Vertex(shape));
        }
        else {
            if (project) {
                Base::Vector3d world;
                placement.multVec(points[index], world);
                BRepExtrema_DistShapeShape distance(
                    BRepBuilderAPI_MakeVertex(gp_Pnt(world.x, world.y, world.z)).Vertex(),
                    shape
                );
                if (!distance.IsDone() || distance.NbSolution() == 0) {
                    throw Base::ValueError("Cannot project point onto its reference.");
                }
                point = distance.PointOnShape2(1);
            }
            if (shape.ShapeType() == TopAbs_EDGE) {
                BRepAdaptor_Curve curve(TopoDS::Edge(shape));
                if (!std::isfinite(curve.FirstParameter()) || !std::isfinite(curve.LastParameter())
                    || curve.LastParameter() - curve.FirstParameter() <= Precision::PConfusion()) {
                    throw Base::ValueError(
                        "The reference edge must have a finite, nonzero parameter range."
                    );
                }
                if (project) {
                    double first, last;
                    auto geometry = BRep_Tool::Curve(TopoDS::Edge(shape), first, last);
                    GeomAPI_ProjectPointOnCurve projection(point, geometry, first, last);
                    parameters[i].x = (projection.LowerDistanceParameter() - first) / (last - first);
                }
                point = curve.Value(
                    curve.FirstParameter()
                    + parameters[i].x * (curve.LastParameter() - curve.FirstParameter())
                );
            }
            else {
                auto face = TopoDS::Face(shape);
                if (project) {
                    GeomAPI_ProjectPointOnSurf projection(point, BRep_Tool::Surface(face));
                    if (!projection.IsDone() || projection.NbPoints() == 0) {
                        throw Base::ValueError("Cannot project point onto its reference face.");
                    }
                    projection.LowerDistanceParameters(parameters[i].x, parameters[i].y);
                }
                point = BRepAdaptor_Surface(face).Value(parameters[i].x, parameters[i].y);
                // Keep the point inside the trimmed face if its boundary changes on recompute.
                BRepExtrema_DistShapeShape distance(BRepBuilderAPI_MakeVertex(point).Vertex(), face);
                if (!distance.IsDone() || distance.NbSolution() == 0) {
                    throw Base::ValueError("Cannot locate point on its reference face.");
                }
                point = distance.PointOnShape2(1);
            }
        }
        inverse.multVec(Base::Vector3d(point.X(), point.Y(), point.Z()), points[index]);
    }
    if (!indices.empty()) {
        Points.setValues(points);
        if (project) {
            SupportParameters.setValues(parameters);
        }
    }
}

App::DocumentObjectExecReturn* FreehandBSpline::execute()
{
    try {
        updateSupportedPoints(false);
        Part::TopoShape result(BRepBuilderAPI_MakeEdge(interpolate()).Edge());
        result.setTransform(Placement.getValue().toMatrix());
        Shape.setValue(result);
        return StdReturn;
    }
    catch (const Standard_Failure& error) {
        return new App::DocumentObjectExecReturn(error.GetMessageString());
    }
    catch (const Base::Exception& error) {
        return new App::DocumentObjectExecReturn(error.what());
    }
}
