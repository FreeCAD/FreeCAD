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
#include <cmath>
#include <vector>
#include <unordered_map>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObjectGroup.h>
#include <App/FeaturePythonPyImp.h>
#include <App/Link.h>
#include <App/PropertyPythonObject.h>
#include <Base/Console.h>
#include <Base/Placement.h>
#include <Base/Rotation.h>
#include <Base/Tools.h>
#include <Base/Interpreter.h>

#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/PartDesign/App/Body.h>

#include <OndselSolver/CREATE.h>
#include <OndselSolver/ASMTSimulationParameters.h>
#include <OndselSolver/ASMTAssembly.h>
#include <OndselSolver/ASMTMarker.h>
#include <OndselSolver/ASMTPart.h>
#include <OndselSolver/ASMTJoint.h>
#include <OndselSolver/ASMTFixedJoint.h>
#include <OndselSolver/ASMTRevoluteJoint.h>
#include <OndselSolver/ASMTCylindricalJoint.h>
#include <OndselSolver/ASMTTranslationalJoint.h>
#include <OndselSolver/ASMTSphericalJoint.h>
#include <OndselSolver/ASMTPointInPlaneJoint.h>
#include <OndselSolver/ASMTPointInLineJoint.h>
#include <OndselSolver/ASMTLineInPlaneJoint.h>
#include <OndselSolver/ASMTPlanarJoint.h>
#include <OndselSolver/ASMTRevCylJoint.h>
#include <OndselSolver/ASMTCylSphJoint.h>
#include <OndselSolver/ASMTSphSphJoint.h>
#include <OndselSolver/ASMTTime.h>
#include <OndselSolver/ASMTConstantGravity.h>

#include "AssemblyObject.h"
#include "AssemblyObjectPy.h"
#include "JointGroup.h"

namespace PartApp = Part;

using namespace Assembly;
using namespace MbD;

// ================================ Assembly Object ============================

PROPERTY_SOURCE(Assembly::AssemblyObject, App::Part)

AssemblyObject::AssemblyObject()
    : mbdAssembly(std::make_shared<ASMTAssembly>())
{}

AssemblyObject::~AssemblyObject() = default;

PyObject* AssemblyObject::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new AssemblyObjectPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

std::vector<App::DocumentObject*> AssemblyObject::getJointsOfObj(App::DocumentObject* obj)
{
    std::vector<App::DocumentObject*> joints = getJoints(false);
    std::vector<App::DocumentObject*> jointsOf;

    for (auto joint : joints) {
        App::DocumentObject* obj1 = getObjFromNameProp(joint, "object1", "Part1");
        App::DocumentObject* obj2 = getObjFromNameProp(joint, "Object2", "Part2");
        if (obj == obj1 || obj == obj2) {
            jointsOf.push_back(obj);
        }
    }

    return jointsOf;
}

std::vector<App::DocumentObject*> AssemblyObject::getJointsOfPart(App::DocumentObject* part)
{
    std::vector<App::DocumentObject*> joints = getJoints(false);
    std::vector<App::DocumentObject*> jointsOf;

    for (auto joint : joints) {
        App::DocumentObject* part1 = getLinkObjFromProp(joint, "Part1");
        App::DocumentObject* part2 = getLinkObjFromProp(joint, "Part2");
        if (part == part1 || part == part2) {
            jointsOf.push_back(joint);
        }
    }

    return jointsOf;
}

App::DocumentObject* AssemblyObject::getJointOfPartConnectingToGround(App::DocumentObject* part,
                                                                      std::string& name)
{
    std::vector<App::DocumentObject*> joints = getJointsOfPart(part);

    for (auto joint : joints) {
        if (!joint) {
            continue;
        }
        App::DocumentObject* part1 = getLinkObjFromProp(joint, "Part1");
        App::DocumentObject* part2 = getLinkObjFromProp(joint, "Part2");
        if (!part1 || !part2) {
            continue;
        }

        if (part == part1 && isJointConnectingPartToGround(joint, "Part1")) {
            name = "Part1";
            return joint;
        }
        if (part == part2 && isJointConnectingPartToGround(joint, "Part2")) {
            name = "Part2";
            return joint;
        }
    }

    return nullptr;
}

bool AssemblyObject::isJointConnectingPartToGround(App::DocumentObject* joint, const char* propname)
{
    auto* propPart = dynamic_cast<App::PropertyLink*>(joint->getPropertyByName(propname));
    if (!propPart) {
        return false;
    }
    App::DocumentObject* part = propPart->getValue();
    // Check if the part is disconnected even with the joint
    bool isConnected = isPartConnected(part);
    if (!isConnected) {
        return false;
    }

    // now we disconnect this joint temporarily
    propPart->setValue(nullptr);

    isConnected = isPartConnected(part);

    propPart->setValue(part);

    return !isConnected;
}

std::vector<App::DocumentObject*> AssemblyObject::getJoints(bool updateJCS)
{
    std::vector<App::DocumentObject*> joints = {};

    JointGroup* jointGroup = getJointGroup();
    if (!jointGroup) {
        return {};
    }

    Base::PyGILStateLocker lock;
    for (auto obj : jointGroup->getObjects()) {
        if (!obj) {
            continue;
        }

        auto proxy = dynamic_cast<App::PropertyPythonObject*>(obj->getPropertyByName("Proxy"));
        if (proxy) {
            Py::Object joint = proxy->getValue();
            if (joint.hasAttr("setJointConnectors")) {
                joints.push_back(obj);
            }
        }
    }

    // Make sure the joints are up to date.
    if (updateJCS) {
        recomputeJointPlacements(joints);
    }

    return joints;
}

std::vector<App::DocumentObject*> AssemblyObject::getGroundedJoints()
{
    std::vector<App::DocumentObject*> joints = {};

    JointGroup* jointGroup = getJointGroup();
    if (!jointGroup) {
        return {};
    }

    Base::PyGILStateLocker lock;
    for (auto obj : jointGroup->getObjects()) {
        if (!obj) {
            continue;
        }

        auto* propObj = dynamic_cast<App::PropertyLink*>(obj->getPropertyByName("ObjectToGround"));

        if (propObj) {
            joints.push_back(obj);
        }
    }

    return joints;
}

void AssemblyObject::removeUnconnectedJoints(std::vector<App::DocumentObject*>& joints,
                                             std::vector<App::DocumentObject*> groundedObjs)
{
    std::set<App::DocumentObject*> connectedParts;

    // Initialize connectedParts with groundedObjs
    for (auto* groundedObj : groundedObjs) {
        connectedParts.insert(groundedObj);
    }

    // Perform a traversal from each grounded object
    for (auto* groundedObj : groundedObjs) {
        traverseAndMarkConnectedParts(groundedObj, connectedParts, joints);
    }

    // Filter out unconnected joints
    joints.erase(
        std::remove_if(
            joints.begin(),
            joints.end(),
            [&connectedParts, this](App::DocumentObject* joint) {
                App::DocumentObject* obj1 = getLinkObjFromProp(joint, "Part1");
                App::DocumentObject* obj2 = getLinkObjFromProp(joint, "Part2");
                if ((connectedParts.find(obj1) == connectedParts.end())
                    || (connectedParts.find(obj2) == connectedParts.end())) {
                    Base::Console().Warning(
                        "%s is unconnected to a grounded part so it is ignored.\n",
                        joint->getFullName());
                    return true;  // Remove joint if any connected object is not in connectedParts
                }
                return false;
            }),
        joints.end());
}

void AssemblyObject::traverseAndMarkConnectedParts(App::DocumentObject* currentObj,
                                                   std::set<App::DocumentObject*>& connectedParts,
                                                   const std::vector<App::DocumentObject*>& joints)
{
    // getConnectedParts returns the objs connected to the currentObj by any joint
    auto connectedObjs = getConnectedParts(currentObj, joints);
    for (auto* nextObj : connectedObjs) {
        if (connectedParts.find(nextObj) == connectedParts.end()) {
            connectedParts.insert(nextObj);
            traverseAndMarkConnectedParts(nextObj, connectedParts, joints);
        }
    }
}

std::vector<App::DocumentObject*>
AssemblyObject::getConnectedParts(App::DocumentObject* part,
                                  const std::vector<App::DocumentObject*>& joints)
{
    std::vector<App::DocumentObject*> connectedParts;
    for (auto joint : joints) {
        App::DocumentObject* obj1 = getLinkObjFromProp(joint, "Part1");
        App::DocumentObject* obj2 = getLinkObjFromProp(joint, "Part2");
        if (obj1 == part) {
            connectedParts.push_back(obj2);
        }
        else if (obj2 == part) {
            connectedParts.push_back(obj1);
        }
    }
    return connectedParts;
}

bool AssemblyObject::isPartGrounded(App::DocumentObject* obj)
{
    std::vector<App::DocumentObject*> groundedObjs = fixGroundedParts();

    for (auto* groundedObj : groundedObjs) {
        if (groundedObj->getFullName() == obj->getFullName()) {
            return true;
        }
    }

    return false;
}

bool AssemblyObject::isPartConnected(App::DocumentObject* obj)
{
    std::vector<App::DocumentObject*> groundedObjs = getGroundedParts();
    std::vector<App::DocumentObject*> joints = getJoints(false);

    std::set<App::DocumentObject*> connectedParts;

    // Initialize connectedParts with groundedObjs
    for (auto* groundedObj : groundedObjs) {
        connectedParts.insert(groundedObj);
    }

    // Perform a traversal from each grounded object
    for (auto* groundedObj : groundedObjs) {
        traverseAndMarkConnectedParts(groundedObj, connectedParts, joints);
    }

    for (auto part : connectedParts) {
        if (obj == part) {
            return true;
        }
    }

    return false;
}

std::vector<App::DocumentObject*> AssemblyObject::getDownstreamParts(App::DocumentObject* part,
                                                                     int limit)
{
    if (limit > 1000) {  // Inifinite loop protection
        return {};
    }
    limit++;

    std::vector<App::DocumentObject*> downstreamParts = {part};
    std::string name;
    App::DocumentObject* connectingJoint = getJointOfPartConnectingToGround(part, name);
    std::vector<App::DocumentObject*> jointsOfPart = getJointsOfPart(part);

    // remove connectingJoint from jointsOfPart
    auto it = std::remove(jointsOfPart.begin(), jointsOfPart.end(), connectingJoint);
    jointsOfPart.erase(it, jointsOfPart.end());
    for (auto joint : jointsOfPart) {
        App::DocumentObject* part1 = getLinkObjFromProp(joint, "Part1");
        App::DocumentObject* part2 = getLinkObjFromProp(joint, "Part2");
        App::DocumentObject* downstreamPart =
            part->getFullName() == part1->getFullName() ? part2 : part1;

        std::vector<App::DocumentObject*> subDownstreamParts =
            getDownstreamParts(downstreamPart, limit);
        for (auto downPart : subDownstreamParts) {
            if (std::find(downstreamParts.begin(), downstreamParts.end(), downPart)
                == downstreamParts.end()) {
                downstreamParts.push_back(downPart);
            }
        }
    }
    return downstreamParts;
}

std::vector<App::DocumentObject*> AssemblyObject::getUpstreamParts(App::DocumentObject* part,
                                                                   int limit)
{
    if (limit > 1000) {  // Inifinite loop protection
        return {};
    }
    limit++;

    if (isPartGrounded(part)) {
        return {part};
    }

    std::string name;
    App::DocumentObject* connectingJoint = getJointOfPartConnectingToGround(part, name);
    App::DocumentObject* upPart =
        getLinkObjFromProp(connectingJoint, name == "Part1" ? "Part2" : "Part1");

    std::vector<App::DocumentObject*> upstreamParts = getUpstreamParts(upPart, limit);
    upstreamParts.push_back(part);
    return upstreamParts;
}

App::DocumentObject* AssemblyObject::getUpstreamMovingPart(App::DocumentObject* part)
{
    if (isPartGrounded(part)) {
        return nullptr;
    }

    std::string name;
    App::DocumentObject* connectingJoint = getJointOfPartConnectingToGround(part, name);
    JointType jointType = getJointType(connectingJoint);
    if (jointType != JointType::Fixed) {
        return part;
    }

    App::DocumentObject* upPart =
        getLinkObjFromProp(connectingJoint, name == "Part1" ? "Part2" : "Part1");

    return getUpstreamMovingPart(upPart);
}

JointGroup* AssemblyObject::getJointGroup()
{
    App::Document* doc = getDocument();

    std::vector<DocumentObject*> jointGroups =
        doc->getObjectsOfType(Assembly::JointGroup::getClassTypeId());
    if (jointGroups.empty()) {
        return nullptr;
    }
    for (auto jointGroup : jointGroups) {
        if (hasObject(jointGroup)) {
            return dynamic_cast<JointGroup*>(jointGroup);
        }
    }
    return nullptr;
}

std::vector<App::DocumentObject*> AssemblyObject::getGroundedParts()
{
    std::vector<App::DocumentObject*> groundedJoints = getGroundedJoints();

    std::vector<App::DocumentObject*> groundedObjs;
    for (auto gJoint : groundedJoints) {
        if (!gJoint) {
            continue;
        }

        auto* propObj =
            dynamic_cast<App::PropertyLink*>(gJoint->getPropertyByName("ObjectToGround"));

        if (propObj) {
            App::DocumentObject* objToGround = propObj->getValue();
            groundedObjs.push_back(objToGround);
        }
    }
    return groundedObjs;
}

std::vector<App::DocumentObject*> AssemblyObject::fixGroundedParts()
{
    std::vector<App::DocumentObject*> groundedJoints = getGroundedJoints();

    std::vector<App::DocumentObject*> groundedObjs;
    for (auto obj : groundedJoints) {
        if (!obj) {
            continue;
        }

        auto* propObj = dynamic_cast<App::PropertyLink*>(obj->getPropertyByName("ObjectToGround"));

        if (propObj) {
            App::DocumentObject* objToGround = propObj->getValue();

            Base::Placement plc = getPlacementFromProp(obj, "Placement");
            std::string str = obj->getFullName();
            fixGroundedPart(objToGround, plc, str);
            groundedObjs.push_back(objToGround);
        }
    }
    return groundedObjs;
}

void AssemblyObject::fixGroundedPart(App::DocumentObject* obj,
                                     Base::Placement& plc,
                                     std::string& name)
{
    std::string markerName1 = "marker-" + obj->getFullName();
    auto mbdMarker1 = makeMbdMarker(markerName1, plc);
    mbdAssembly->addMarker(mbdMarker1);

    std::shared_ptr<ASMTPart> mbdPart = getMbDPart(obj);

    std::string markerName2 = "FixingMarker";
    Base::Placement basePlc = Base::Placement();
    auto mbdMarker2 = makeMbdMarker(markerName2, basePlc);
    mbdPart->addMarker(mbdMarker2);

    markerName1 = "/OndselAssembly/" + mbdMarker1->name;
    markerName2 = "/OndselAssembly/" + mbdPart->name + "/" + mbdMarker2->name;

    auto mbdJoint = CREATE<ASMTFixedJoint>::With();
    mbdJoint->setName(name);
    mbdJoint->setMarkerI(markerName1);
    mbdJoint->setMarkerJ(markerName2);

    mbdAssembly->addJoint(mbdJoint);
}

void AssemblyObject::jointParts(std::vector<App::DocumentObject*> joints)
{
    for (auto* joint : joints) {
        if (!joint) {
            continue;
        }

        std::vector<std::shared_ptr<MbD::ASMTJoint>> mbdJoints = makeMbdJoint(joint);
        for (auto& mbdJoint : mbdJoints) {
            mbdAssembly->addJoint(mbdJoint);
        }
    }
}

int AssemblyObject::solve(bool enableRedo)
{
    mbdAssembly = makeMbdAssembly();
    objectPartMap.clear();

    std::vector<App::DocumentObject*> groundedObjs = fixGroundedParts();
    if (groundedObjs.empty()) {
        // If no part fixed we can't solve.
        return -6;
    }

    std::vector<App::DocumentObject*> joints = getJoints();

    removeUnconnectedJoints(joints, groundedObjs);

    jointParts(joints);

    if (enableRedo) {
        savePlacementsForUndo();
    }

    try {
        mbdAssembly->solve();
    }
    catch (...) {
        Base::Console().Error("Solve failed\n");
        return -1;
    }

    setNewPlacements();

    redrawJointPlacements(joints);

    return 0;
}

void AssemblyObject::exportAsASMT(std::string fileName)
{
    mbdAssembly = makeMbdAssembly();
    objectPartMap.clear();
    fixGroundedParts();

    std::vector<App::DocumentObject*> joints = getJoints();

    jointParts(joints);

    mbdAssembly->outputFile(fileName);
}

std::shared_ptr<ASMTJoint> AssemblyObject::makeMbdJointOfType(App::DocumentObject* joint,
                                                              JointType type)
{
    if (type == JointType::Fixed) {
        return CREATE<ASMTFixedJoint>::With();
    }
    else if (type == JointType::Revolute) {
        return CREATE<ASMTRevoluteJoint>::With();
    }
    else if (type == JointType::Cylindrical) {
        return CREATE<ASMTCylindricalJoint>::With();
    }
    else if (type == JointType::Slider) {
        return CREATE<ASMTTranslationalJoint>::With();
    }
    else if (type == JointType::Ball) {
        return CREATE<ASMTSphericalJoint>::With();
    }
    else if (type == JointType::Distance) {
        return makeMbdJointDistance(joint);
    }

    return nullptr;
}

std::shared_ptr<ASMTJoint> AssemblyObject::makeMbdJointDistance(App::DocumentObject* joint)
{
    // Depending on the type of element of the JCS, we apply the correct set of constraints.
    std::string type1 = getElementTypeFromProp(joint, "Element1");
    std::string type2 = getElementTypeFromProp(joint, "Element2");

    if (type1 == "Vertex" && type2 == "Vertex") {
        // Point to point distance, or ball joint if distance=0.
        auto mbdJoint = CREATE<ASMTSphSphJoint>::With();
        mbdJoint->distanceIJ = getJointDistance(joint);
        return mbdJoint;
    }
    else if (type1 == "Edge" && type2 == "Edge") {
        return makeMbdJointDistanceEdgeEdge(joint);
    }
    else if (type1 == "Face" && type2 == "Face") {
        return makeMbdJointDistanceFaceFace(joint);
    }
    else if ((type1 == "Vertex" && type2 == "Face") || (type1 == "Face" && type2 == "Vertex")) {
        if (type1 == "Vertex") {  // Make sure face is the first.
            swapJCS(joint);
        }
        return makeMbdJointDistanceFaceVertex(joint);
    }
    else if ((type1 == "Edge" && type2 == "Face") || (type1 == "Face" && type2 == "Edge")) {
        if (type1 == "Edge") {  // Make sure face is the first.
            swapJCS(joint);
        }
        return makeMbdJointDistanceFaceEdge(joint);
    }
    else if ((type1 == "Vertex" && type2 == "Edge") || (type1 == "Edge" && type2 == "Vertex")) {
        if (type1 == "Vertex") {  // Make sure edge is the first.
            swapJCS(joint);
        }
        return makeMbdJointDistanceEdgeVertex(joint);
    }

    return nullptr;
}

std::shared_ptr<ASMTJoint> AssemblyObject::makeMbdJointDistanceEdgeEdge(App::DocumentObject* joint)
{
    const char* elt1 = getElementFromProp(joint, "Element1");
    const char* elt2 = getElementFromProp(joint, "Element2");
    auto* obj1 = getLinkedObjFromNameProp(joint, "Object1", "Part1");
    auto* obj2 = getLinkedObjFromNameProp(joint, "Object2", "Part2");

    if (isEdgeType(obj1, elt1, GeomAbs_Line) || isEdgeType(obj2, elt2, GeomAbs_Line)) {
        if (!isEdgeType(obj1, elt1, GeomAbs_Line)) {
            swapJCS(joint);  // make sure that line is first if not 2 lines.
            std::swap(elt1, elt2);
            std::swap(obj1, obj2);
        }

        if (isEdgeType(obj2, elt2, GeomAbs_Line)) {
            auto mbdJoint = CREATE<ASMTRevCylJoint>::With();
            mbdJoint->distanceIJ = getJointDistance(joint);
            return mbdJoint;
        }
        else if (isEdgeType(obj2, elt2, GeomAbs_Circle)) {
            auto mbdJoint = CREATE<ASMTRevCylJoint>::With();
            mbdJoint->distanceIJ = getJointDistance(joint) + getEdgeRadius(obj2, elt2);
            return mbdJoint;
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
            auto mbdJoint = CREATE<ASMTRevCylJoint>::With();
            mbdJoint->distanceIJ =
                getJointDistance(joint) + getEdgeRadius(obj1, elt1) + getEdgeRadius(obj2, elt2);
            return mbdJoint;
        }
        // TODO : other cases Ellipse, parabola, hyperbola...
    }

    // TODO : other cases Ellipse, parabola, hyperbola...

    return nullptr;
}

std::shared_ptr<ASMTJoint> AssemblyObject::makeMbdJointDistanceFaceFace(App::DocumentObject* joint)
{
    const char* elt1 = getElementFromProp(joint, "Element1");
    const char* elt2 = getElementFromProp(joint, "Element2");
    auto* obj1 = getLinkedObjFromNameProp(joint, "Object1", "Part1");
    auto* obj2 = getLinkedObjFromNameProp(joint, "Object2", "Part2");

    if (isFaceType(obj1, elt1, GeomAbs_Plane) || isFaceType(obj2, elt2, GeomAbs_Plane)) {
        if (!isFaceType(obj1, elt1, GeomAbs_Plane)) {
            swapJCS(joint);  // make sure plane is first if its not 2 planes.
            std::swap(elt1, elt2);
            std::swap(obj1, obj2);
        }

        if (isFaceType(obj2, elt2, GeomAbs_Plane)) {
            auto mbdJoint = CREATE<ASMTPlanarJoint>::With();
            mbdJoint->offset = getJointDistance(joint);
            return mbdJoint;
        }
        else if (isFaceType(obj2, elt2, GeomAbs_Cylinder)) {
            auto mbdJoint = CREATE<ASMTLineInPlaneJoint>::With();
            mbdJoint->offset = getJointDistance(joint) + getFaceRadius(obj2, elt2);
            return mbdJoint;
        }
        else if (isFaceType(obj2, elt2, GeomAbs_Sphere)) {
            auto mbdJoint = CREATE<ASMTPointInPlaneJoint>::With();
            mbdJoint->offset = getJointDistance(joint) + getFaceRadius(obj2, elt2);
            return mbdJoint;
        }
        else if (isFaceType(obj2, elt2, GeomAbs_Cone)) {
            // TODO
        }
        else if (isFaceType(obj2, elt2, GeomAbs_Torus)) {
            auto mbdJoint = CREATE<ASMTPlanarJoint>::With();
            mbdJoint->offset = getJointDistance(joint);
            return mbdJoint;
        }
    }

    else if (isFaceType(obj1, elt1, GeomAbs_Cylinder) || isFaceType(obj2, elt2, GeomAbs_Cylinder)) {
        if (!isFaceType(obj1, elt1, GeomAbs_Cylinder)) {
            swapJCS(joint);  // make sure cylinder is first if its not 2 cylinders.
            std::swap(elt1, elt2);
            std::swap(obj1, obj2);
        }

        if (isFaceType(obj2, elt2, GeomAbs_Cylinder)) {
            auto mbdJoint = CREATE<ASMTRevCylJoint>::With();
            mbdJoint->distanceIJ =
                getJointDistance(joint) + getFaceRadius(obj1, elt1) + getFaceRadius(obj2, elt2);
            return mbdJoint;
        }
        else if (isFaceType(obj2, elt2, GeomAbs_Sphere)) {
            auto mbdJoint = CREATE<ASMTCylSphJoint>::With();
            mbdJoint->distanceIJ =
                getJointDistance(joint) + getFaceRadius(obj1, elt1) + getFaceRadius(obj2, elt2);
            return mbdJoint;
        }
        else if (isFaceType(obj2, elt2, GeomAbs_Cone)) {
            // TODO
        }
        else if (isFaceType(obj2, elt2, GeomAbs_Torus)) {
            auto mbdJoint = CREATE<ASMTRevCylJoint>::With();
            mbdJoint->distanceIJ =
                getJointDistance(joint) + getFaceRadius(obj1, elt1) + getFaceRadius(obj2, elt2);
            return mbdJoint;
        }
    }

    else if (isFaceType(obj1, elt1, GeomAbs_Cone) || isFaceType(obj2, elt2, GeomAbs_Cone)) {
        if (!isFaceType(obj1, elt1, GeomAbs_Cone)) {
            swapJCS(joint);  // make sure cone is first if its not 2 cones.
            std::swap(elt1, elt2);
            std::swap(obj1, obj2);
        }

        if (isFaceType(obj2, elt2, GeomAbs_Cone)) {
            // TODO
        }
        else if (isFaceType(obj2, elt2, GeomAbs_Torus)) {
            // TODO
        }
        else if (isFaceType(obj2, elt2, GeomAbs_Sphere)) {
            // TODO
        }
    }

    else if (isFaceType(obj1, elt1, GeomAbs_Torus) || isFaceType(obj2, elt2, GeomAbs_Torus)) {
        if (!isFaceType(obj1, elt1, GeomAbs_Torus)) {
            swapJCS(joint);  // make sure torus is first if its not 2 torus.
            std::swap(elt1, elt2);
            std::swap(obj1, obj2);
        }

        if (isFaceType(obj2, elt2, GeomAbs_Torus)) {
            auto mbdJoint = CREATE<ASMTPlanarJoint>::With();
            mbdJoint->offset = getJointDistance(joint);
            return mbdJoint;
        }
        else if (isFaceType(obj2, elt2, GeomAbs_Sphere)) {
            auto mbdJoint = CREATE<ASMTCylSphJoint>::With();
            mbdJoint->distanceIJ =
                getJointDistance(joint) + getFaceRadius(obj1, elt1) + getFaceRadius(obj2, elt2);
            return mbdJoint;
        }
    }

    else if (isFaceType(obj1, elt1, GeomAbs_Sphere) || isFaceType(obj2, elt2, GeomAbs_Sphere)) {
        if (!isFaceType(obj1, elt1, GeomAbs_Sphere)) {
            swapJCS(joint);  // make sure sphere is first if its not 2 spheres.
            std::swap(elt1, elt2);
            std::swap(obj1, obj2);
        }

        if (isFaceType(obj2, elt2, GeomAbs_Sphere)) {
            auto mbdJoint = CREATE<ASMTSphSphJoint>::With();
            mbdJoint->distanceIJ =
                getJointDistance(joint) + getFaceRadius(obj1, elt1) + getFaceRadius(obj2, elt2);
            return mbdJoint;
        }
    }
    else {
        // by default we make a planar joint.
        auto mbdJoint = CREATE<ASMTPlanarJoint>::With();
        mbdJoint->offset = getJointDistance(joint);
        return mbdJoint;
    }

    return nullptr;
}

std::shared_ptr<ASMTJoint>
AssemblyObject::makeMbdJointDistanceFaceVertex(App::DocumentObject* joint)
{
    const char* elt1 = getElementFromProp(joint, "Element1");
    auto* obj1 = getLinkedObjFromNameProp(joint, "Object1", "Part1");

    if (isFaceType(obj1, elt1, GeomAbs_Plane)) {
        auto mbdJoint = CREATE<ASMTPointInPlaneJoint>::With();
        mbdJoint->offset = getJointDistance(joint);
        return mbdJoint;
    }
    else if (isFaceType(obj1, elt1, GeomAbs_Cylinder)) {
        auto mbdJoint = CREATE<ASMTCylSphJoint>::With();
        mbdJoint->distanceIJ = getJointDistance(joint) + getFaceRadius(obj1, elt1);
        return mbdJoint;
    }
    else if (isFaceType(obj1, elt1, GeomAbs_Sphere)) {
        auto mbdJoint = CREATE<ASMTSphSphJoint>::With();
        mbdJoint->distanceIJ = getJointDistance(joint) + getFaceRadius(obj1, elt1);
        return mbdJoint;
    }
    /*else if (isFaceType(obj1, elt1, GeomAbs_Cone)) {
        // TODO
    }
    else if (isFaceType(obj1, elt1, GeomAbs_Thorus)) {
        // TODO
    }*/

    return nullptr;
}

std::shared_ptr<ASMTJoint>
AssemblyObject::makeMbdJointDistanceEdgeVertex(App::DocumentObject* joint)
{
    const char* elt1 = getElementFromProp(joint, "Element1");
    auto* obj1 = getLinkedObjFromNameProp(joint, "Object1", "Part1");

    if (isEdgeType(obj1, elt1, GeomAbs_Line)) {  // Point on line joint.
        auto mbdJoint = CREATE<ASMTCylSphJoint>::With();
        mbdJoint->distanceIJ = getJointDistance(joint);
        return mbdJoint;
    }
    else {
        // For other curves we do a point in plane-of-the-curve.
        // Maybe it would be best tangent / distance to the conic?
        // For arcs and circles we could use ASMTRevSphJoint. But is it better than pointInPlane?
        auto mbdJoint = CREATE<ASMTPointInPlaneJoint>::With();
        mbdJoint->offset = getJointDistance(joint);
        return mbdJoint;
    }

    return nullptr;
}

std::shared_ptr<ASMTJoint> AssemblyObject::makeMbdJointDistanceFaceEdge(App::DocumentObject* joint)
{
    const char* elt2 = getElementFromProp(joint, "Element2");
    auto* obj2 = getLinkedObjFromNameProp(joint, "Object2", "Part2");

    if (isEdgeType(obj2, elt2, GeomAbs_Line)) {
        // Make line in plane joint.
        auto mbdJoint = CREATE<ASMTLineInPlaneJoint>::With();
        mbdJoint->offset = getJointDistance(joint);
        return mbdJoint;
    }
    else {
        // planar joint for other edges.
        auto mbdJoint = CREATE<ASMTPlanarJoint>::With();
        mbdJoint->offset = getJointDistance(joint);
        return mbdJoint;
    }

    return nullptr;
}


std::vector<std::shared_ptr<MbD::ASMTJoint>>
AssemblyObject::makeMbdJoint(App::DocumentObject* joint)
{
    JointType jointType = getJointType(joint);

    std::shared_ptr<ASMTJoint> mbdJoint = makeMbdJointOfType(joint, jointType);
    if (!mbdJoint) {
        return {};
    }

    std::string fullMarkerName1 = handleOneSideOfJoint(joint, "Object1", "Part1", "Placement1");
    std::string fullMarkerName2 = handleOneSideOfJoint(joint, "Object2", "Part2", "Placement2");

    mbdJoint->setName(joint->getFullName());
    mbdJoint->setMarkerI(fullMarkerName1);
    mbdJoint->setMarkerJ(fullMarkerName2);

    return {mbdJoint};
}

std::string AssemblyObject::handleOneSideOfJoint(App::DocumentObject* joint,
                                                 const char* propObjName,
                                                 const char* propPartName,
                                                 const char* propPlcName)
{
    App::DocumentObject* part = getLinkObjFromProp(joint, propPartName);
    App::DocumentObject* obj = getObjFromNameProp(joint, propObjName, propPartName);

    std::shared_ptr<ASMTPart> mbdPart = getMbDPart(part);
    Base::Placement partPlc = getPlacementFromProp(part, "Placement");
    Base::Placement objPlc = getPlacementFromProp(obj, "Placement");
    Base::Placement plc = getPlacementFromProp(joint, propPlcName);
    // Now we have plc which is the JCS placement, but its relative to the Object, not to the
    // containing Part.

    if (obj->getNameInDocument() != part->getNameInDocument()) {
        // Make plc relative to the containing part
        // plc = objPlc * plc; // this would not work for nested parts.

        Base::Placement obj_global_plc = getGlobalPlacement(obj, part);
        plc = obj_global_plc * plc;

        Base::Placement part_global_plc = getGlobalPlacement(part);
        plc = part_global_plc.inverse() * plc;
    }

    std::string markerName = joint->getFullName();
    auto mbdMarker = makeMbdMarker(markerName, plc);
    mbdPart->addMarker(mbdMarker);

    return "/OndselAssembly/" + mbdPart->name + "/" + markerName;
}

std::shared_ptr<ASMTPart> AssemblyObject::getMbDPart(App::DocumentObject* obj)
{
    std::shared_ptr<ASMTPart> mbdPart;

    Base::Placement plc = getPlacementFromProp(obj, "Placement");

    auto it = objectPartMap.find(obj);
    if (it != objectPartMap.end()) {
        // obj has been associated with an ASMTPart before
        mbdPart = it->second;
    }
    else {
        // obj has not been associated with an ASMTPart before
        std::string str = obj->getFullName();
        mbdPart = makeMbdPart(str, plc);
        mbdAssembly->addPart(mbdPart);
        objectPartMap[obj] = mbdPart;  // Store the association
    }

    return mbdPart;
}

std::shared_ptr<ASMTPart>
AssemblyObject::makeMbdPart(std::string& name, Base::Placement plc, double mass)
{
    auto mdbPart = CREATE<ASMTPart>::With();
    mdbPart->setName(name);

    auto massMarker = CREATE<ASMTPrincipalMassMarker>::With();
    massMarker->setMass(mass);
    massMarker->setDensity(1.0);
    massMarker->setMomentOfInertias(1.0, 1.0, 1.0);
    mdbPart->setPrincipalMassMarker(massMarker);

    Base::Vector3d pos = plc.getPosition();
    mdbPart->setPosition3D(pos.x, pos.y, pos.z);
    // Base::Console().Warning("MbD Part placement : (%f, %f, %f)\n", pos.x, pos.y, pos.z);

    // TODO : replace with quaternion to simplify
    Base::Rotation rot = plc.getRotation();
    Base::Matrix4D mat;
    rot.getValue(mat);
    Base::Vector3d r0 = mat.getRow(0);
    Base::Vector3d r1 = mat.getRow(1);
    Base::Vector3d r2 = mat.getRow(2);
    mdbPart->setRotationMatrix(r0.x, r0.y, r0.z, r1.x, r1.y, r1.z, r2.x, r2.y, r2.z);
    /*double q0, q1, q2, q3;
    rot.getValue(q0, q1, q2, q3);
    mdbPart->setQuarternions(q0, q1, q2, q3);*/

    return mdbPart;
}

std::shared_ptr<ASMTAssembly> AssemblyObject::makeMbdAssembly()
{
    auto assembly = CREATE<ASMTAssembly>::With();
    assembly->setName("OndselAssembly");

    return assembly;
}

std::shared_ptr<ASMTMarker> AssemblyObject::makeMbdMarker(std::string& name, Base::Placement& plc)
{
    auto mbdMarker = CREATE<ASMTMarker>::With();
    mbdMarker->setName(name);

    Base::Vector3d pos = plc.getPosition();
    mbdMarker->setPosition3D(pos.x, pos.y, pos.z);

    // TODO : replace with quaternion to simplify
    Base::Rotation rot = plc.getRotation();
    Base::Matrix4D mat;
    rot.getValue(mat);
    Base::Vector3d r0 = mat.getRow(0);
    Base::Vector3d r1 = mat.getRow(1);
    Base::Vector3d r2 = mat.getRow(2);
    mbdMarker->setRotationMatrix(r0.x, r0.y, r0.z, r1.x, r1.y, r1.z, r2.x, r2.y, r2.z);
    /*double q0, q1, q2, q3;
    rot.getValue(q0, q1, q2, q3);
    mbdMarker->setQuarternions(q0, q1, q2, q3);*/
    return mbdMarker;
}

void AssemblyObject::swapJCS(App::DocumentObject* joint)
{
    auto propElement1 = dynamic_cast<App::PropertyString*>(joint->getPropertyByName("Element1"));
    auto propElement2 = dynamic_cast<App::PropertyString*>(joint->getPropertyByName("Element2"));
    if (propElement1 && propElement2) {
        auto temp = std::string(propElement1->getValue());
        propElement1->setValue(propElement2->getValue());
        propElement2->setValue(temp);
    }
    auto propVertex1 = dynamic_cast<App::PropertyString*>(joint->getPropertyByName("Vertex1"));
    auto propVertex2 = dynamic_cast<App::PropertyString*>(joint->getPropertyByName("Vertex2"));
    if (propVertex1 && propVertex2) {
        auto temp = std::string(propVertex1->getValue());
        propVertex1->setValue(propVertex2->getValue());
        propVertex2->setValue(temp);
    }
    auto propPlacement1 =
        dynamic_cast<App::PropertyPlacement*>(joint->getPropertyByName("Placement1"));
    auto propPlacement2 =
        dynamic_cast<App::PropertyPlacement*>(joint->getPropertyByName("Placement2"));
    if (propPlacement1 && propPlacement2) {
        auto temp = propPlacement1->getValue();
        propPlacement1->setValue(propPlacement2->getValue());
        propPlacement2->setValue(temp);
    }
    auto propObject1 = dynamic_cast<App::PropertyString*>(joint->getPropertyByName("Object1"));
    auto propObject2 = dynamic_cast<App::PropertyString*>(joint->getPropertyByName("Object2"));
    if (propObject1 && propObject2) {
        auto temp = std::string(propObject1->getValue());
        propObject1->setValue(propObject2->getValue());
        propObject2->setValue(temp);
    }
    auto propPart1 = dynamic_cast<App::PropertyLink*>(joint->getPropertyByName("Part1"));
    auto propPart2 = dynamic_cast<App::PropertyLink*>(joint->getPropertyByName("Part2"));
    if (propPart1 && propPart2) {
        auto temp = propPart1->getValue();
        propPart1->setValue(propPart2->getValue());
        propPart2->setValue(temp);
    }
}

void AssemblyObject::savePlacementsForUndo()
{
    previousPositions.clear();

    for (auto& pair : objectPartMap) {
        App::DocumentObject* obj = pair.first;
        if (!obj) {
            continue;
        }

        std::pair<App::DocumentObject*, Base::Placement> savePair;
        savePair.first = obj;

        // Check if the object has a "Placement" property
        auto* propPlc = dynamic_cast<App::PropertyPlacement*>(obj->getPropertyByName("Placement"));
        if (!propPlc) {
            continue;
        }
        savePair.second = propPlc->getValue();

        previousPositions.push_back(savePair);
    }
}

void AssemblyObject::undoSolve()
{
    if (previousPositions.size() == 0) {
        return;
    }

    for (auto& pair : previousPositions) {
        App::DocumentObject* obj = pair.first;
        if (!obj) {
            continue;
        }

        // Check if the object has a "Placement" property
        auto* propPlacement =
            dynamic_cast<App::PropertyPlacement*>(obj->getPropertyByName("Placement"));
        if (!propPlacement) {
            continue;
        }

        propPlacement->setValue(pair.second);
    }
    previousPositions.clear();

    // update joint placements:
    getJoints();
}

void AssemblyObject::clearUndo()
{
    previousPositions.clear();
}

void AssemblyObject::setNewPlacements()
{
    for (auto& pair : objectPartMap) {
        App::DocumentObject* obj = pair.first;
        std::shared_ptr<ASMTPart> mbdPart = pair.second;

        if (!obj || !mbdPart) {
            continue;
        }

        // Check if the object has a "Placement" property
        auto* propPlacement =
            dynamic_cast<App::PropertyPlacement*>(obj->getPropertyByName("Placement"));
        if (!propPlacement) {
            continue;
        }

        double x, y, z;
        mbdPart->getPosition3D(x, y, z);
        // Base::Console().Warning("in set placement : (%f, %f, %f)\n", x, y, z);
        Base::Vector3d pos = Base::Vector3d(x, y, z);

        // TODO : replace with quaternion to simplify
        auto& r0 = mbdPart->rotationMatrix->at(0);
        auto& r1 = mbdPart->rotationMatrix->at(1);
        auto& r2 = mbdPart->rotationMatrix->at(2);
        Base::Vector3d row0 = Base::Vector3d(r0->at(0), r0->at(1), r0->at(2));
        Base::Vector3d row1 = Base::Vector3d(r1->at(0), r1->at(1), r1->at(2));
        Base::Vector3d row2 = Base::Vector3d(r2->at(0), r2->at(1), r2->at(2));
        Base::Matrix4D mat;
        mat.setRow(0, row0);
        mat.setRow(1, row1);
        mat.setRow(2, row2);
        Base::Rotation rot = Base::Rotation(mat);

        /*double q0, q1, q2, q3;
        mbdPart->getQuarternions(q0, q1, q2, q3);
        Base::Rotation rot = Base::Rotation(q0, q1, q2, q3);*/

        Base::Placement newPlacement = Base::Placement(pos, rot);

        propPlacement->setValue(newPlacement);
    }
}

void AssemblyObject::redrawJointPlacements(std::vector<App::DocumentObject*> joints)
{
    // Notify the joint objects that the transform of the coin object changed.
    for (auto* joint : joints) {
        auto* propPlacement =
            dynamic_cast<App::PropertyPlacement*>(joint->getPropertyByName("Placement1"));
        if (propPlacement) {
            propPlacement->setValue(propPlacement->getValue());
        }
        propPlacement =
            dynamic_cast<App::PropertyPlacement*>(joint->getPropertyByName("Placement2"));
        if (propPlacement) {
            propPlacement->setValue(propPlacement->getValue());
        }
    }
}

void AssemblyObject::recomputeJointPlacements(std::vector<App::DocumentObject*> joints)
{
    // The Placement1 and Placement2 of each joint needs to be updated as the parts moved.
    for (auto* joint : joints) {
        App::PropertyPythonObject* proxy = joint
            ? dynamic_cast<App::PropertyPythonObject*>(joint->getPropertyByName("Proxy"))
            : nullptr;

        if (!proxy) {
            continue;
        }

        Py::Object jointPy = proxy->getValue();

        if (!jointPy.hasAttr("updateJCSPlacements")) {
            continue;
        }

        Py::Object attr = jointPy.getAttr("updateJCSPlacements");
        if (attr.ptr() && attr.isCallable()) {
            Py::Tuple args(1);
            args.setItem(0, Py::asObject(joint->getPyObject()));
            Py::Callable(attr).apply(args);
        }
    }
}

double AssemblyObject::getObjMass(App::DocumentObject* obj)
{
    for (auto& pair : objMasses) {
        if (pair.first == obj) {
            return pair.second;
        }
    }
    return 1.0;
}

void AssemblyObject::setObjMasses(std::vector<std::pair<App::DocumentObject*, double>> objectMasses)
{
    objMasses = objectMasses;
}

bool AssemblyObject::isFaceType(App::DocumentObject* obj,
                                const char* elName,
                                GeomAbs_SurfaceType type)
{
    auto base = static_cast<PartApp::Feature*>(obj);
    PartApp::TopoShape TopShape = base->Shape.getShape();

    // Check for valid face types
    TopoDS_Face face = TopoDS::Face(TopShape.getSubShape(elName));
    BRepAdaptor_Surface sf(face);
    // GeomAbs_Plane GeomAbs_Cylinder GeomAbs_Cone GeomAbs_Sphere GeomAbs_Thorus
    if (sf.GetType() == type) {
        return true;
    }

    return false;
}

bool AssemblyObject::isEdgeType(App::DocumentObject* obj,
                                const char* elName,
                                GeomAbs_CurveType type)
{
    PartApp::Feature* base = static_cast<PartApp::Feature*>(obj);
    const PartApp::TopoShape& TopShape = base->Shape.getShape();

    // Check for valid face types
    TopoDS_Edge edge = TopoDS::Edge(TopShape.getSubShape(elName));
    BRepAdaptor_Curve sf(edge);

    if (sf.GetType() == type) {
        return true;
    }

    return false;
}

double AssemblyObject::getFaceRadius(App::DocumentObject* obj, const char* elt)
{
    auto base = static_cast<PartApp::Feature*>(obj);
    const PartApp::TopoShape& TopShape = base->Shape.getShape();

    // Check for valid face types
    TopoDS_Face face = TopoDS::Face(TopShape.getSubShape(elt));
    BRepAdaptor_Surface sf(face);

    if (sf.GetType() == GeomAbs_Cylinder) {
        return sf.Cylinder().Radius();
    }
    else if (sf.GetType() == GeomAbs_Sphere) {
        return sf.Sphere().Radius();
    }

    return 0.0;
}

double AssemblyObject::getEdgeRadius(App::DocumentObject* obj, const char* elt)
{
    auto base = static_cast<PartApp::Feature*>(obj);
    const PartApp::TopoShape& TopShape = base->Shape.getShape();

    // Check for valid face types
    TopoDS_Edge edge = TopoDS::Edge(TopShape.getSubShape(elt));
    BRepAdaptor_Curve sf(edge);

    if (sf.GetType() == GeomAbs_Circle) {
        return sf.Circle().Radius();
    }

    return 0.0;
}


// ======================================= Utils ======================================

void printPlacement(Base::Placement plc, const char* name)
{
    Base::Vector3d pos = plc.getPosition();
    Base::Vector3d axis;
    double angle;
    Base::Rotation rot = plc.getRotation();
    rot.getRawValue(axis, angle);
    Base::Console().Warning(
        "placement %s : position (%.1f, %.1f, %.1f) - axis (%.1f, %.1f, %.1f) angle %.1f\n",
        name,
        pos.x,
        pos.y,
        pos.z,
        axis.x,
        axis.y,
        axis.z,
        angle);
}

Base::Placement AssemblyObject::getPlacementFromProp(App::DocumentObject* obj, const char* propName)
{
    Base::Placement plc = Base::Placement();
    auto* propPlacement = dynamic_cast<App::PropertyPlacement*>(obj->getPropertyByName(propName));
    if (propPlacement) {
        plc = propPlacement->getValue();
    }
    return plc;
}

bool AssemblyObject::getTargetPlacementRelativeTo(Base::Placement& foundPlc,
                                                  App::DocumentObject* targetObj,
                                                  App::DocumentObject* part,
                                                  App::DocumentObject* container,
                                                  bool inContainerBranch,
                                                  bool ignorePlacement)
{
    inContainerBranch = inContainerBranch || (!ignorePlacement && part == container);

    if (targetObj == part && inContainerBranch && !ignorePlacement) {
        foundPlc = getPlacementFromProp(targetObj, "Placement");
        return true;
    }

    if (part->isDerivedFrom(App::DocumentObjectGroup::getClassTypeId())) {
        for (auto& obj : part->getOutList()) {
            bool found = getTargetPlacementRelativeTo(foundPlc,
                                                      targetObj,
                                                      obj,
                                                      container,
                                                      inContainerBranch,
                                                      ignorePlacement);
            if (found) {
                return true;
            }
        }
    }
    else if (part->isDerivedFrom(Assembly::AssemblyObject::getClassTypeId())
             || part->isDerivedFrom(App::Part::getClassTypeId())
             || part->isDerivedFrom(PartDesign::Body::getClassTypeId())) {
        for (auto& obj : part->getOutList()) {
            bool found = getTargetPlacementRelativeTo(foundPlc,
                                                      targetObj,
                                                      obj,
                                                      container,
                                                      inContainerBranch);
            if (!found) {
                continue;
            }

            if (!ignorePlacement) {
                foundPlc = getPlacementFromProp(part, "Placement") * foundPlc;
            }

            return true;
        }
    }
    else if (auto link = dynamic_cast<App::Link*>(part)) {
        auto linked_obj = link->getLinkedObject();

        if (dynamic_cast<App::Part*>(linked_obj) || dynamic_cast<AssemblyObject*>(linked_obj)) {
            for (auto& obj : linked_obj->getOutList()) {
                bool found = getTargetPlacementRelativeTo(foundPlc,
                                                          targetObj,
                                                          obj,
                                                          container,
                                                          inContainerBranch);
                if (!found) {
                    continue;
                }

                foundPlc = getPlacementFromProp(link, "Placement") * foundPlc;
                return true;
            }
        }

        bool found = getTargetPlacementRelativeTo(foundPlc,
                                                  targetObj,
                                                  linked_obj,
                                                  container,
                                                  inContainerBranch,
                                                  true);

        if (found) {
            if (!ignorePlacement) {
                foundPlc = getPlacementFromProp(link, "Placement") * foundPlc;
            }

            return true;
        }
    }

    return false;
}

Base::Placement AssemblyObject::getGlobalPlacement(App::DocumentObject* targetObj,
                                                   App::DocumentObject* container)
{
    bool inContainerBranch = (container == nullptr);
    auto rootObjects = App::GetApplication().getActiveDocument()->getRootObjects();
    for (auto& part : rootObjects) {
        Base::Placement foundPlc;
        bool found =
            getTargetPlacementRelativeTo(foundPlc, targetObj, part, container, inContainerBranch);
        if (found) {
            return foundPlc;
        }
    }

    return Base::Placement();
}

Base::Placement AssemblyObject::getGlobalPlacement(App::DocumentObject* joint,
                                                   const char* targetObj,
                                                   const char* container)
{
    App::DocumentObject* obj = getObjFromNameProp(joint, targetObj, container);
    App::DocumentObject* part = getLinkObjFromProp(joint, container);
    return getGlobalPlacement(obj, part);
}

double AssemblyObject::getJointDistance(App::DocumentObject* joint)
{
    double distance = 0.0;

    auto* prop = dynamic_cast<App::PropertyFloat*>(joint->getPropertyByName("Distance"));
    if (prop) {
        distance = prop->getValue();
    }

    return distance;
}

JointType AssemblyObject::getJointType(App::DocumentObject* joint)
{
    JointType jointType = JointType::Fixed;

    auto* prop = dynamic_cast<App::PropertyEnumeration*>(joint->getPropertyByName("JointType"));
    if (prop) {
        jointType = static_cast<JointType>(prop->getValue());
    }

    return jointType;
}

const char* AssemblyObject::getElementFromProp(App::DocumentObject* obj, const char* propName)
{
    auto* prop = dynamic_cast<App::PropertyString*>(obj->getPropertyByName(propName));
    if (!prop) {
        return "";
    }

    return prop->getValue();
}

std::string AssemblyObject::getElementTypeFromProp(App::DocumentObject* obj, const char* propName)
{
    // The prop is going to be something like 'Edge14' or 'Face7'. We need 'Edge' or 'Face'
    std::string elementType;
    for (char ch : std::string(getElementFromProp(obj, propName))) {
        if (std::isalpha(ch)) {
            elementType += ch;
        }
    }
    return elementType;
}

App::DocumentObject* AssemblyObject::getLinkObjFromProp(App::DocumentObject* joint,
                                                        const char* propLinkName)
{
    auto* propObj = dynamic_cast<App::PropertyLink*>(joint->getPropertyByName(propLinkName));
    if (!propObj) {
        Base::Console().Warning("getLinkObjFromProp nullptr\n");
        return nullptr;
    }
    return propObj->getValue();
}

App::DocumentObject* AssemblyObject::getObjFromNameProp(App::DocumentObject* joint,
                                                        const char* pObjName,
                                                        const char* pPart)
{
    auto* propObjName = dynamic_cast<App::PropertyString*>(joint->getPropertyByName(pObjName));
    if (!propObjName) {
        return nullptr;
    }
    std::string objName = std::string(propObjName->getValue());

    App::DocumentObject* containingPart = getLinkObjFromProp(joint, pPart);
    if (!containingPart) {
        return nullptr;
    }

    if (objName == containingPart->getNameInDocument()) {
        return containingPart;
    }

    if (containingPart->getTypeId().isDerivedFrom(App::Link::getClassTypeId())) {
        App::Link* link = dynamic_cast<App::Link*>(containingPart);

        containingPart = link->getLinkedObject();
        if (!containingPart) {
            return nullptr;
        }
    }

    for (auto obj : containingPart->getOutList()) {
        if (objName == obj->getNameInDocument()) {
            return obj;
        }
    }

    return nullptr;
}

App::DocumentObject* AssemblyObject::getLinkedObjFromNameProp(App::DocumentObject* joint,
                                                              const char* pObjName,
                                                              const char* pPart)
{
    auto* obj = getObjFromNameProp(joint, pObjName, pPart);
    if (obj) {
        return obj->getLinkedObject(true);
    }
    return nullptr;
}


/*void Part::handleChangedPropertyType(Base::XMLReader& reader, const char* TypeName, App::Property*
prop)
{
    App::Part::handleChangedPropertyType(reader, TypeName, prop);
}*/

/* Apparently not necessary as App::Part doesn't have this.
// Python Assembly feature ---------------------------------------------------------

namespace App
{
    /// @cond DOXERR
    PROPERTY_SOURCE_TEMPLATE(Assembly::AssemblyObjectPython, Assembly::AssemblyObject)
        template<>
    const char* Assembly::AssemblyObjectPython::getViewProviderName() const
    {
        return "AssemblyGui::ViewProviderAssembly";
    }
    template<>
    PyObject* Assembly::AssemblyObjectPython::getPyObject()
    {
        if (PythonObject.is(Py::_None())) {
            // ref counter is set to 1
            PythonObject = Py::Object(new FeaturePythonPyT<AssemblyObjectPy>(this), true);
        }
        return Py::new_reference_to(PythonObject);
    }
    /// @endcond

    // explicit template instantiation
    template class AssemblyExport FeaturePythonT<Assembly::AssemblyObject>;
}// namespace App*/
