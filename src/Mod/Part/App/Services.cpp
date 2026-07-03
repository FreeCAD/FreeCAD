// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2024 Kacper Donat <kacper@kadet.net>                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include <BRepGProp.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRep_Tool.hxx>
#include <GProp_GProps.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopExp_Explorer.hxx>

#include <Base/Interpreter.h>
#include <Base/Matrix.h>
#include <Base/Precision.h>
#include <Base/Vector3D.h>

#include <App/Datums.h>
#include <App/Link.h>
#include <App/Part.h>
#include <App/Placement.h>

#include "Geometry.h"
#include "PartFeature.h"
#include "Services.h"
#include "Tools.h"

#include <optional>

AttacherSubObjectPlacement::AttacherSubObjectPlacement()
    : attacher(std::make_unique<Attacher::AttachEngine3D>())
{
    attacher->setUp({}, Attacher::mmMidpoint);
}

Base::Placement AttacherSubObjectPlacement::calculate(
    App::SubObjectT object,
    Base::Placement basePlacement
) const
{
    attacher->setReferences({object});

    auto calculatedAttachment = attacher->calculateAttachedPlacement(basePlacement);

    return basePlacement.inverse() * calculatedAttachment;
}

namespace
{

constexpr double edgeEndpointSnapThreshold = 0.15;

bool isAxisSystemObject(const App::SubObjectT& object)
{
    if (!object.getOldElementName().empty()) {
        return false;
    }

    auto isAxisSystem = [](const App::DocumentObject* obj) {
        return obj
            && (obj->isDerivedFrom<App::LocalCoordinateSystem>()
                || obj->isDerivedFrom<App::Placement>());
    };

    if (isAxisSystem(object.getObject())) {
        return true;
    }

    auto subObjects = object.getSubObjectList();
    return !subObjects.empty() && isAxisSystem(subObjects.back());
}

Base::Vector3d toVector(const gp_Pnt& point)
{
    return Base::Vector3d(point.X(), point.Y(), point.Z());
}

Base::Vector3d toVector(const gp_Dir& direction)
{
    return Base::Vector3d(direction.X(), direction.Y(), direction.Z());
}

Base::Vector3d pointOnAxisNear(const Base::Vector3d& point, const gp_Ax1& axis)
{
    const auto axisOrigin = toVector(axis.Location());
    auto axisDirection = toVector(axis.Direction());
    axisDirection.Normalize();

    return axisOrigin + axisDirection * ((point - axisOrigin) * axisDirection);
}

Base::Rotation rotationWithZAxis(
    const Base::Vector3d& zDirection,
    std::optional<Base::Vector3d> xDirection = std::nullopt
)
{
    auto z = zDirection;
    if (z.Length() < Base::Precision::Confusion()) {
        z = Base::Vector3d::UnitZ;
    }
    z.Normalize();

    auto x = xDirection.value_or(Base::Vector3d::UnitX);
    x = x - z * (x * z);
    if (x.Length() < Base::Precision::Confusion()) {
        x = Base::Vector3d::UnitX - z * (Base::Vector3d::UnitX * z);
    }
    if (x.Length() < Base::Precision::Confusion()) {
        x = Base::Vector3d::UnitY - z * (Base::Vector3d::UnitY * z);
    }
    x.Normalize();

    auto y = z.Cross(x);
    y.Normalize();

    return Base::Rotation::makeRotationByAxes(x, y, z, "ZXY");
}

Part::TopoShape getSubTopoShape(const App::SubObjectT& object)
{
    std::string elementName = object.getOldElementName();
    if (!object.getObject() || elementName.empty()) {
        return {};
    }

    auto getShape = [](const App::DocumentObject* obj, const std::string& subname) {
        if (!obj || subname.empty()) {
            return Part::TopoShape {};
        }

        return Part::Feature::getTopoShape(
            obj,
            Part::ShapeOption::NeedSubElement | Part::ShapeOption::ResolveLink,
            subname.c_str()
        );
    };

    auto tryShape = [&getShape](const App::DocumentObject* obj, const std::string& subname) {
        auto shape = getShape(obj, subname);
        return shape.isNull() ? Part::TopoShape {} : shape;
    };

    const auto subnameNoElement = object.getSubNameNoElement();
    if (auto shape = tryShape(object.getObject(), subnameNoElement + elementName); !shape.isNull()) {
        return shape;
    }

    const auto newElementName = object.getNewElementName();
    if (!newElementName.empty() && newElementName != elementName) {
        if (auto shape = tryShape(object.getObject(), subnameNoElement + newElementName);
            !shape.isNull()) {
            return shape;
        }
    }

    auto normalizedObject = object.normalized(App::SubObjectT::NormalizeOption::KeepSubName);
    if (normalizedObject.getObject()
        && (normalizedObject.getObject() != object.getObject()
            || normalizedObject.getSubName() != object.getSubName())) {
        const auto normalizedSubname = normalizedObject.getSubNameNoElement();
        const auto normalizedElementName = normalizedObject.getOldElementName();
        if (auto shape
            = tryShape(normalizedObject.getObject(), normalizedSubname + normalizedElementName);
            !shape.isNull()) {
            return shape;
        }

        const auto normalizedNewElementName = normalizedObject.getNewElementName();
        if (!normalizedNewElementName.empty() && normalizedNewElementName != normalizedElementName) {
            if (auto shape
                = tryShape(normalizedObject.getObject(), normalizedSubname + normalizedNewElementName);
                !shape.isNull()) {
                return shape;
            }
        }
    }

    auto subObjects = object.getSubObjectList();
    if (!subObjects.empty() && subObjects.back() != object.getObject()) {
        if (auto shape = tryShape(subObjects.back(), elementName); !shape.isNull()) {
            return shape;
        }
        if (!newElementName.empty() && newElementName != elementName) {
            if (auto shape = tryShape(subObjects.back(), newElementName); !shape.isNull()) {
                return shape;
            }
        }
    }

    return {};
}

std::optional<Base::Placement> snapPlacementFromEdge(const TopoDS_Edge& edge)
{
    BRepAdaptor_Curve curve(edge);
    switch (curve.GetType()) {
        case GeomAbs_Line: {
            const auto line = curve.Line();
            return Base::Placement(
                toVector(line.Location()),
                rotationWithZAxis(toVector(line.Direction()))
            );
        }
        case GeomAbs_Circle: {
            const auto circle = curve.Circle();
            const auto position = circle.Position();
            return Base::Placement(
                toVector(circle.Location()),
                rotationWithZAxis(toVector(circle.Axis().Direction()), toVector(position.XDirection()))
            );
        }
        default:
            return std::nullopt;
    }
}

std::optional<Base::Placement> snapPlacementFromFace(const TopoDS_Face& face, Part::TopoShape& shape)
{
    BRepAdaptor_Surface surface(face, Standard_False);
    auto origin = shape.centerOfGravity().value_or(shape.getBoundBox().GetCenter());

    switch (surface.GetType()) {
        case GeomAbs_Plane: {
            const auto plane = surface.Plane();
            auto normal = toVector(plane.Axis().Direction());
            if (face.Orientation() == TopAbs_REVERSED) {
                normal *= -1.0;
            }
            return Base::Placement(
                origin,
                rotationWithZAxis(normal, toVector(plane.Position().XDirection()))
            );
        }
        case GeomAbs_Cylinder: {
            const auto cylinder = surface.Cylinder();
            const auto axis = cylinder.Axis();
            return Base::Placement(
                pointOnAxisNear(origin, axis),
                rotationWithZAxis(toVector(axis.Direction()), toVector(cylinder.Position().XDirection()))
            );
        }
        case GeomAbs_Cone: {
            const auto cone = surface.Cone();
            const auto axis = cone.Axis();
            return Base::Placement(
                pointOnAxisNear(origin, axis),
                rotationWithZAxis(toVector(axis.Direction()), toVector(cone.Position().XDirection()))
            );
        }
        case GeomAbs_Sphere:
            return Base::Placement(toVector(surface.Sphere().Location()), {});
        default:
            break;
    }

    return std::nullopt;
}

std::optional<Base::Placement> snapPlacementFromShape(Part::TopoShape& shape)
{
    switch (shape.getShape().ShapeType()) {
        case TopAbs_VERTEX: {
            const auto point = BRep_Tool::Pnt(TopoDS::Vertex(shape.getShape()));
            return Base::Placement(toVector(point), {});
        }
        case TopAbs_EDGE:
            return snapPlacementFromEdge(TopoDS::Edge(shape.getShape()));
        case TopAbs_FACE:
            return snapPlacementFromFace(TopoDS::Face(shape.getShape()), shape);
        default:
            break;
    }

    for (TopExp_Explorer faceExplorer(shape.getShape(), TopAbs_FACE); faceExplorer.More();
         faceExplorer.Next()) {
        auto face = Part::TopoShape(faceExplorer.Current());
        auto facePlacement = snapPlacementFromFace(TopoDS::Face(face.getShape()), face);
        if (facePlacement) {
            return facePlacement;
        }
    }
    for (TopExp_Explorer edgeExplorer(shape.getShape(), TopAbs_EDGE); edgeExplorer.More();
         edgeExplorer.Next()) {
        auto edgePlacement = snapPlacementFromEdge(TopoDS::Edge(edgeExplorer.Current()));
        if (edgePlacement) {
            return edgePlacement;
        }
    }

    return std::nullopt;
}

App::SubObjectPlacementProvider::SnapGeometryType snapGeometryTypeFromShape(Part::TopoShape& shape)
{
    using SnapGeometryType = App::SubObjectPlacementProvider::SnapGeometryType;

    switch (shape.getShape().ShapeType()) {
        case TopAbs_VERTEX:
            return SnapGeometryType::Point;
        case TopAbs_EDGE: {
            BRepAdaptor_Curve curve(TopoDS::Edge(shape.getShape()));
            switch (curve.GetType()) {
                case GeomAbs_Line:
                case GeomAbs_Circle:
                    return SnapGeometryType::Axis;
                default:
                    return SnapGeometryType::Unknown;
            }
        }
        case TopAbs_FACE: {
            BRepAdaptor_Surface surface(TopoDS::Face(shape.getShape()), Standard_False);
            switch (surface.GetType()) {
                case GeomAbs_Plane:
                    return SnapGeometryType::Plane;
                case GeomAbs_Cylinder:
                case GeomAbs_Cone:
                    return SnapGeometryType::Axis;
                case GeomAbs_Sphere:
                    return SnapGeometryType::Point;
                default:
                    return SnapGeometryType::Unknown;
            }
        }
        default:
            break;
    }

    for (TopExp_Explorer faceExplorer(shape.getShape(), TopAbs_FACE); faceExplorer.More();
         faceExplorer.Next()) {
        auto face = Part::TopoShape(faceExplorer.Current());
        auto type = snapGeometryTypeFromShape(face);
        if (type != SnapGeometryType::Unknown) {
            return type;
        }
    }
    for (TopExp_Explorer edgeExplorer(shape.getShape(), TopAbs_EDGE); edgeExplorer.More();
         edgeExplorer.Next()) {
        auto edge = Part::TopoShape(edgeExplorer.Current());
        auto type = snapGeometryTypeFromShape(edge);
        if (type != SnapGeometryType::Unknown) {
            return type;
        }
    }

    return SnapGeometryType::Unknown;
}

}  // namespace

App::SubObjectPlacementProvider::SnapGeometryType AttacherSubObjectPlacement::snapGeometryType(
    const App::SubObjectT& object
) const
{
    if (isAxisSystemObject(object)) {
        return SnapGeometryType::AxisSystem;
    }

    auto shape = getSubTopoShape(object);
    if (shape.isNull()) {
        return SnapGeometryType::Unknown;
    }

    return snapGeometryTypeFromShape(shape);
}

std::optional<Base::Placement> AttacherSubObjectPlacement::snapPlacement(
    const App::SubObjectT& object,
    Base::Placement
) const
{
    if (isAxisSystemObject(object)) {
        return Base::Placement {};
    }

    auto shape = getSubTopoShape(object);
    if (shape.isNull()) {
        return std::nullopt;
    }

    return snapPlacementFromShape(shape);
}

std::optional<Base::Vector3d> AttacherSubObjectPlacement::snapPosition(
    const App::SubObjectT& object,
    std::optional<Base::Vector3d> worldCursor,
    const Base::Matrix4D& objectToWorld
) const
{
    // Non-3D selections arrive with (0,0,0)
    if (!worldCursor) {
        return std::nullopt;
    }

    // getOldElementName() resolves via GeoFeature::resolveElement(), giving
    // "Edge1" / "Face2" etc. regardless of whether TNP mapped names are active.
    std::string elementName = object.getOldElementName();
    if (!elementName.starts_with("Edge")) {
        return std::nullopt;
    }

    auto shape = getSubTopoShape(object);

    if (shape.isNull() || shape.getShape().ShapeType() != TopAbs_EDGE) {
        return std::nullopt;
    }

    // Evaluate endpoint positions in local space via the same Geometry
    // abstraction the Attacher uses — this gives parameter-correct arc
    // endpoints regardless of curve type.
    auto geom = Part::Geometry::fromShape(shape.getShape());
    auto* curve = dynamic_cast<Part::GeomCurve*>(geom.get());
    if (!curve) {
        return std::nullopt;
    }

    auto u1 = curve->getFirstParameter();
    auto u2 = curve->getLastParameter();
    auto localP1 = curve->pointAtParameter(u1);
    auto localP2 = curve->pointAtParameter(u2);

    // Skip closed edges (full circles, closed splines).
    if ((localP1 - localP2).Length() < 1e-7) {
        return std::nullopt;
    }

    // Apply the caller-supplied object-to-world matrix to convert local edge
    // endpoints to world space for comparison with worldCursor.
    Base::Vector3d p1, p2;
    objectToWorld.multVec(localP1, p1);
    objectToWorld.multVec(localP2, p2);

    // Arc length gives a physically meaningful threshold regardless of curve
    // type or parameterisation non-uniformity.
    TopoDS_Edge edge = TopoDS::Edge(shape.getShape());
    GProp_GProps props;
    BRepGProp::LinearProperties(edge, props);
    double threshold = edgeEndpointSnapThreshold * props.Mass();

    double d1 = (*worldCursor - p1).Length();
    double d2 = (*worldCursor - p2).Length();

    // Snap to the nearer endpoint when both are within threshold, otherwise
    // to whichever single endpoint qualifies.
    if (d1 <= threshold && d1 <= d2) {
        return p1;
    }
    if (d2 <= threshold) {
        return p2;
    }

    return std::nullopt;
}

std::optional<Base::Vector3d> PartCenterOfMass::ofDocumentObject(App::DocumentObject* object) const
{
    if (const auto* feature = freecad_cast<Part::Feature*>(object)) {
        const auto shape = feature->Shape.getShape();

        if (const auto cog = shape.centerOfGravity()) {
            const Base::Placement comPlacement {*cog, Base::Rotation {}};

            return (feature->Placement.getValue().inverse() * comPlacement).getPosition();
        }
        return {};
    }

    // getTopoShape applies all placement levels so centerOfGravity() is in world
    // space; undo the object's own placement to get the dragger-local offset.
    const auto shape = Part::Feature::getTopoShape(object, Part::ShapeOption::Transform);
    if (!shape.isNull()) {
        if (const auto cog = shape.centerOfGravity()) {
            const Base::Placement comPlacement {*cog, Base::Rotation {}};
            if (const auto* prop = object->getPlacementProperty()) {
                return (prop->getValue().inverse() * comPlacement).getPosition();
            }
        }
    }

    return {};
}

bool PartCenterOfMass::supports(App::DocumentObject* object) const
{
    if (object->isDerivedFrom<Part::Feature>()) {
        return true;
    }

    if (object->isDerivedFrom<App::Link>()) {
        const auto* linked = object->getLinkedObject(true);
        return linked && linked != object;
    }

    if (object->isDerivedFrom<App::Part>()) {
        return !object->getSubObjects().empty();
    }

    return false;
}

std::optional<PyObject*> ShapeAttributeProvider::getAttribute(
    App::DocumentObject* object,
    const char* attr
) const
{
    if (Base::streq(attr, "Shape")) {
        Base::PyGILStateLocker lock;
        // Special treatment of Shape property
        static PyObject* getShape = nullptr;
        if (!getShape) {
            getShape = Py_None;
            PyObject* mod = PyImport_ImportModule("Part");
            if (!mod) {
                PyErr_Clear();
                return {};
            }

            Py::Object pyMod = Py::asObject(mod);
            if (pyMod.hasAttr("getShape")) {
                getShape = Py::new_reference_to(pyMod.getAttr("getShape"));
            }
        }
        if (getShape != Py_None) {
            Py::Tuple args(1);
            args.setItem(0, Py::asObject(object->getPyObject()));
            auto res = PyObject_CallObject(getShape, args.ptr());
            if (!res) {
                PyErr_Clear();
                return {};
            }

            Py::Object pyres(res, true);
            if (pyres.hasAttr("isNull")) {
                Py::Callable func(pyres.getAttr("isNull"));
                if (!func.apply().isTrue()) {
                    return Py::new_reference_to(res);
                }
            }
        }
    }

    return {};
}

Py::Object PartPseudoShapeProvider::getElement(
    const Py::Object& module,
    const Py::Object& object,
    const std::string& subname
) const
{
    Py::Callable func(module.getAttr("getShape"));
    Py::Tuple tuple(1);
    tuple.setItem(0, object);
    if (subname.empty()) {
        return func.apply(tuple);
    }

    Py::Dict dict;
    dict.setItem("subname", Py::String(subname));
    dict.setItem("needSubElement", Py::True());
    return func.apply(tuple, dict);
}
