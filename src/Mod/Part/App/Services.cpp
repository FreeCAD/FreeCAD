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
#include <GProp_GProps.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>

#include <Base/Interpreter.h>
#include <Base/Matrix.h>
#include <Base/Vector3D.h>

#include <App/Link.h>
#include <App/Part.h>

#include "Geometry.h"
#include "PartFeature.h"
#include "Services.h"
#include "Tools.h"

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

}  // namespace

std::optional<Base::Vector3d> PartSubObjectSnap::snapPosition(
    App::SubObjectT object,
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

    // Mirror the Attacher's subname construction so all object types (Part,
    // PartDesign, links, assemblies) resolve correctly.
    std::string fullSubname = object.getSubNameNoElement() + elementName;

    auto shape = Part::Feature::getTopoShape(
        object.getObject(),
        Part::ShapeOption::NeedSubElement | Part::ShapeOption::ResolveLink,
        fullSubname.c_str()
    );

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
