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

#include "PreCompiled.h"
#ifndef _PreComp_
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Circ.hxx>
#include <gp_Cylinder.hxx>
#include <gp_Sphere.hxx>
#endif

#include <App/Application.h>
#include <App/Datums.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/PropertyStandard.h>
// #include <App/DocumentObjectGroup.h>
#include <App/Link.h>
// #include <Base/Console.h>
#include <Base/Placement.h>
#include <Base/Tools.h>
#include <Base/Interpreter.h>

#include <Mod/Part/App/DatumFeature.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/PartDesign/App/Body.h>

#include "AssemblyUtils.h"
#include "AssemblyObject.h"
#include "AssemblyLink.h"

#include "JointGroup.h"


namespace PartApp = Part;

// ======================================= Utils ======================================
namespace Assembly
{

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

bool isEdgeType(const App::DocumentObject* obj,
                const std::string& elName,
                const GeomAbs_CurveType type)
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

bool isFaceType(const App::DocumentObject* obj,
                const std::string& elName,
                const GeomAbs_SurfaceType type)
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

DistanceType getDistanceType(App::DocumentObject* joint)
{
    if (!joint) {
        return DistanceType::Other;
    }

    const auto type1 = getElementTypeFromProp(joint, "Reference1");
    const auto type2 = getElementTypeFromProp(joint, "Reference2");
    auto elt1 = getElementFromProp(joint, "Reference1");
    auto elt2 = getElementFromProp(joint, "Reference2");
    auto* obj1 = getLinkedObjFromRef(joint, "Reference1");
    auto* obj2 = getLinkedObjFromRef(joint, "Reference2");

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

        else if (isFaceType(obj1, elt1, GeomAbs_Cylinder)
                 || isFaceType(obj2, elt2, GeomAbs_Cylinder)) {
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
            return dynamic_cast<JointGroup*>(jointGroup);
        }
    }
    return nullptr;
}

void setJointActivated(const App::DocumentObject* joint, bool val)
{
    if (!joint) {
        return;
    }

    if (auto propActivated = joint->getPropertyByName<App::PropertyBool>("Activated")) {
        propActivated->setValue(val);
    }
}

bool getJointActivated(const App::DocumentObject* joint)
{
    if (!joint) {
        return false;
    }

    if (const auto propActivated = joint->getPropertyByName<App::PropertyBool>("Activated")) {
        return propActivated->getValue();
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
    if (subs.empty()) {
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
    if (names.empty()) {
        return "";
    }

    return names.back();
}

std::string getElementTypeFromProp(const App::DocumentObject* obj, const char* propName)
{
    // The prop is going to be something like 'Edge14' or 'Face7'. We need 'Edge' or 'Face'
    std::string elementType;
    for (const char ch : getElementFromProp(obj, propName)) {
        if (std::isalpha(ch)) {
            elementType += ch;
        }
    }
    return elementType;
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

App::DocumentObject* getObjFromRef(const App::DocumentObject* obj, const std::string& sub)
{
    if (!obj) {
        return nullptr;
    }

    const auto* doc = obj->getDocument();
    const auto names = Base::Tools::splitSubName(sub);

    // Lambda function to check if the typeId is a BodySubObject
    const auto isBodySubObject = [](App::DocumentObject* obj) -> bool {
        // PartDesign::Point + Line + Plane + CoordinateSystem
        // getViewProviderName instead of isDerivedFrom to avoid dependency on sketcher
        const auto isDerivedFromVpSketch =
            strcmp(obj->getViewProviderName(), "SketcherGui::ViewProviderSketch") == 0;
        return isDerivedFromVpSketch || obj->isDerivedFrom<PartApp::Datum>()
            || obj->isDerivedFrom<App::DatumElement>()
            || obj->isDerivedFrom<App::LocalCoordinateSystem>();
    };

    // Helper function to handle PartDesign::Body objects
    const auto handlePartDesignBody =
        [&](App::DocumentObject* obj,
            std::vector<std::string>::const_iterator it) -> App::DocumentObject* {
        const auto nextIt = std::next(it);
        if (nextIt != names.end()) {
            for (auto* obji : obj->getOutList()) {
                if (*nextIt == obji->getNameInDocument() && isBodySubObject(obji)) {
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
        else if (obj->isDerivedFrom<PartApp::Feature>()) {
            // Primitive, fastener, gear, etc.
            return obj;
        }
        else if (obj->isLink()) {
            App::DocumentObject* linked_obj = obj->getLinkedObject();
            if (linked_obj->isDerivedFrom<PartDesign::Body>()) {
                auto* retObj = handlePartDesignBody(linked_obj, it);
                return retObj == linked_obj ? obj : retObj;
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

    const App::DocumentObject* obj = prop->getValue();
    if (!obj) {
        return nullptr;
    }

    const std::vector<std::string> subs = prop->getSubValues();
    if (subs.empty()) {
        return nullptr;
    }

    return getObjFromRef(obj, subs[0]);
}

App::DocumentObject* getObjFromRef(const App::DocumentObject* joint, const char* pName)
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

    if (const auto* obj = getObjFromRef(joint, pObj)) {
        return obj->getLinkedObject(true);
    }
    return nullptr;
}

App::DocumentObject* getMovingPartFromRef(const AssemblyObject* assemblyObject,
                                          App::DocumentObject* obj,
                                          const std::string& sub)
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

App::DocumentObject* getMovingPartFromRef(const AssemblyObject* assemblyObject,
                                          App::PropertyXLinkSub* prop)
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

    return getMovingPartFromRef(assemblyObject, obj, subs[0]);
}

App::DocumentObject* getMovingPartFromRef(const AssemblyObject* assemblyObject,
                                          App::DocumentObject* joint,
                                          const char* pName)
{
    if (!joint) {
        return nullptr;
    }

    auto* prop = joint->getPropertyByName<App::PropertyXLinkSub>(pName);
    return getMovingPartFromRef(assemblyObject, prop);
}

/*
Base::Placement getPlacementFromProp(App::DocumentObject* obj, const char* propName)
{
    Base::Placement plc = Base::Placement();
    auto* propPlacement = dynamic_cast<App::PropertyPlacement*>(obj->getPropertyByName(propName));
    if (propPlacement) {
        plc = propPlacement->getValue();
    }
    return plc;
}

// Currently unused
Base::Placement* getTargetPlacementRelativeTo(
    App::DocumentObject* targetObj, App::DocumentObject* part, App::DocumentObject* container,
    bool inContainerBranch, bool ignorePlacement = false)
{
    inContainerBranch = inContainerBranch || (!ignorePlacement && part == container);

    Base::Console().Warning("sub --------------\n");
    if (targetObj == part && inContainerBranch && !ignorePlacement) {
        Base::Console().Warning("found0\n");
        return &getPlacementFromProp(targetObj, "Placement");
    }

    if (auto group = dynamic_cast<App::DocumentObjectGroup*>(part)) {
        for (auto& obj : group->getOutList()) {
            auto foundPlacement = getTargetPlacementRelativeTo(
                targetObj, obj, container, inContainerBranch, ignorePlacement
            );
            if (foundPlacement != nullptr) {
                return foundPlacement;
            }
        }
    }
    else if (auto assembly = dynamic_cast<AssemblyObject*>(part)) {
        Base::Console().Warning("h3\n");
        for (auto& obj : assembly->getOutList()) {
            auto foundPlacement = getTargetPlacementRelativeTo(
                targetObj, obj, container, inContainerBranch
            );
            if (foundPlacement == nullptr) {
                continue;
            }

            if (!ignorePlacement) {
                *foundPlacement = getPlacementFromProp(part, "Placement") * *foundPlacement;
            }

            Base::Console().Warning("found\n");
            return foundPlacement;
        }
    }
    else if (auto link = dynamic_cast<App::Link*>(part)) {
        Base::Console().Warning("h4\n");
        auto linked_obj = link->getLinkedObject();

        if (dynamic_cast<App::Part*>(linked_obj) || dynamic_cast<AssemblyObject*>(linked_obj)) {
            for (auto& obj : linked_obj->getOutList()) {
                auto foundPlacement = getTargetPlacementRelativeTo(
                    targetObj, obj, container, inContainerBranch
                );
                if (foundPlacement == nullptr) {
                    continue;
                }

                *foundPlacement = getPlacementFromProp(link, "Placement") * *foundPlacement;
                return foundPlacement;
            }
        }

        auto foundPlacement = getTargetPlacementRelativeTo(
            targetObj, linked_obj, container, inContainerBranch, true
        );

        if (foundPlacement != nullptr && !ignorePlacement) {
            *foundPlacement = getPlacementFromProp(link, "Placement") * *foundPlacement;
        }

        Base::Console().Warning("found2\n");
        return foundPlacement;
    }

    return nullptr;
}

Base::Placement getGlobalPlacement(App::DocumentObject* targetObj, App::DocumentObject* container =
nullptr) { bool inContainerBranch = container == nullptr; auto rootObjects =
App::GetApplication().getActiveDocument()->getRootObjects(); for (auto& part : rootObjects) { auto
foundPlacement = getTargetPlacementRelativeTo(targetObj, part, container, inContainerBranch); if
(foundPlacement != nullptr) { Base::Placement plc(foundPlacement->toMatrix()); return plc;
        }
    }

    return Base::Placement();
}
*/


}  // namespace Assembly
