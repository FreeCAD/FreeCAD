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
#include <cmath>
#include <vector>
#include <unordered_map>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObjectGroup.h>
#include <App/FeaturePythonPyImp.h>
#include <App/PropertyPythonObject.h>
#include <Base/Console.h>
#include <Base/Placement.h>
#include <Base/Rotation.h>
#include <Base/Tools.h>
#include <Base/Interpreter.h>

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
#include <OndselSolver/ASMTTime.h>
#include <OndselSolver/ASMTConstantGravity.h>

#include "AssemblyObject.h"
#include "AssemblyObjectPy.h"
#include "JointGroup.h"

using namespace App;
using namespace Assembly;
using namespace MbD;


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

std::vector<App::DocumentObject*> AssemblyObject::getJoints()
{
    std::vector<App::DocumentObject*> joints = {};

    App::Document* doc = getDocument();

    std::vector<DocumentObject*> jointGroups =
        doc->getObjectsOfType(Assembly::JointGroup::getClassTypeId());

    Base::PyGILStateLocker lock;
    if (jointGroups.size() > 0) {
        for (auto* obj : static_cast<App::DocumentObjectGroup*>(jointGroups[0])->getObjects()) {
            App::PropertyPythonObject* proxy = obj
                ? dynamic_cast<App::PropertyPythonObject*>(obj->getPropertyByName("Proxy"))
                : nullptr;
            if (proxy) {
                Py::Object joint = proxy->getValue();
                if (joint.hasAttr("setJointConnectors")) {
                    joints.push_back(obj);
                }
            }
        }
    }

    // Make sure the joints are up to date.
    recomputeJointPlacements(joints);

    return joints;
}

bool AssemblyObject::fixGroundedParts()
{
    App::Document* doc = getDocument();
    App::DocumentObject* jointsGroup = doc->getObject("Joints");

    bool onePartFixed = false;

    Base::PyGILStateLocker lock;
    if (jointsGroup && jointsGroup->isDerivedFrom(App::DocumentObjectGroup::getClassTypeId())) {
        for (auto* obj : static_cast<App::DocumentObjectGroup*>(jointsGroup)->getObjects()) {
            auto* propObj =
                dynamic_cast<App::PropertyLink*>(obj->getPropertyByName("ObjectToGround"));
            if (propObj) {
                App::DocumentObject* objToGround = propObj->getValue();

                Base::Placement plc = getPlacementFromProp(obj, "Placement");
                std::string str = obj->getFullName();
                fixGroundedPart(objToGround, plc, str);
                onePartFixed = true;
            }
        }
    }
    return onePartFixed;
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
    auto mbdMarker2 = makeMbdMarker(markerName2, plc);
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
        std::shared_ptr<ASMTJoint> mbdJoint = makeMbdJoint(joint);
        mbdAssembly->addJoint(mbdJoint);
    }
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

int AssemblyObject::solve()
{
    // Base::Console().Warning("solve\n");
    mbdAssembly = makeMbdAssembly();
    objectPartMap.clear();

    if (!fixGroundedParts()) {
        // If no part fixed we can't solve.
        return -6;
    }

    std::vector<App::DocumentObject*> joints = getJoints();

    jointParts(joints);

    try {
        mbdAssembly->solve();
    }
    catch (...) {
        Base::Console().Error("Solve failed\n");
        return -1;
    }

    setNewPlacements();

    // The Placement1 and Placement2 of each joint needs to be updated as the parts moved.
    // Note calling only recomputeJointPlacements makes a weird illegal storage access
    // When solving while moving part. Happens in Py::Callable(attr).apply();
    // it apparantly can't access the JointObject 'updateJCSPlacements' function.
    getJoints();

    return 0;
}

void AssemblyObject::exportAsASMT(std::string fileName)
{
    Base::Console().Warning("hello 1\n");
    mbdAssembly = makeMbdAssembly();
    objectPartMap.clear();
    Base::Console().Warning("hello 2\n");
    fixGroundedParts();

    std::vector<App::DocumentObject*> joints = getJoints();

    Base::Console().Warning("hello 3\n");
    jointParts(joints);

    Base::Console().Warning("hello 4\n");
    Base::Console().Warning("%s\n", fileName.c_str());
    mbdAssembly->outputFile(fileName);
    Base::Console().Warning("hello 5\n");
}

std::shared_ptr<ASMTJoint> AssemblyObject::makeMbdJointOfType(JointType jointType)
{
    std::shared_ptr<ASMTJoint> mbdJoint;

    if (jointType == JointType::Fixed) {
        mbdJoint = CREATE<ASMTFixedJoint>::With();
    }
    else if (jointType == JointType::Revolute) {
        mbdJoint = CREATE<ASMTRevoluteJoint>::With();
    }
    else if (jointType == JointType::Cylindrical) {
        mbdJoint = CREATE<ASMTCylindricalJoint>::With();
    }
    else if (jointType == JointType::Slider) {
        mbdJoint = CREATE<ASMTTranslationalJoint>::With();
    }
    else if (jointType == JointType::Ball) {
        mbdJoint = CREATE<ASMTSphericalJoint>::With();
    }
    else if (jointType == JointType::Planar) {
        mbdJoint = CREATE<ASMTPointInPlaneJoint>::With();
    }
    else if (jointType == JointType::Parallel) {
        // TODO
        mbdJoint = CREATE<ASMTFixedJoint>::With();
    }
    else if (jointType == JointType::Tangent) {
        // TODO
        mbdJoint = CREATE<ASMTFixedJoint>::With();
    }

    return mbdJoint;
}

std::shared_ptr<ASMTJoint> AssemblyObject::makeMbdJoint(App::DocumentObject* joint)
{
    JointType jointType = JointType::Fixed;

    auto* prop = joint
        ? dynamic_cast<App::PropertyEnumeration*>(joint->getPropertyByName("JointType"))
        : nullptr;
    if (prop) {
        jointType = static_cast<JointType>(prop->getValue());
    }

    std::shared_ptr<ASMTJoint> mbdJoint = makeMbdJointOfType(jointType);

    std::string fullMarkerName1 = handleOneSideOfJoint(joint, "Object1", "Placement1");
    std::string fullMarkerName2 = handleOneSideOfJoint(joint, "Object2", "Placement2");

    mbdJoint->setMarkerI(fullMarkerName1);
    mbdJoint->setMarkerJ(fullMarkerName2);

    return mbdJoint;
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

std::string AssemblyObject::handleOneSideOfJoint(App::DocumentObject* joint,
                                                 const char* propLinkName,
                                                 const char* propPlcName)
{
    auto* propObj = dynamic_cast<App::PropertyLink*>(joint->getPropertyByName(propLinkName));
    if (!propObj) {
        return nullptr;
    }
    App::DocumentObject* obj = propObj->getValue();

    std::shared_ptr<ASMTPart> mbdPart = getMbDPart(obj);
    Base::Placement objPlc = getPlacementFromProp(obj, "Placement");
    Base::Placement plc = getPlacementFromProp(joint, propPlcName);
    // Now we have plc which is the JCS placement, but its relative to the doc origin, not to the
    // obj.

    plc = objPlc.inverse() * plc;

    std::string markerName = joint->getFullName();
    auto mbdMarker = makeMbdMarker(markerName, plc);
    mbdPart->addMarker(mbdMarker);

    return "/OndselAssembly/" + mbdPart->name + "/" + markerName;
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
        if (propPlacement) {

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

/*void Part::handleChangedPropertyType(Base::XMLReader& reader, const char* TypeName, App::Property*
prop)
{
    App::Part::handleChangedPropertyType(reader, TypeName, prop);
}*/

/* Apparantly not necessary as App::Part doesn't have this.
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
