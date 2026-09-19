// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2023 Ondsel <development@ondsel.com>                     *
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

#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Circ.hxx>
#include <gp_Cylinder.hxx>
#include <gp_Sphere.hxx>


#include <cctype>
#include <optional>

#include <App/Application.h>
#include <App/Datums.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/PropertyStandard.h>
#include <App/PropertyLinks.h>
#include <App/SemanticReference.h>
#include <App/Link.h>

#include <Base/Placement.h>
#include <Base/Tools.h>
#include <Base/Interpreter.h>

#include <Mod/Part/App/DatumFeature.h>
#include <Mod/Part/App/LinkArray.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/PartDesign/App/Body.h>

#include "AssemblyUtils.h"
#include "AssemblyObject.h"
#include "AssemblyLink.h"

#include "Groups.h"


namespace PartApp = Part;

// ======================================= Utils ======================================
namespace Assembly
{

bool isSuppressedLinkElement(const App::DocumentObject* obj)
{
    if (!obj || !obj->isDerivedFrom<App::LinkElement>()) {
        return false;
    }
    auto* ext = obj->getExtension<App::SuppressibleExtension>();
    return ext && ext->Suppressed.getValue();
}

void swapJCS(const App::DocumentObject* joint)
{
    if (!joint) {
        return;
    }

    auto pPlc1 = joint->getPropertyByName<App::PropertyPlacement>("Placement1");
    auto pPlc2 = joint->getPropertyByName<App::PropertyPlacement>("Placement2");
    if (pPlc1 && pPlc2) {
        const auto temp = pPlc1->getValue();
        pPlc1->setValue(pPlc2->getValue());
        pPlc2->setValue(temp);
    }
    auto pRef1 = joint->getPropertyByName<App::PropertyXLinkSub>("Reference1");
    auto pRef2 = joint->getPropertyByName<App::PropertyXLinkSub>("Reference2");
    if (pRef1 && pRef2) {
        auto temp = pRef1->getValue();
        auto subs1 = pRef1->getSubValues();
        auto subs2 = pRef2->getSubValues();
        pRef1->setValue(pRef2->getValue());
        pRef1->setSubValues(std::move(subs2));
        pRef2->setValue(temp);
        pRef2->setSubValues(std::move(subs1));
    }
}

bool isEdgeType(const App::DocumentObject* obj, const std::string& elName, const GeomAbs_CurveType type)
{
    auto* base = dynamic_cast<const PartApp::Feature*>(obj);
    if (!base) {
        return false;
    }

    const auto& TopShape = base->Shape.getShape();

    // Check for valid face types
    const auto edge = TopoDS::Edge(TopShape.getSubShape(elName.c_str()));
    BRepAdaptor_Curve sf(edge);

    return sf.GetType() == type;
}

bool isFaceType(const App::DocumentObject* obj, const std::string& elName, const GeomAbs_SurfaceType type)
{
    auto* base = dynamic_cast<const PartApp::Feature*>(obj);
    if (!base) {
        return false;
    }

    const auto TopShape = base->Shape.getShape();

    // Check for valid face types
    const auto face = TopoDS::Face(TopShape.getSubShape(elName.c_str()));
    BRepAdaptor_Surface sf(face);

    return sf.GetType() == type;
}

double getFaceRadius(const App::DocumentObject* obj, const std::string& elt)
{
    auto* base = dynamic_cast<const PartApp::Feature*>(obj);
    if (!base) {
        return 0.0;
    }

    const PartApp::TopoShape& TopShape = base->Shape.getShape();

    // Check for valid face types
    TopoDS_Face face = TopoDS::Face(TopShape.getSubShape(elt.c_str()));
    BRepAdaptor_Surface sf(face);

    const auto type = sf.GetType();
    return type == GeomAbs_Cylinder ? sf.Cylinder().Radius()
        : type == GeomAbs_Sphere    ? sf.Sphere().Radius()
                                    : 0.0;
}

double getEdgeRadius(const App::DocumentObject* obj, const std::string& elt)
{
    auto* base = dynamic_cast<const PartApp::Feature*>(obj);
    if (!base) {
        return 0.0;
    }

    const auto& TopShape = base->Shape.getShape();

    // Check for valid face types
    const auto edge = TopoDS::Edge(TopShape.getSubShape(elt.c_str()));
    BRepAdaptor_Curve sf(edge);

    return sf.GetType() == GeomAbs_Circle ? sf.Circle().Radius() : 0.0;
}

// Strip digits from a FaceN/EdgeN/VertexN (or Binding index toString) name.
// ASCII-only; unsigned char cast avoids std::isalpha UB on signed char.
static std::string elementTypeFromElementName(const std::string& elt)
{
    std::string elementType;
    for (const unsigned char ch : elt) {
        if (std::isalpha(ch)) {
            elementType += static_cast<char>(ch);
        }
    }
    return elementType;
}

DistanceType getDistanceType(App::DocumentObject* joint)
{
    // Convenience wrapper (AJ24-W1): discards post-swap elt/obj out-params.
    // Prefer the overload with out-params when the caller needs elements (AJ16-D1).
    std::string elt1;
    std::string elt2;
    App::DocumentObject* obj1 = nullptr;
    App::DocumentObject* obj2 = nullptr;
    return getDistanceType(joint, elt1, elt2, obj1, obj2);
}

DistanceType getDistanceType(
    App::DocumentObject* joint,
    std::string& elt1,
    std::string& elt2,
    App::DocumentObject*& obj1,
    App::DocumentObject*& obj2
)
{
    elt1.clear();
    elt2.clear();
    obj1 = nullptr;
    obj2 = nullptr;
    if (!joint) {
        return DistanceType::Other;
    }

    // Resolve each Reference once (Binding prefer + I7 FaceN/EdgeN fallback),
    // then derive Face/Edge/Vertex type from the resolved name. Avoids a second
    // uniqueBindingOnFeature walk via getElementTypeFromProp. Out-params expose the
    // post-swap resolution so makeMbdJointDistance need not resolve again (AJ16-D1).
    elt1 = getElementFromProp(joint, "Reference1");
    elt2 = getElementFromProp(joint, "Reference2");
    const auto type1 = elementTypeFromElementName(elt1);
    const auto type2 = elementTypeFromElementName(elt2);
    obj1 = getLinkedObjFromRef(joint, "Reference1");
    obj2 = getLinkedObjFromRef(joint, "Reference2");

    if (type1 == "Vertex" && type2 == "Vertex") {
        return DistanceType::PointPoint;
    }
    else if (type1 == "Edge" && type2 == "Edge") {
        if (isEdgeType(obj1, elt1, GeomAbs_Line) || isEdgeType(obj2, elt2, GeomAbs_Line)) {
            if (!isEdgeType(obj1, elt1, GeomAbs_Line)) {
                swapJCS(joint);  // make sure that line is first if not 2 lines.
                std::swap(elt1, elt2);
                std::swap(obj1, obj2);
            }

            if (isEdgeType(obj2, elt2, GeomAbs_Line)) {
                return DistanceType::LineLine;
            }
            else if (isEdgeType(obj2, elt2, GeomAbs_Circle)) {
                return DistanceType::LineCircle;
            }
            // TODO : other cases Ellipse, parabola, hyperbola...
        }

        else if (isEdgeType(obj1, elt1, GeomAbs_Circle) || isEdgeType(obj2, elt2, GeomAbs_Circle)) {
            if (!isEdgeType(obj1, elt1, GeomAbs_Circle)) {
                swapJCS(joint);  // make sure that circle is first if not 2 lines.
                std::swap(elt1, elt2);
                std::swap(obj1, obj2);
            }

            if (isEdgeType(obj2, elt2, GeomAbs_Circle)) {
                return DistanceType::CircleCircle;
            }
            // TODO : other cases Ellipse, parabola, hyperbola...
        }
    }
    else if (type1 == "Face" && type2 == "Face") {
        if (isFaceType(obj1, elt1, GeomAbs_Plane) || isFaceType(obj2, elt2, GeomAbs_Plane)) {
            if (!isFaceType(obj1, elt1, GeomAbs_Plane)) {
                swapJCS(joint);  // make sure plane is first if its not 2 planes.
                std::swap(elt1, elt2);
                std::swap(obj1, obj2);
            }

            if (isFaceType(obj2, elt2, GeomAbs_Plane)) {
                return DistanceType::PlanePlane;
            }
            else if (isFaceType(obj2, elt2, GeomAbs_Cylinder)) {
                return DistanceType::PlaneCylinder;
            }
            else if (isFaceType(obj2, elt2, GeomAbs_Sphere)) {
                return DistanceType::PlaneSphere;
            }
            else if (isFaceType(obj2, elt2, GeomAbs_Cone)) {
                return DistanceType::PlaneCone;
            }
            else if (isFaceType(obj2, elt2, GeomAbs_Torus)) {
                return DistanceType::PlaneTorus;
            }
        }

        else if (isFaceType(obj1, elt1, GeomAbs_Cylinder) || isFaceType(obj2, elt2, GeomAbs_Cylinder)) {
            if (!isFaceType(obj1, elt1, GeomAbs_Cylinder)) {
                swapJCS(joint);  // make sure cylinder is first if its not 2 cylinders.
                std::swap(elt1, elt2);
                std::swap(obj1, obj2);
            }

            if (isFaceType(obj2, elt2, GeomAbs_Cylinder)) {
                return DistanceType::CylinderCylinder;
            }
            else if (isFaceType(obj2, elt2, GeomAbs_Sphere)) {
                return DistanceType::CylinderSphere;
            }
            else if (isFaceType(obj2, elt2, GeomAbs_Cone)) {
                return DistanceType::CylinderCone;
            }
            else if (isFaceType(obj2, elt2, GeomAbs_Torus)) {
                return DistanceType::CylinderTorus;
            }
        }

        else if (isFaceType(obj1, elt1, GeomAbs_Cone) || isFaceType(obj2, elt2, GeomAbs_Cone)) {
            if (!isFaceType(obj1, elt1, GeomAbs_Cone)) {
                swapJCS(joint);  // make sure cone is first if its not 2 cones.
                std::swap(elt1, elt2);
                std::swap(obj1, obj2);
            }

            if (isFaceType(obj2, elt2, GeomAbs_Cone)) {
                return DistanceType::ConeCone;
            }
            else if (isFaceType(obj2, elt2, GeomAbs_Torus)) {
                return DistanceType::ConeTorus;
            }
            else if (isFaceType(obj2, elt2, GeomAbs_Sphere)) {
                return DistanceType::ConeSphere;
            }
        }

        else if (isFaceType(obj1, elt1, GeomAbs_Torus) || isFaceType(obj2, elt2, GeomAbs_Torus)) {
            if (!isFaceType(obj1, elt1, GeomAbs_Torus)) {
                swapJCS(joint);  // make sure torus is first if its not 2 torus.
                std::swap(elt1, elt2);
                std::swap(obj1, obj2);
            }

            if (isFaceType(obj2, elt2, GeomAbs_Torus)) {
                return DistanceType::TorusTorus;
            }
            else if (isFaceType(obj2, elt2, GeomAbs_Sphere)) {
                return DistanceType::TorusSphere;
            }
        }

        else if (isFaceType(obj1, elt1, GeomAbs_Sphere) || isFaceType(obj2, elt2, GeomAbs_Sphere)) {
            if (!isFaceType(obj1, elt1, GeomAbs_Sphere)) {
                swapJCS(joint);  // make sure sphere is first if its not 2 spheres.
                std::swap(elt1, elt2);
                std::swap(obj1, obj2);
            }

            if (isFaceType(obj2, elt2, GeomAbs_Sphere)) {
                return DistanceType::SphereSphere;
            }
        }
    }
    else if ((type1 == "Vertex" && type2 == "Face") || (type1 == "Face" && type2 == "Vertex")) {
        if (type1 == "Vertex") {  // Make sure face is the first.
            swapJCS(joint);
            std::swap(elt1, elt2);
            std::swap(obj1, obj2);
        }
        if (isFaceType(obj1, elt1, GeomAbs_Plane)) {
            return DistanceType::PointPlane;
        }
        else if (isFaceType(obj1, elt1, GeomAbs_Cylinder)) {
            return DistanceType::PointCylinder;
        }
        else if (isFaceType(obj1, elt1, GeomAbs_Sphere)) {
            return DistanceType::PointSphere;
        }
        else if (isFaceType(obj1, elt1, GeomAbs_Cone)) {
            return DistanceType::PointCone;
        }
        else if (isFaceType(obj1, elt1, GeomAbs_Torus)) {
            return DistanceType::PointTorus;
        }
    }
    else if ((type1 == "Edge" && type2 == "Face") || (type1 == "Face" && type2 == "Edge")) {
        if (type1 == "Edge") {  // Make sure face is the first.
            swapJCS(joint);
            std::swap(elt1, elt2);
            std::swap(obj1, obj2);
        }
        if (isEdgeType(obj2, elt2, GeomAbs_Line)) {
            if (isFaceType(obj1, elt1, GeomAbs_Plane)) {
                return DistanceType::LinePlane;
            }
            else if (isFaceType(obj1, elt1, GeomAbs_Cylinder)) {
                return DistanceType::LineCylinder;
            }
            else if (isFaceType(obj1, elt1, GeomAbs_Sphere)) {
                return DistanceType::LineSphere;
            }
            else if (isFaceType(obj1, elt1, GeomAbs_Cone)) {
                return DistanceType::LineCone;
            }
            else if (isFaceType(obj1, elt1, GeomAbs_Torus)) {
                return DistanceType::LineTorus;
            }
        }
        else {
            // For other curves we consider them as planes for now. Can be refined later.
            if (isFaceType(obj1, elt1, GeomAbs_Plane)) {
                return DistanceType::CurvePlane;
            }
            else if (isFaceType(obj1, elt1, GeomAbs_Cylinder)) {
                return DistanceType::CurveCylinder;
            }
            else if (isFaceType(obj1, elt1, GeomAbs_Sphere)) {
                return DistanceType::CurveSphere;
            }
            else if (isFaceType(obj1, elt1, GeomAbs_Cone)) {
                return DistanceType::CurveCone;
            }
            else if (isFaceType(obj1, elt1, GeomAbs_Torus)) {
                return DistanceType::CurveTorus;
            }
        }
    }
    else if ((type1 == "Vertex" && type2 == "Edge") || (type1 == "Edge" && type2 == "Vertex")) {
        if (type1 == "Vertex") {  // Make sure edge is the first.
            swapJCS(joint);
            std::swap(elt1, elt2);
            std::swap(obj1, obj2);
        }
        if (isEdgeType(obj1, elt1, GeomAbs_Line)) {  // Point on line joint.
            return DistanceType::PointLine;
        }
        else {
            // For other curves we do a point in plane-of-the-curve.
            // Maybe it would be best tangent / distance to the conic? For arcs and
            // circles we could use ASMTRevSphJoint. But is it better than pointInPlane?
            return DistanceType::PointCurve;
        }
    }
    return DistanceType::Other;
}

JointGroup* getJointGroup(const App::Part* part)
{
    if (!part) {
        return nullptr;
    }

    const auto* doc = part->getDocument();

    const auto jointGroups = doc->getObjectsOfType(JointGroup::getClassTypeId());
    if (jointGroups.empty()) {
        return nullptr;
    }
    for (auto jointGroup : jointGroups) {
        if (part->hasObject(jointGroup)) {
            return freecad_cast<JointGroup*>(jointGroup);
        }
    }
    return nullptr;
}

void setJointActivated(const App::DocumentObject* joint, bool val)
{
    if (!joint) {
        return;
    }

    if (auto propSuppressed = joint->getPropertyByName<App::PropertyBool>("Suppressed")) {
        propSuppressed->setValue(!val);
    }
}

bool getJointActivated(const App::DocumentObject* joint)
{
    if (!joint) {
        return false;
    }

    if (const auto propActivated = joint->getPropertyByName<App::PropertyBool>("Suppressed")) {
        return !propActivated->getValue();
    }
    return false;
}

double getJointDistance(const App::DocumentObject* joint, const char* propertyName)
{
    if (!joint) {
        return 0.0;
    }

    const auto* prop = joint->getPropertyByName<App::PropertyFloat>(propertyName);
    if (!prop) {
        return 0.0;
    }

    return prop->getValue();
}

double getJointAngle(const App::DocumentObject* joint)
{
    return getJointDistance(joint, "Angle");
}

double getJointDistance(const App::DocumentObject* joint)
{
    return getJointDistance(joint, "Distance");
}

double getJointDistance2(const App::DocumentObject* joint)
{
    return getJointDistance(joint, "Distance2");
}

JointType getJointType(const App::DocumentObject* joint)
{
    if (!joint) {
        return JointType::Fixed;
    }

    const auto* prop = joint->getPropertyByName<App::PropertyEnumeration>("JointType");
    if (!prop) {
        return JointType::Fixed;
    }

    return static_cast<JointType>(prop->getValue());
}

std::vector<std::string> getSubAsList(const App::PropertyXLinkSub* prop)
{
    if (!prop) {
        return {};
    }

    const auto subs = prop->getSubValues();
    // AJ6-M1 / I13: product Joints are single-sub. Multi-sub is ambiguous —
    // silence (empty) rather than first-wins on subs[0].
    if (subs.size() != 1) {
        return {};
    }
    return Base::Tools::splitSubName(subs[0]);
}

std::vector<std::string> getSubAsList(const App::DocumentObject* obj, const char* pName)
{
    if (!obj) {
        return {};
    }
    return getSubAsList(obj->getPropertyByName<App::PropertyXLinkSub>(pName));
}

std::string getElementFromProp(const App::DocumentObject* obj, const char* pName)
{
    if (!obj) {
        return "";
    }

    const auto names = getSubAsList(obj, pName);
    const std::string fallback = names.empty() ? std::string() : names.back();

    // Prefer a unique Binding index from dual-write seeds (I7 fallback to FaceN/EdgeN).
    // AJ6-M1 / I13: exactly one valid seed → Binding prefer; 0 seeds → FaceN cache;
    // >1 valid seeds → silence (cache fallback, no first-seed-wins). Binding
    // uniqueness itself is uniqueBindingOnFeature (0 or >1 → fallback).
    const auto* prop = obj->getPropertyByName<App::PropertyXLinkSub>(pName);
    if (!prop) {
        return fallback;
    }
    const auto& refs = prop->getSemanticRefs();
    const App::SemanticReference* uniqueSeed = nullptr;
    for (const auto& r : refs) {
        if (!r.seed.valid()) {
            continue;
        }
        if (uniqueSeed) {
            // Multi-seed XLinkSub: fail-closed (I13), do not front()-pick.
            return fallback;
        }
        uniqueSeed = &r;
    }
    if (!uniqueSeed) {
        return fallback;
    }
    const App::SemanticReference& sref = *uniqueSeed;
    App::DocumentObject* linked = getLinkedObjFromRef(obj, pName);
    if (!linked) {
        return fallback;
    }
    const App::SemanticGraph* graph = nullptr;
    if (obj->getDocument()) {
        graph = &obj->getDocument()->semanticGraph();
    }
    const char* indexType = "Face";
    if (sref.seed.kind == App::SemanticKind::Edge || sref.kind == App::SemanticKind::Edge) {
        indexType = "Edge";
    }
    else if (sref.seed.kind == App::SemanticKind::Vertex || sref.kind == App::SemanticKind::Vertex) {
        indexType = "Vertex";
    }
    // Consume uniqueness is all-eval (AG21-E1 / EM14-U1 lockstep): publishers
    // clearBindings first; do not switch to max-eval here without TESTS coverage.
    const std::optional<App::SemanticBinding> unique = App::uniqueBindingOnFeature(
        graph,
        sref.seed,
        static_cast<App::ObjectId>(linked->getID()),
        indexType
    );
    if (!unique.has_value() || unique->index.toString().empty()) {
        return fallback;  // I7 / I10: 0 or >1 stays the cache; no mint
    }
    return unique->index.toString();
}

std::string getElementTypeFromProp(const App::DocumentObject* obj, const char* propName)
{
    // Thin Face/Edge/Vertex classifier over getElementFromProp (Binding prefer + I7).
    // After AJ16-D1, in-tree Joint distance uses getDistanceType out-params instead;
    // keep this exported helper for other callers / future use (AJ24-T1) — do not
    // delete without an API audit. Prefer getDistanceType(..., elt, ...) when both
    // type and element are needed to avoid a second Binding walk.
    return elementTypeFromElementName(getElementFromProp(obj, propName));
}

App::DocumentObject* getObjFromProp(const App::DocumentObject* joint, const char* pName)
{
    if (!joint) {
        return {};
    }

    const auto* propObj = joint->getPropertyByName<App::PropertyLink>(pName);
    if (!propObj) {
        return {};
    }

    return propObj->getValue();
}

App::DocumentObject* getObjFromRef(App::DocumentObject* comp, const std::string& sub)
{
    if (!comp) {
        return nullptr;
    }

    const auto* doc = comp->getDocument();
    auto names = Base::Tools::splitSubName(sub);
    names.insert(names.begin(), comp->getNameInDocument());

    if (names.size() <= 2) {
        return comp;
    }

    // Lambda function to check if the typeId is a BodySubObject
    const auto isBodySubObject = [](App::DocumentObject* obj) -> bool {
        // PartDesign::Point + Line + Plane + CoordinateSystem
        // getViewProviderName instead of isDerivedFrom to avoid dependency on sketcher
        const auto isDerivedFromVpSketch
            = strcmp(obj->getViewProviderName(), "SketcherGui::ViewProviderSketch") == 0;
        return isDerivedFromVpSketch || obj->isDerivedFrom<PartApp::Datum>()
            || obj->isDerivedFrom<App::DatumElement>()
            || obj->isDerivedFrom<App::LocalCoordinateSystem>();
    };

    // Helper function to handle PartDesign::Body objects
    const auto handlePartDesignBody =
        [&](App::DocumentObject* obj,
            std::vector<std::string>::const_iterator it) -> App::DocumentObject* {
        auto nextIt = std::next(it);
        if (nextIt != names.end()) {
            for (auto* obji : obj->getOutList()) {
                if (*nextIt == obji->getNameInDocument() && isBodySubObject(obji)) {
                    // if obji is a LCS then perhaps we need to resolve one more level
                    if (auto* lcs = freecad_cast<App::LocalCoordinateSystem*>(obji)) {
                        nextIt = std::next(nextIt);
                        if (nextIt != names.end()) {
                            for (auto* objj : lcs->baseObjects()) {
                                if (*nextIt == objj->getNameInDocument()
                                    && objj->isDerivedFrom<App::DatumElement>()) {
                                    return objj;
                                }
                            }
                        }
                    }
                    return obji;
                }
            }
        }
        return obj;
    };


    for (auto it = names.begin(); it != names.end(); ++it) {
        App::DocumentObject* obj = doc->getObject(it->c_str());
        if (!obj) {
            return nullptr;
        }

        if (obj->isDerivedFrom<App::DocumentObjectGroup>()) {
            continue;
        }

        // The last but one name should be the selected
        if (std::next(it) == std::prev(names.end())) {
            return obj;
        }

        if (obj->isDerivedFrom<App::Part>() || obj->isLinkGroup()) {
            continue;
        }
        else if (obj->isDerivedFrom<PartDesign::Body>()) {
            return handlePartDesignBody(obj, it);
        }
        else if (obj->isDerivedFrom<PartApp::LinkArray>() || obj->isDerivedFrom<PartApp::Feature>()) {
            // Primitive, fastener, gear, etc.
            return obj;
        }
        else if (obj->isLink()) {
            App::DocumentObject* linked_obj = obj->getLinkedObject();
            if (linked_obj->isDerivedFrom<PartDesign::Body>()) {
                auto* retObj = handlePartDesignBody(linked_obj, it);
                return retObj == linked_obj ? obj : retObj;
            }
            else if (linked_obj->isDerivedFrom<PartApp::LinkArray>()) {
                return obj;
            }
            else if (linked_obj->isDerivedFrom<PartApp::Feature>()) {
                return obj;
            }
            else {
                doc = linked_obj->getDocument();
                continue;
            }
        }
    }

    return nullptr;
}

App::DocumentObject* getObjFromRef(const App::PropertyXLinkSub* prop)
{
    if (!prop) {
        return nullptr;
    }

    App::DocumentObject* obj = prop->getValue();
    if (!obj) {
        return nullptr;
    }

    const std::vector<std::string> subs = prop->getSubValues();
    if (subs.empty()) {
        return nullptr;
    }

    return getObjFromRef(obj, subs[0]);
}

App::DocumentObject* getObjFromJointRef(const App::DocumentObject* joint, const char* pName)
{
    if (!joint) {
        return nullptr;
    }

    const auto* prop = joint->getPropertyByName<App::PropertyXLinkSub>(pName);
    return getObjFromRef(prop);
}

App::DocumentObject* getLinkedObjFromRef(const App::DocumentObject* joint, const char* pObj)
{
    if (!joint) {
        return nullptr;
    }

    if (const auto* obj = getObjFromJointRef(joint, pObj)) {
        return obj->getLinkedObject(true);
    }
    return nullptr;
}

App::DocumentObject* getMovingPartFromSel(
    const AssemblyObject* assemblyObject,
    App::DocumentObject* obj,
    const std::string& sub
)
{
    if (!obj) {
        return nullptr;
    }

    auto* doc = obj->getDocument();

    auto names = Base::Tools::splitSubName(sub);
    names.insert(names.begin(), obj->getNameInDocument());

    bool assemblyPassed = false;

    for (const auto& objName : names) {
        obj = doc->getObject(objName.c_str());
        if (!obj) {
            continue;
        }

        if (obj->isLink()) {  // update the document if necessary for next object
            doc = obj->getLinkedObject()->getDocument();
        }

        if (obj == assemblyObject) {
            // We make sure we pass the assembly for cases like part.assembly.part.body
            assemblyPassed = true;
            continue;
        }
        if (!assemblyPassed) {
            continue;
        }

        if (obj->isDerivedFrom<App::DocumentObjectGroup>()) {
            continue;  // we ignore groups.
        }

        if (obj->isLinkGroup()) {
            continue;
        }

        // We ignore dynamic sub-assemblies.
        if (obj->isDerivedFrom<Assembly::AssemblyLink>()) {
            const auto* pRigid = obj->getPropertyByName<App::PropertyBool>("Rigid");
            if (pRigid && !pRigid->getValue()) {
                continue;
            }
        }

        return obj;
    }
    return nullptr;
}

App::DocumentObject* getMovingPartFromRef(App::PropertyXLinkSub* prop)
{
    if (!prop) {
        return nullptr;
    }

    return prop->getValue();
}

App::DocumentObject* getMovingPartFromRef(App::DocumentObject* joint, const char* pName)
{
    if (!joint) {
        return nullptr;
    }

    auto* prop = joint->getPropertyByName<App::PropertyXLinkSub>(pName);
    return getMovingPartFromRef(prop);
}

void syncPlacements(App::DocumentObject* src, App::DocumentObject* to)
{
    auto* plcPropSource = dynamic_cast<App::PropertyPlacement*>(src->getPropertyByName("Placement"));
    auto* plcPropLink = dynamic_cast<App::PropertyPlacement*>(to->getPropertyByName("Placement"));

    if (plcPropSource && plcPropLink) {
        if (!plcPropSource->getValue().isSame(plcPropLink->getValue())) {
            plcPropLink->setValue(plcPropSource->getValue());
        }
    }
}
namespace
{
// Helper function to perform the recursive traversal. Kept in an anonymous
// namespace as it's an implementation detail of getAssemblyComponents.
void collectComponentsRecursively(
    const std::vector<App::DocumentObject*>& objects,
    std::vector<App::DocumentObject*>& results
)
{
    for (auto* obj : objects) {
        if (!obj || isSuppressedLinkElement(obj)) {
            continue;
        }

        if (auto* asmLink = freecad_cast<Assembly::AssemblyLink*>(obj)) {
            // If the sub-assembly is rigid, treat it as a single movable part.
            // If it's flexible, we need to check its individual components.
            if (asmLink->isRigid()) {
                results.push_back(asmLink);
            }
            else {
                collectComponentsRecursively(asmLink->Group.getValues(), results);
            }
            continue;
        }
        else if (obj->isLinkGroup()) {
            auto* linkGroup = static_cast<App::Link*>(obj);
            for (auto* elt : linkGroup->ElementList.getValues()) {
                if (!elt || isSuppressedLinkElement(elt)) {
                    continue;
                }
                results.push_back(elt);
            }
            continue;
        }
        else if (obj->isDerivedFrom<PartApp::LinkArray>()) {
            results.push_back(obj);
            continue;
        }
        else if (auto* group = freecad_cast<App::DocumentObjectGroup*>(obj)) {
            collectComponentsRecursively(group->Group.getValues(), results);
            continue;
        }
        else if (auto* link = freecad_cast<App::Link*>(obj)) {
            obj = link->getLinkedObject();
            if (!obj) {
                continue;
            }
            if (obj->isDerivedFrom<PartApp::LinkArray>()
                || (obj->isDerivedFrom<App::GeoFeature>()
                    && !obj->isDerivedFrom<App::LocalCoordinateSystem>())) {
                results.push_back(link);
            }
        }

        else if (
            obj->isDerivedFrom<App::GeoFeature>() && !obj->isDerivedFrom<App::LocalCoordinateSystem>()
        ) {
            results.push_back(obj);
        }
    }
}
}  // namespace

std::vector<App::DocumentObject*> getAssemblyComponents(const AssemblyObject* assembly)
{
    if (!assembly) {
        return {};
    }

    std::vector<App::DocumentObject*> components;
    collectComponentsRecursively(assembly->Group.getValues(), components);
    return components;
}

double getJointCurrentValue(App::DocumentObject* joint, bool isAngle)
{
    Base::Placement plc1 = App::GeoFeature::getPlacementFromProp(joint, "Placement1");
    Base::Placement plc2 = App::GeoFeature::getPlacementFromProp(joint, "Placement2");

    auto* ref1 = dynamic_cast<App::PropertyXLinkSub*>(joint->getPropertyByName("Reference1"));
    auto* ref2 = dynamic_cast<App::PropertyXLinkSub*>(joint->getPropertyByName("Reference2"));
    if (!ref1 || !ref2) {
        return 0.0;
    }
    Base::Placement obj_global_plc1 = App::GeoFeature::getGlobalPlacement(nullptr, ref1);
    Base::Placement obj_global_plc2 = App::GeoFeature::getGlobalPlacement(nullptr, ref2);

    plc1 = obj_global_plc1 * plc1;
    plc2 = obj_global_plc2 * plc2;

    Base::Placement plc3 = plc1.inverse() * plc2;

    if (isAngle) {
        Base::Vector3d x_axis = plc3.getRotation().multVec(Base::Vector3d(1, 0, 0));
        return std::atan2(x_axis.y, x_axis.x);
    }
    return (plc1.getPosition() - plc2.getPosition()).Length()
        * (plc3.getPosition().z < 0 ? -1.0 : 1.0);
}
}  // namespace Assembly
