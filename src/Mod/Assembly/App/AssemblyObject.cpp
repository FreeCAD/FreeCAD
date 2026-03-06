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

#include <boost/core/ignore_unused.hpp>
#include <cmath>
#include <vector>
#include <unordered_map>


#include <App/Application.h>
#include <App/Datums.h>
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

#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/AttachExtension.h>

#include <OndselSolver/CREATE.h>
#include <OndselSolver/ASMTSimulationParameters.h>
#include <OndselSolver/ASMTAssembly.h>
#include <OndselSolver/ASMTMarker.h>
#include <OndselSolver/ASMTPart.h>
#include <OndselSolver/ASMTJoint.h>
#include <OndselSolver/ASMTAngleJoint.h>
#include <OndselSolver/ASMTFixedJoint.h>
#include <OndselSolver/ASMTGearJoint.h>
#include <OndselSolver/ASMTRevoluteJoint.h>
#include <OndselSolver/ASMTCylindricalJoint.h>
#include <OndselSolver/ASMTTranslationalJoint.h>
#include <OndselSolver/ASMTSphericalJoint.h>
#include <OndselSolver/ASMTParallelAxesJoint.h>
#include <OndselSolver/ASMTPerpendicularJoint.h>
#include <OndselSolver/ASMTPointInPlaneJoint.h>
#include <OndselSolver/ASMTPointInLineJoint.h>
#include <OndselSolver/ASMTLineInPlaneJoint.h>
#include <OndselSolver/ASMTPlanarJoint.h>
#include <OndselSolver/ASMTRevCylJoint.h>
#include <OndselSolver/ASMTCylSphJoint.h>
#include <OndselSolver/ASMTRackPinionJoint.h>
#include <OndselSolver/ASMTRotationLimit.h>
#include <OndselSolver/ASMTTranslationLimit.h>
#include <OndselSolver/ASMTRotationalMotion.h>
#include <OndselSolver/ASMTTranslationalMotion.h>
#include <OndselSolver/ASMTGeneralMotion.h>
#include <OndselSolver/ASMTScrewJoint.h>
#include <OndselSolver/ASMTSphSphJoint.h>
#include <OndselSolver/ASMTTime.h>
#include <OndselSolver/ASMTConstantGravity.h>
#include <OndselSolver/ExternalSystem.h>
#include <OndselSolver/enum.h>

#include "AssemblyLink.h"
#include "AssemblyObject.h"
#include "AssemblyObjectPy.h"
#include "AssemblyUtils.h"
#include "JointGroup.h"
#include "ViewGroup.h"

FC_LOG_LEVEL_INIT("Assembly", true, true, true)

using namespace Assembly;
using namespace MbD;


namespace PartApp = Part;


// ================================ Assembly Object ============================

PROPERTY_SOURCE(Assembly::AssemblyObject, App::Part)

AssemblyObject::AssemblyObject()
    : mbdAssembly(std::make_shared<ASMTAssembly>())
    , bundleFixed(false)
    , lastDoF(0)
    , lastHasConflict(false)
    , lastHasRedundancies(false)
    , lastHasPartialRedundancies(false)
    , lastHasMalformedConstraints(false)
    , lastSolverStatus(0)
{
    mbdAssembly->externalSystem->freecadAssemblyObject = this;

    lastDoF = numberOfComponents() * 6;
    signalSolverUpdate();
}

AssemblyObject::~AssemblyObject() = default;

PyObject* AssemblyObject::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new AssemblyObjectPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

App::DocumentObjectExecReturn* AssemblyObject::execute()
{
    App::DocumentObjectExecReturn* ret = App::Part::execute();

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Assembly"
    );
    if (hGrp->GetBool("SolveOnRecompute", true)) {
        solve(false, false);  // No need to update jcs since recompute updated them.
    }
    return ret;
}

void AssemblyObject::onChanged(const App::Property* prop)
{
    if (prop == &Group) {
        updateSolveStatus();
    }
    App::Part::onChanged(prop);
}

int AssemblyObject::solve(bool enableRedo, bool updateJCS)
{
    ensureIdentityPlacements();

    mbdAssembly = makeMbdAssembly();
    objectPartMap.clear();
    motions.clear();

    auto groundedObjs = fixGroundedParts();
    if (groundedObjs.empty()) {
        // If no part fixed we can't solve.
        return -6;
    }

    std::vector<App::DocumentObject*> joints = getJoints(updateJCS);

    removeUnconnectedJoints(joints, groundedObjs);

    jointParts(joints);

    if (enableRedo) {
        savePlacementsForUndo();
    }

    try {
        mbdAssembly->runPreDrag();
        lastSolverStatus = 0;
    }
    catch (const std::exception& e) {
        FC_ERR("Solve failed: " << e.what());
        lastSolverStatus = -1;
        updateSolveStatus();
        return -1;
    }
    catch (...) {
        FC_ERR("Solve failed: unhandled exception");
        lastSolverStatus = -1;
        updateSolveStatus();
        return -1;
    }

    setNewPlacements();

    redrawJointPlacements(joints);

    updateSolveStatus();

    return 0;
}

void AssemblyObject::updateSolveStatus()
{
    lastRedundantJoints.clear();
    lastHasRedundancies = false;
    //+1 because there's a grounded joint to origin
    lastDoF = (1 + numberOfComponents()) * 6;

    if (!mbdAssembly || !mbdAssembly->mbdSystem) {
        solve();
    }

    if (!mbdAssembly || !mbdAssembly->mbdSystem) {
        return;
    }

    // Helper lambda to clean up the joint name from the solver
    auto cleanJointName = [](const std::string& rawName) -> std::string {
        // rawName is like : /OndselAssembly/ground_moves#Joint001
        size_t hashPos = rawName.find_last_of('#');
        if (hashPos != std::string::npos) {
            // Return the substring after the '#'
            return rawName.substr(hashPos + 1);
        }
        return rawName;
    };


    // Iterate through all joints and motions in the MBD system
    mbdAssembly->mbdSystem->jointsMotionsDo([&](std::shared_ptr<MbD::Joint> jm) {
        if (!jm) {
            return;
        }
        // Base::Console().warning("jm->name %s\n", jm->name);
        bool isJointRedundant = false;

        jm->constraintsDo([&](std::shared_ptr<MbD::Constraint> con) {
            if (!con) {
                return;
            }

            std::string spec = con->constraintSpec();
            // A constraint is redundant if its spec starts with "Redundant"
            if (spec.rfind("Redundant", 0) == 0) {
                isJointRedundant = true;
            }
            // Base::Console().warning("    - %s\n", spec);
            --lastDoF;
        });

        const std::string fullName = cleanJointName(jm->name);
        App::DocumentObject* docObj = getDocument()->getObject(fullName.c_str());

        // We only care about objects that are actual joints in the FreeCAD document.
        // This effectively filters out the grounding joints, which are named after parts.
        if (!docObj || !docObj->getPropertyByName("Reference1")) {
            return;
        }

        if (isJointRedundant) {
            // Check if this joint is already in the list to avoid duplicates
            std::string objName = docObj->getNameInDocument();
            if (std::find(lastRedundantJoints.begin(), lastRedundantJoints.end(), objName)
                == lastRedundantJoints.end()) {
                lastRedundantJoints.push_back(objName);
            }
        }
    });

    // Update the summary boolean flag
    if (!lastRedundantJoints.empty()) {
        lastHasRedundancies = true;
    }

    signalSolverUpdate();
}

int AssemblyObject::generateSimulation(App::DocumentObject* sim)
{
    mbdAssembly = makeMbdAssembly();
    objectPartMap.clear();

    motions = getMotionsFromSimulation(sim);

    auto groundedObjs = fixGroundedParts();
    if (groundedObjs.empty()) {
        // If no part fixed we can't solve.
        return -6;
    }

    std::vector<App::DocumentObject*> joints = getJoints();

    removeUnconnectedJoints(joints, groundedObjs);

    jointParts(joints);

    create_mbdSimulationParameters(sim);

    try {
        mbdAssembly->runKINEMATIC();
    }
    catch (...) {
        Base::Console().error("Generation of simulation failed\n");
        motions.clear();
        return -1;
    }

    motions.clear();

    return 0;
}

std::vector<App::DocumentObject*> AssemblyObject::getMotionsFromSimulation(App::DocumentObject* sim)
{
    if (!sim) {
        return {};
    }

    auto* prop = dynamic_cast<App::PropertyLinkList*>(sim->getPropertyByName("Group"));
    if (!prop) {
        return {};
    }

    return prop->getValue();
}

int Assembly::AssemblyObject::updateForFrame(size_t index, bool updateJCS)
{
    if (!mbdAssembly) {
        return -1;
    }

    auto nfrms = mbdAssembly->numberOfFrames();
    if (index >= nfrms) {
        return -1;
    }

    mbdAssembly->updateForFrame(index);
    setNewPlacements();
    auto jointDocs = getJoints(updateJCS);
    redrawJointPlacements(jointDocs);
    return 0;
}

size_t Assembly::AssemblyObject::numberOfFrames()
{
    return mbdAssembly->numberOfFrames();
}

void AssemblyObject::preDrag(std::vector<App::DocumentObject*> dragParts)
{
    bundleFixed = true;
    solve();
    bundleFixed = false;

    draggedParts.clear();
    for (auto part : dragParts) {
        // make sure no duplicate
        if (std::ranges::find(draggedParts, part) != draggedParts.end()) {
            continue;
        }

        // Free-floating parts should not be added since they are ignored by the solver!
        if (!isPartConnected(part)) {
            continue;
        }

        // Some objects have been bundled, we don't want to add these to dragged parts
        Base::Placement plc;
        for (auto& pair : objectPartMap) {
            App::DocumentObject* parti = pair.first;
            if (parti != part) {
                continue;
            }
            plc = pair.second.offsetPlc;
        }
        if (!plc.isIdentity()) {
            // If not identity, then it's a bundled object. Some bundled objects may
            // have identity placement if they have the same position as the main object of
            // the bundle. But they're not going to be a problem.
            continue;
        }

        draggedParts.push_back(part);
    }
}

void AssemblyObject::doDragStep()
{
    try {
        std::vector<std::shared_ptr<MbD::ASMTPart>> dragMbdParts;

        for (auto& part : draggedParts) {
            if (!part) {
                continue;
            }

            auto mbdPart = getMbDPart(part);
            dragMbdParts.push_back(mbdPart);

            // Update the MBD part's position
            Base::Placement plc = getPlacementFromProp(part, "Placement");
            Base::Vector3d pos = plc.getPosition();
            mbdPart->updateMbDFromPosition3D(
                std::make_shared<FullColumn<double>>(ListD {pos.x, pos.y, pos.z})
            );

            // Update the MBD part's rotation
            Base::Rotation rot = plc.getRotation();
            Base::Matrix4D mat;
            rot.getValue(mat);
            Base::Vector3d r0 = mat.getRow(0);
            Base::Vector3d r1 = mat.getRow(1);
            Base::Vector3d r2 = mat.getRow(2);
            mbdPart->updateMbDFromRotationMatrix(r0.x, r0.y, r0.z, r1.x, r1.y, r1.z, r2.x, r2.y, r2.z);
        }

        // Timing mbdAssembly->runDragStep()
        auto dragPartsVec = std::make_shared<std::vector<std::shared_ptr<ASMTPart>>>(dragMbdParts);
        mbdAssembly->runDragStep(dragPartsVec);

        // Timing the validation and placement setting
        if (validateNewPlacements()) {
            setNewPlacements();

            auto joints = getJoints(false);
            for (auto* joint : joints) {
                if (joint->Visibility.getValue()) {
                    // redraw only the moving joint as its quite slow as its python code.
                    redrawJointPlacement(joint);
                }
            }
        }
    }
    catch (...) {
        // We do nothing if a solve step fails.
    }
}

Base::Placement AssemblyObject::getMbdPlacement(std::shared_ptr<ASMTPart> mbdPart)
{
    if (!mbdPart) {
        return Base::Placement();
    }

    double x, y, z;
    mbdPart->getPosition3D(x, y, z);
    Base::Vector3d pos = Base::Vector3d(x, y, z);

    double q0, q1, q2, q3;
    mbdPart->getQuarternions(q3, q0, q1, q2);
    Base::Rotation rot = Base::Rotation(q0, q1, q2, q3);

    return Base::Placement(pos, rot);
}

bool AssemblyObject::validateNewPlacements()
{
    // First we check if a grounded object has moved. It can happen that they flip.
    auto groundedParts = getGroundedParts();
    for (auto* obj : groundedParts) {
        auto* propPlacement = obj->getPlacementProperty();
        if (propPlacement) {
            Base::Placement oldPlc = propPlacement->getValue();

            auto it = objectPartMap.find(obj);
            if (it != objectPartMap.end()) {
                std::shared_ptr<MbD::ASMTPart> mbdPart = it->second.part;
                Base::Placement newPlacement = getMbdPlacement(mbdPart);
                if (!it->second.offsetPlc.isIdentity()) {
                    newPlacement = newPlacement * it->second.offsetPlc;
                }

                if (!oldPlc.isSame(newPlacement, Precision::Confusion())) {
                    Base::Console().warning(
                        "Assembly : Ignoring bad solve, a grounded object (%s) moved.\n",
                        obj->getFullLabel()
                    );
                    return false;
                }
            }
        }
    }

    // TODO: We could do further tests
    // For example check if the joints connectors are correctly aligned.
    return true;
}

void AssemblyObject::postDrag()
{
    mbdAssembly->runPostDrag();  // Do this after last drag
    purgeTouched();
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
        auto* propPlc = obj->getPlacementProperty();
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
        auto* propPlacement = obj->getPlacementProperty();
        if (!propPlacement) {
            continue;
        }

        propPlacement->setValue(pair.second);
    }
    previousPositions.clear();

    // update joint placements:
    getJoints(/*updateJCS*/ true, /*delBadJoints*/ false);
}

void AssemblyObject::clearUndo()
{
    previousPositions.clear();
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

void AssemblyObject::setNewPlacements()
{
    for (auto& pair : objectPartMap) {
        App::DocumentObject* obj = pair.first;
        std::shared_ptr<ASMTPart> mbdPart = pair.second.part;

        if (!obj || !mbdPart) {
            continue;
        }

        // Check if the object has a "Placement" property
        auto* propPlacement = obj->getPlacementProperty();
        if (!propPlacement) {
            continue;
        }


        Base::Placement newPlacement = getMbdPlacement(mbdPart);
        if (!pair.second.offsetPlc.isIdentity()) {
            newPlacement = newPlacement * pair.second.offsetPlc;
        }
        if (!propPlacement->getValue().isSame(newPlacement)) {
            propPlacement->setValue(newPlacement);
            obj->purgeTouched();
        }
    }
}

void AssemblyObject::redrawJointPlacements(std::vector<App::DocumentObject*> joints)
{
    // Notify the joint objects that the transform of the coin object changed.
    for (auto* joint : joints) {
        if (!joint) {
            continue;
        }
        redrawJointPlacement(joint);
    }
}

void AssemblyObject::redrawJointPlacement(App::DocumentObject* joint)
{
    if (!joint) {
        return;
    }

    Base::PyGILStateLocker lock;

    App::PropertyPythonObject* proxy = joint
        ? dynamic_cast<App::PropertyPythonObject*>(joint->getPropertyByName("Proxy"))
        : nullptr;

    if (!proxy) {
        return;
    }

    Py::Object jointPy = proxy->getValue();

    if (!jointPy.hasAttr("redrawJointPlacements")) {
        return;
    }

    Py::Object attr = jointPy.getAttr("redrawJointPlacements");
    if (attr.ptr() && attr.isCallable()) {
        Py::Tuple args(1);
        args.setItem(0, Py::asObject(joint->getPyObject()));
        Py::Callable(attr).apply(args);
    }
}

std::shared_ptr<ASMTAssembly> AssemblyObject::makeMbdAssembly()
{
    auto assembly = CREATE<ASMTAssembly>::With();
    assembly->externalSystem->freecadAssemblyObject = this;
    assembly->setName("OndselAssembly");

    ParameterGrp::handle hPgr = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Assembly"
    );

    assembly->setDebug(hPgr->GetBool("LogSolverDebug", false));
    return assembly;
}

App::DocumentObject* AssemblyObject::getJointOfPartConnectingToGround(
    App::DocumentObject* part,
    std::string& name,
    const std::vector<App::DocumentObject*>& excludeJoints
)
{
    if (!part) {
        return nullptr;
    }

    std::vector<App::DocumentObject*> joints = getJointsOfPart(part);

    for (auto joint : joints) {
        if (!joint) {
            continue;
        }

        if (std::ranges::find(excludeJoints, joint) != excludeJoints.end()) {
            continue;
        }

        App::DocumentObject* part1 = getMovingPartFromRef(joint, "Reference1");
        App::DocumentObject* part2 = getMovingPartFromRef(joint, "Reference2");
        if (!part1 || !part2) {
            continue;
        }

        if (part == part1 && isJointConnectingPartToGround(joint, "Reference1")) {
            name = "Reference1";
            return joint;
        }
        if (part == part2 && isJointConnectingPartToGround(joint, "Reference2")) {
            name = "Reference2";
            return joint;
        }
    }
    return nullptr;
}

template<typename T>
T* AssemblyObject::getGroup()
{
    App::Document* doc = getDocument();

    std::vector<DocumentObject*> groups = doc->getObjectsOfType(T::getClassTypeId());
    if (groups.empty()) {
        return nullptr;
    }
    for (auto group : groups) {
        if (hasObject(group)) {
            return freecad_cast<T*>(group);
        }
    }
    return nullptr;
}

JointGroup* AssemblyObject::getJointGroup() const
{
    return Assembly::getJointGroup(this);
}

ViewGroup* AssemblyObject::getExplodedViewGroup() const
{
    App::Document* doc = getDocument();

    std::vector<DocumentObject*> viewGroups = doc->getObjectsOfType(ViewGroup::getClassTypeId());
    if (viewGroups.empty()) {
        return nullptr;
    }
    for (auto viewGroup : viewGroups) {
        if (hasObject(viewGroup)) {
            return freecad_cast<ViewGroup*>(viewGroup);
        }
    }
    return nullptr;
}

std::vector<App::DocumentObject*> AssemblyObject::getJoints(bool updateJCS, bool delBadJoints, bool subJoints)
{
    std::vector<App::DocumentObject*> joints = {};

    JointGroup* jointGroup = getJointGroup();
    if (!jointGroup) {
        return {};
    }

    Base::PyGILStateLocker lock;
    for (auto joint : jointGroup->getObjects()) {
        if (!joint) {
            continue;
        }

        auto* prop = dynamic_cast<App::PropertyBool*>(joint->getPropertyByName("Suppressed"));
        if (joint->isError() || !prop || prop->getValue()) {
            // Filter grounded joints and deactivated joints.
            continue;
        }

        auto* part1 = getMovingPartFromRef(joint, "Reference1");
        auto* part2 = getMovingPartFromRef(joint, "Reference2");
        if (!part1 || !part2 || part1->getFullName() == part2->getFullName()) {
            // Remove incomplete joints. Left-over when the user deletes a part.
            // Remove incoherent joints (self-pointing joints)
            if (delBadJoints) {
                getDocument()->removeObject(joint->getNameInDocument());
            }
            continue;
        }

        auto proxy = dynamic_cast<App::PropertyPythonObject*>(joint->getPropertyByName("Proxy"));
        if (proxy) {
            if (proxy->getValue().hasAttr("setJointConnectors")) {
                joints.push_back(joint);
            }
        }
    }

    // add sub assemblies joints.
    if (subJoints) {
        for (auto& assembly : getSubAssemblies()) {
            auto subJoints = assembly->getJoints();
            joints.insert(joints.end(), subJoints.begin(), subJoints.end());
        }
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

std::vector<App::DocumentObject*> AssemblyObject::getJointsOfObj(App::DocumentObject* obj)
{
    if (!obj) {
        return {};
    }

    std::vector<App::DocumentObject*> joints = getJoints(false);
    std::vector<App::DocumentObject*> jointsOf;

    for (auto joint : joints) {
        App::DocumentObject* obj1 = getObjFromJointRef(joint, "Reference1");
        App::DocumentObject* obj2 = getObjFromJointRef(joint, "Reference2");
        if (obj == obj1 || obj == obj2) {
            jointsOf.push_back(joint);
        }
    }

    return jointsOf;
}

std::vector<App::DocumentObject*> AssemblyObject::getJointsOfPart(App::DocumentObject* part)
{
    if (!part) {
        return {};
    }

    std::vector<App::DocumentObject*> joints = getJoints(false);
    std::vector<App::DocumentObject*> jointsOf;

    for (auto joint : joints) {
        App::DocumentObject* part1 = getMovingPartFromRef(joint, "Reference1");
        App::DocumentObject* part2 = getMovingPartFromRef(joint, "Reference2");
        if (part == part1 || part == part2) {
            jointsOf.push_back(joint);
        }
    }
    return jointsOf;
}

std::unordered_set<App::DocumentObject*> AssemblyObject::getGroundedParts()
{
    std::vector<App::DocumentObject*> groundedJoints = getGroundedJoints();

    std::unordered_set<App::DocumentObject*> groundedSet;
    for (auto gJoint : groundedJoints) {
        if (!gJoint) {
            continue;
        }

        auto* propObj = dynamic_cast<App::PropertyLink*>(gJoint->getPropertyByName("ObjectToGround"));

        if (propObj) {
            App::DocumentObject* objToGround = propObj->getValue();
            if (objToGround) {
                if (auto* asmLink = dynamic_cast<AssemblyLink*>(objToGround)) {
                    if (!asmLink->isRigid()) {
                        continue;
                    }
                }
                groundedSet.insert(objToGround);
            }
        }
    }

    // We also need to add all the root-level datums objects that are not attached.
    std::vector<App::DocumentObject*> objs = Group.getValues();
    for (auto* obj : objs) {
        if (obj->isDerivedFrom<App::LocalCoordinateSystem>()
            || obj->isDerivedFrom<App::DatumElement>()) {
            auto* pcAttach = obj->getExtensionByType<PartApp::AttachExtension>();
            if (pcAttach) {
                // If it's a Part datums, we check if it's attached. If yes then we ignore it.
                std::string mode = pcAttach->MapMode.getValueAsString();
                if (mode != "Deactivated") {
                    continue;
                }
            }
            groundedSet.insert(obj);
        }
    }

    // Origin is not in Group so we add it separately
    groundedSet.insert(Origin.getValue());

    return groundedSet;
}

std::unordered_set<App::DocumentObject*> AssemblyObject::fixGroundedParts()
{
    auto groundedParts = getGroundedParts();

    for (auto obj : groundedParts) {
        if (!obj) {
            continue;
        }

        Base::Placement plc = getPlacementFromProp(obj, "Placement");
        std::string str = obj->getFullName();
        fixGroundedPart(obj, plc, str);
    }
    return groundedParts;
}

void AssemblyObject::fixGroundedPart(App::DocumentObject* obj, Base::Placement& plc, std::string& name)
{
    if (!obj) {
        return;
    }

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

bool AssemblyObject::isJointConnectingPartToGround(App::DocumentObject* joint, const char* propname)
{
    if (!joint || !isJointTypeConnecting(joint)) {
        return false;
    }

    App::DocumentObject* part = getMovingPartFromRef(joint, propname);
    if (!part) {
        return false;
    }

    // Check if the part is grounded.
    bool isGrounded = isPartGrounded(part);
    if (isGrounded) {
        return false;
    }

    // Check if the part is disconnected even with the joint
    bool isConnected = isPartConnected(part);
    if (!isConnected) {
        return false;
    }

    // to know if a joint is connecting to ground we disable all the other joints
    std::vector<App::DocumentObject*> jointsOfPart = getJointsOfPart(part);
    std::vector<bool> activatedStates;

    for (auto jointi : jointsOfPart) {
        if (jointi->getFullName() == joint->getFullName()) {
            continue;
        }

        activatedStates.push_back(getJointActivated(jointi));
        setJointActivated(jointi, false);
    }

    isConnected = isPartConnected(part);

    // restore activation states
    for (auto jointi : jointsOfPart) {
        if (jointi->getFullName() == joint->getFullName() || activatedStates.empty()) {
            continue;
        }

        setJointActivated(jointi, activatedStates[0]);
        activatedStates.erase(activatedStates.begin());
    }

    return isConnected;
}

bool AssemblyObject::isJointTypeConnecting(App::DocumentObject* joint)
{
    if (!joint) {
        return false;
    }

    JointType jointType = getJointType(joint);
    return jointType != JointType::RackPinion && jointType != JointType::Screw
        && jointType != JointType::Gears && jointType != JointType::Belt;
}


bool AssemblyObject::isObjInSetOfObjRefs(App::DocumentObject* obj, const std::vector<ObjRef>& set)
{
    if (!obj) {
        return false;
    }

    for (const auto& pair : set) {
        if (pair.obj == obj) {
            return true;
        }
    }
    return false;
}

void AssemblyObject::removeUnconnectedJoints(
    std::vector<App::DocumentObject*>& joints,
    std::unordered_set<App::DocumentObject*> groundedObjs
)
{
    std::vector<ObjRef> connectedParts;

    // Initialize connectedParts with groundedObjs
    for (auto* groundedObj : groundedObjs) {
        connectedParts.push_back({groundedObj, nullptr});
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
            [&](App::DocumentObject* joint) {
                App::DocumentObject* obj1 = getMovingPartFromRef(joint, "Reference1");
                App::DocumentObject* obj2 = getMovingPartFromRef(joint, "Reference2");
                return (
                    !isObjInSetOfObjRefs(obj1, connectedParts)
                    || !isObjInSetOfObjRefs(obj2, connectedParts)
                );
            }
        ),
        joints.end()
    );
}

void AssemblyObject::traverseAndMarkConnectedParts(
    App::DocumentObject* currentObj,
    std::vector<ObjRef>& connectedParts,
    const std::vector<App::DocumentObject*>& joints
)
{
    // getConnectedParts returns the objs connected to the currentObj by any joint
    auto connectedObjs = getConnectedParts(currentObj, joints);
    for (auto& nextObjRef : connectedObjs) {
        if (!isObjInSetOfObjRefs(nextObjRef.obj, connectedParts)) {
            // Create a new ObjRef with the nextObj and a nullptr for PropertyXLinkSub*
            connectedParts.push_back(nextObjRef);
            traverseAndMarkConnectedParts(nextObjRef.obj, connectedParts, joints);
        }
    }
}

std::vector<ObjRef> AssemblyObject::getConnectedParts(
    App::DocumentObject* part,
    const std::vector<App::DocumentObject*>& joints
)
{
    if (!part) {
        return {};
    }

    std::vector<ObjRef> connectedParts;

    for (auto joint : joints) {
        if (!isJointTypeConnecting(joint)) {
            continue;
        }

        App::DocumentObject* obj1 = getMovingPartFromRef(joint, "Reference1");
        App::DocumentObject* obj2 = getMovingPartFromRef(joint, "Reference2");

        if (!obj1 || !obj2) {
            continue;
        }

        if (obj1 == part) {
            auto* ref = dynamic_cast<App::PropertyXLinkSub*>(joint->getPropertyByName("Reference2"));
            if (!ref) {
                continue;
            }
            connectedParts.push_back({obj2, ref});
        }
        else if (obj2 == part) {
            auto* ref = dynamic_cast<App::PropertyXLinkSub*>(joint->getPropertyByName("Reference1"));
            if (!ref) {
                continue;
            }
            connectedParts.push_back({obj1, ref});
        }
    }
    return connectedParts;
}

bool AssemblyObject::isPartGrounded(App::DocumentObject* obj)
{
    if (!obj) {
        return false;
    }

    auto groundedObjs = getGroundedParts();

    for (auto* groundedObj : groundedObjs) {
        if (groundedObj->getFullName() == obj->getFullName()) {
            return true;
        }
    }

    return false;
}

bool AssemblyObject::isPartConnected(App::DocumentObject* obj)
{
    if (!obj) {
        return false;
    }

    auto groundedObjs = getGroundedParts();
    std::vector<App::DocumentObject*> joints = getJoints(false);

    std::vector<ObjRef> connectedParts;

    // Initialize connectedParts with groundedObjs
    for (auto* groundedObj : groundedObjs) {
        connectedParts.push_back({groundedObj, nullptr});
    }

    // Perform a traversal from each grounded object
    for (auto* groundedObj : groundedObjs) {
        traverseAndMarkConnectedParts(groundedObj, connectedParts, joints);
    }

    for (auto& objRef : connectedParts) {
        if (obj == objRef.obj) {
            return true;
        }
    }

    return false;
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

void Assembly::AssemblyObject::create_mbdSimulationParameters(App::DocumentObject* sim)
{
    auto mbdSim = mbdAssembly->simulationParameters;
    if (!sim) {
        return;
    }
    auto valueOf = [](DocumentObject* docObj, const char* propName) {
        auto* prop = dynamic_cast<App::PropertyFloat*>(docObj->getPropertyByName(propName));
        if (!prop) {
            return 0.0;
        }
        return prop->getValue();
    };
    mbdSim->settstart(valueOf(sim, "aTimeStart"));
    mbdSim->settend(valueOf(sim, "bTimeEnd"));
    mbdSim->sethout(valueOf(sim, "cTimeStepOutput"));
    mbdSim->sethmin(1.0e-9);
    mbdSim->sethmax(1.0);
    mbdSim->seterrorTol(valueOf(sim, "fGlobalErrorTolerance"));
}

std::shared_ptr<ASMTJoint> AssemblyObject::makeMbdJointOfType(App::DocumentObject* joint, JointType type)
{
    switch (type) {
        case JointType::Fixed:
            if (bundleFixed) {
                return nullptr;
            }
            return CREATE<ASMTFixedJoint>::With();

        case JointType::Revolute:
            return CREATE<ASMTRevoluteJoint>::With();

        case JointType::Cylindrical:
            return CREATE<ASMTCylindricalJoint>::With();

        case JointType::Slider:
            return CREATE<ASMTTranslationalJoint>::With();

        case JointType::Ball:
            return CREATE<ASMTSphericalJoint>::With();

        case JointType::Distance:
            return makeMbdJointDistance(joint);

        case JointType::Parallel:
            return CREATE<ASMTParallelAxesJoint>::With();

        case JointType::Perpendicular:
            return CREATE<ASMTPerpendicularJoint>::With();

        case JointType::Angle: {
            double angle = fabs(Base::toRadians(getJointAngle(joint)));
            if (fmod(angle, 2 * std::numbers::pi) < Precision::Confusion()) {
                return CREATE<ASMTParallelAxesJoint>::With();
            }
            auto mbdJoint = CREATE<ASMTAngleJoint>::With();
            mbdJoint->theIzJz = angle;
            return mbdJoint;
        }

        case JointType::RackPinion: {
            auto mbdJoint = CREATE<ASMTRackPinionJoint>::With();
            mbdJoint->pitchRadius = getJointDistance(joint);
            return mbdJoint;
        }

        case JointType::Screw: {
            int slidingIndex = slidingPartIndex(joint);
            if (slidingIndex == 0) {  // invalid this joint needs a slider
                return nullptr;
            }

            if (slidingIndex != 1) {
                swapJCS(joint);  // make sure that sliding is first.
            }

            auto mbdJoint = CREATE<ASMTScrewJoint>::With();
            mbdJoint->pitch = getJointDistance(joint);
            return mbdJoint;
        }

        case JointType::Gears: {
            auto mbdJoint = CREATE<ASMTGearJoint>::With();
            mbdJoint->radiusI = getJointDistance(joint);
            mbdJoint->radiusJ = getJointDistance2(joint);
            return mbdJoint;
        }

        case JointType::Belt: {
            auto mbdJoint = CREATE<ASMTGearJoint>::With();
            mbdJoint->radiusI = getJointDistance(joint);
            mbdJoint->radiusJ = -getJointDistance2(joint);
            return mbdJoint;
        }

        default:
            return nullptr;
    }
}

std::shared_ptr<ASMTJoint> AssemblyObject::makeMbdJointDistance(App::DocumentObject* joint)
{
    DistanceType type = getDistanceType(joint);

    std::string elt1 = getElementFromProp(joint, "Reference1");
    std::string elt2 = getElementFromProp(joint, "Reference2");
    auto* obj1 = getLinkedObjFromRef(joint, "Reference1");
    auto* obj2 = getLinkedObjFromRef(joint, "Reference2");

    switch (type) {
        case DistanceType::PointPoint: {
            // Point to point distance, or ball joint if distance=0.
            double distance = getJointDistance(joint);
            if (distance < Precision::Confusion()) {
                return CREATE<ASMTSphericalJoint>::With();
            }
            auto mbdJoint = CREATE<ASMTSphSphJoint>::With();
            mbdJoint->distanceIJ = distance;
            return mbdJoint;
        }

        // Edge - edge cases
        case DistanceType::LineLine: {
            auto mbdJoint = CREATE<ASMTRevCylJoint>::With();
            mbdJoint->distanceIJ = getJointDistance(joint);
            return mbdJoint;
        }

        case DistanceType::LineCircle: {
            auto mbdJoint = CREATE<ASMTRevCylJoint>::With();
            mbdJoint->distanceIJ = getJointDistance(joint) + getEdgeRadius(obj2, elt2);
            return mbdJoint;
        }

        case DistanceType::CircleCircle: {
            auto mbdJoint = CREATE<ASMTRevCylJoint>::With();
            mbdJoint->distanceIJ = getJointDistance(joint) + getEdgeRadius(obj1, elt1)
                + getEdgeRadius(obj2, elt2);
            return mbdJoint;
        }

        // Face - Face cases
        case DistanceType::PlanePlane: {
            auto mbdJoint = CREATE<ASMTPlanarJoint>::With();
            mbdJoint->offset = getJointDistance(joint);
            return mbdJoint;
        }

        case DistanceType::PlaneCylinder: {
            auto mbdJoint = CREATE<ASMTLineInPlaneJoint>::With();
            mbdJoint->offset = getJointDistance(joint) + getFaceRadius(obj2, elt2);
            return mbdJoint;
        }

        case DistanceType::PlaneSphere: {
            auto mbdJoint = CREATE<ASMTPointInPlaneJoint>::With();
            mbdJoint->offset = getJointDistance(joint) + getFaceRadius(obj2, elt2);
            return mbdJoint;
        }

        case DistanceType::PlaneTorus: {
            auto mbdJoint = CREATE<ASMTPlanarJoint>::With();
            mbdJoint->offset = getJointDistance(joint);
            return mbdJoint;
        }

        case DistanceType::CylinderCylinder: {
            auto mbdJoint = CREATE<ASMTRevCylJoint>::With();
            mbdJoint->distanceIJ = getJointDistance(joint) + getFaceRadius(obj1, elt1)
                + getFaceRadius(obj2, elt2);
            return mbdJoint;
        }

        case DistanceType::CylinderSphere: {
            auto mbdJoint = CREATE<ASMTCylSphJoint>::With();
            mbdJoint->distanceIJ = getJointDistance(joint) + getFaceRadius(obj1, elt1)
                + getFaceRadius(obj2, elt2);
            return mbdJoint;
        }

        case DistanceType::CylinderTorus: {
            auto mbdJoint = CREATE<ASMTRevCylJoint>::With();
            mbdJoint->distanceIJ = getJointDistance(joint) + getFaceRadius(obj1, elt1)
                + getFaceRadius(obj2, elt2);
            return mbdJoint;
        }

        case DistanceType::TorusTorus: {
            auto mbdJoint = CREATE<ASMTPlanarJoint>::With();
            mbdJoint->offset = getJointDistance(joint);
            return mbdJoint;
        }

        case DistanceType::TorusSphere: {
            auto mbdJoint = CREATE<ASMTCylSphJoint>::With();
            mbdJoint->distanceIJ = getJointDistance(joint) + getFaceRadius(obj1, elt1)
                + getFaceRadius(obj2, elt2);
            return mbdJoint;
        }

        case DistanceType::SphereSphere: {
            auto mbdJoint = CREATE<ASMTSphSphJoint>::With();
            mbdJoint->distanceIJ = getJointDistance(joint) + getFaceRadius(obj1, elt1)
                + getFaceRadius(obj2, elt2);
            return mbdJoint;
        }

        // Point - Face cases
        case DistanceType::PointPlane: {
            auto mbdJoint = CREATE<ASMTPointInPlaneJoint>::With();
            mbdJoint->offset = getJointDistance(joint);
            return mbdJoint;
        }

        case DistanceType::PointCylinder: {
            auto mbdJoint = CREATE<ASMTCylSphJoint>::With();
            mbdJoint->distanceIJ = getJointDistance(joint) + getFaceRadius(obj1, elt1);
            return mbdJoint;
        }

        case DistanceType::PointSphere: {
            auto mbdJoint = CREATE<ASMTSphSphJoint>::With();
            mbdJoint->distanceIJ = getJointDistance(joint) + getFaceRadius(obj1, elt1);
            return mbdJoint;
        }

        // Edge - Face cases
        case DistanceType::LinePlane: {
            auto mbdJoint = CREATE<ASMTLineInPlaneJoint>::With();
            mbdJoint->offset = getJointDistance(joint);
            return mbdJoint;
        }

        // Point - Edge cases
        case DistanceType::PointLine: {
            auto mbdJoint = CREATE<ASMTCylSphJoint>::With();
            mbdJoint->distanceIJ = getJointDistance(joint);
            return mbdJoint;
        }

        case DistanceType::PointCurve: {
            // For other curves we do a point in plane-of-the-curve.
            // Maybe it would be best tangent / distance to the conic?
            // For arcs and circles we could use ASMTRevSphJoint. But is it better than
            // pointInPlane?
            auto mbdJoint = CREATE<ASMTPointInPlaneJoint>::With();
            mbdJoint->offset = getJointDistance(joint);
            return mbdJoint;
        }

        default: {
            // by default we make a planar joint.
            auto mbdJoint = CREATE<ASMTPlanarJoint>::With();
            mbdJoint->offset = getJointDistance(joint);
            return mbdJoint;
        }
    }
}

std::vector<std::shared_ptr<MbD::ASMTJoint>> AssemblyObject::makeMbdJoint(App::DocumentObject* joint)
{
    if (!joint) {
        return {};
    }

    JointType jointType = getJointType(joint);

    std::shared_ptr<ASMTJoint> mbdJoint = makeMbdJointOfType(joint, jointType);
    if (!mbdJoint || !isMbDJointValid(joint)) {
        return {};
    }

    std::string fullMarkerNameI, fullMarkerNameJ;
    if (jointType == JointType::RackPinion) {
        getRackPinionMarkers(joint, fullMarkerNameI, fullMarkerNameJ);
    }
    else {
        fullMarkerNameI = handleOneSideOfJoint(joint, "Reference1", "Placement1");
        fullMarkerNameJ = handleOneSideOfJoint(joint, "Reference2", "Placement2");
    }
    if (fullMarkerNameI == "" || fullMarkerNameJ == "") {
        return {};
    }

    mbdJoint->setName(joint->getFullName());
    mbdJoint->setMarkerI(fullMarkerNameI);
    mbdJoint->setMarkerJ(fullMarkerNameJ);

    // Add limits if needed. We do not add if this is a simulation or their might clash.
    if (motions.empty()) {
        if (jointType == JointType::Slider || jointType == JointType::Cylindrical) {
            auto* pLenMin = dynamic_cast<App::PropertyFloat*>(joint->getPropertyByName("LengthMin"));
            auto* pLenMax = dynamic_cast<App::PropertyFloat*>(joint->getPropertyByName("LengthMax"));
            auto* pMinEnabled = dynamic_cast<App::PropertyBool*>(
                joint->getPropertyByName("EnableLengthMin")
            );
            auto* pMaxEnabled = dynamic_cast<App::PropertyBool*>(
                joint->getPropertyByName("EnableLengthMax")
            );

            if (pLenMin && pLenMax && pMinEnabled && pMaxEnabled) {  // Make sure properties do exist
                // Swap the values if necessary.
                bool minEnabled = pMinEnabled->getValue();
                bool maxEnabled = pMaxEnabled->getValue();
                double minLength = pLenMin->getValue();
                double maxLength = pLenMax->getValue();

                if ((minLength > maxLength) && minEnabled && maxEnabled) {
                    pLenMin->setValue(maxLength);
                    pLenMax->setValue(minLength);
                    minLength = maxLength;
                    maxLength = pLenMax->getValue();

                    pMinEnabled->setValue(maxEnabled);
                    pMaxEnabled->setValue(minEnabled);
                    minEnabled = maxEnabled;
                    maxEnabled = pMaxEnabled->getValue();
                }

                if (minEnabled) {
                    auto limit = ASMTTranslationLimit::With();
                    limit->setName(joint->getFullName() + "-LimitLenMin");
                    limit->setMarkerI(fullMarkerNameI);
                    limit->setMarkerJ(fullMarkerNameJ);
                    limit->settype("=>");
                    limit->setlimit(std::to_string(minLength));
                    limit->settol("1.0e-9");
                    mbdAssembly->addLimit(limit);
                }

                if (maxEnabled) {
                    auto limit2 = ASMTTranslationLimit::With();
                    limit2->setName(joint->getFullName() + "-LimitLenMax");
                    limit2->setMarkerI(fullMarkerNameI);
                    limit2->setMarkerJ(fullMarkerNameJ);
                    limit2->settype("=<");
                    limit2->setlimit(std::to_string(maxLength));
                    limit2->settol("1.0e-9");
                    mbdAssembly->addLimit(limit2);
                }
            }
        }
        if (jointType == JointType::Revolute || jointType == JointType::Cylindrical) {
            auto* pRotMin = dynamic_cast<App::PropertyFloat*>(joint->getPropertyByName("AngleMin"));
            auto* pRotMax = dynamic_cast<App::PropertyFloat*>(joint->getPropertyByName("AngleMax"));
            auto* pMinEnabled = dynamic_cast<App::PropertyBool*>(
                joint->getPropertyByName("EnableAngleMin")
            );
            auto* pMaxEnabled = dynamic_cast<App::PropertyBool*>(
                joint->getPropertyByName("EnableAngleMax")
            );

            if (pRotMin && pRotMax && pMinEnabled && pMaxEnabled) {  // Make sure properties do exist
                // Swap the values if necessary.
                bool minEnabled = pMinEnabled->getValue();
                bool maxEnabled = pMaxEnabled->getValue();
                double minAngle = pRotMin->getValue();
                double maxAngle = pRotMax->getValue();
                if ((minAngle > maxAngle) && minEnabled && maxEnabled) {
                    pRotMin->setValue(maxAngle);
                    pRotMax->setValue(minAngle);
                    minAngle = maxAngle;
                    maxAngle = pRotMax->getValue();

                    pMinEnabled->setValue(maxEnabled);
                    pMaxEnabled->setValue(minEnabled);
                    minEnabled = maxEnabled;
                    maxEnabled = pMaxEnabled->getValue();
                }

                if (minEnabled) {
                    auto limit = ASMTRotationLimit::With();
                    limit->setName(joint->getFullName() + "-LimitRotMin");
                    limit->setMarkerI(fullMarkerNameI);
                    limit->setMarkerJ(fullMarkerNameJ);
                    limit->settype("=>");
                    limit->setlimit(std::to_string(minAngle) + "*pi/180.0");
                    limit->settol("1.0e-9");
                    mbdAssembly->addLimit(limit);
                }

                if (maxEnabled) {
                    auto limit2 = ASMTRotationLimit::With();
                    limit2->setName(joint->getFullName() + "-LimitRotMax");
                    limit2->setMarkerI(fullMarkerNameI);
                    limit2->setMarkerJ(fullMarkerNameJ);
                    limit2->settype("=<");
                    limit2->setlimit(std::to_string(maxAngle) + "*pi/180.0");
                    limit2->settol("1.0e-9");
                    mbdAssembly->addLimit(limit2);
                }
            }
        }
    }
    std::vector<App::DocumentObject*> done;
    // Add motions if needed
    for (auto* motion : motions) {
        if (std::ranges::find(done, motion) != done.end()) {
            continue;  // don't process twice (can happen in case of cylindrical)
        }

        auto* pJoint = dynamic_cast<App::PropertyXLinkSub*>(motion->getPropertyByName("Joint"));
        if (!pJoint) {
            continue;
        }
        App::DocumentObject* motionJoint = pJoint->getValue();
        if (joint != motionJoint) {
            continue;
        }

        auto* pType = dynamic_cast<App::PropertyEnumeration*>(motion->getPropertyByName("MotionType"));
        auto* pFormula = dynamic_cast<App::PropertyString*>(motion->getPropertyByName("Formula"));
        if (!pType || !pFormula) {
            continue;
        }
        std::string formula = pFormula->getValue();
        if (formula == "") {
            continue;
        }
        std::string motionType = pType->getValueAsString();

        // check if there is a second motion as cylindrical can have both,
        // in which case the solver needs a general motion.
        for (auto* motion2 : motions) {
            pJoint = dynamic_cast<App::PropertyXLinkSub*>(motion2->getPropertyByName("Joint"));
            if (!pJoint) {
                continue;
            }
            motionJoint = pJoint->getValue();
            if (joint != motionJoint || motion2 == motion) {
                continue;
            }

            auto* pType2 = dynamic_cast<App::PropertyEnumeration*>(
                motion2->getPropertyByName("MotionType")
            );
            auto* pFormula2 = dynamic_cast<App::PropertyString*>(motion2->getPropertyByName("Formula"));
            if (!pType2 || !pFormula2) {
                continue;
            }
            std::string formula2 = pFormula2->getValue();
            if (formula2 == "") {
                continue;
            }
            std::string motionType2 = pType2->getValueAsString();
            if (motionType2 == motionType) {
                continue;  // only if both motions are different. ie one angular and one linear.
            }

            auto ASMTmotion = CREATE<ASMTGeneralMotion>::With();
            ASMTmotion->setName(joint->getFullName() + "-ScrewMotion");
            ASMTmotion->setMarkerI(fullMarkerNameI);
            ASMTmotion->setMarkerJ(fullMarkerNameJ);
            ASMTmotion->rIJI->atiput(2, motionType == "Angular" ? formula2 : formula);
            ASMTmotion->angIJJ->atiput(2, motionType == "Angular" ? formula : formula2);
            mbdAssembly->addMotion(ASMTmotion);

            done.push_back(motion2);
        }

        if (motionType == "Angular") {
            auto ASMTmotion = CREATE<ASMTRotationalMotion>::With();
            ASMTmotion->setName(joint->getFullName() + "-AngularMotion");
            ASMTmotion->setMarkerI(fullMarkerNameI);
            ASMTmotion->setMarkerJ(fullMarkerNameJ);
            ASMTmotion->setRotationZ(formula);
            mbdAssembly->addMotion(ASMTmotion);
        }
        else if (motionType == "Linear") {
            auto ASMTmotion = CREATE<ASMTTranslationalMotion>::With();
            ASMTmotion->setName(joint->getFullName() + "-LinearMotion");
            ASMTmotion->setMarkerI(fullMarkerNameI);
            ASMTmotion->setMarkerJ(fullMarkerNameJ);
            ASMTmotion->setTranslationZ(formula);
            mbdAssembly->addMotion(ASMTmotion);
        }
    }

    return {mbdJoint};
}

std::string AssemblyObject::handleOneSideOfJoint(
    App::DocumentObject* joint,
    const char* propRefName,
    const char* propPlcName
)
{
    App::DocumentObject* part = getMovingPartFromRef(joint, propRefName);
    App::DocumentObject* obj = getObjFromJointRef(joint, propRefName);

    if (!part || !obj) {
        Base::Console()
            .warning("The property %s of Joint %s is bad.\n", propRefName, joint->getFullName());
        return "";
    }

    MbDPartData data = getMbDData(part);
    std::shared_ptr<ASMTPart> mbdPart = data.part;
    Base::Placement plc = getPlacementFromProp(joint, propPlcName);
    // Now we have plc which is the JCS placement, but its relative to the Object, not to the
    // containing Part.

    if (obj->getNameInDocument() != part->getNameInDocument()) {

        auto* ref = dynamic_cast<App::PropertyXLinkSub*>(joint->getPropertyByName(propRefName));
        if (!ref) {
            return "";
        }

        Base::Placement obj_global_plc = getGlobalPlacement(obj, ref);
        plc = obj_global_plc * plc;

        Base::Placement part_global_plc = getGlobalPlacement(part, ref);
        plc = part_global_plc.inverse() * plc;
    }
    // check if we need to add an offset in case of bundled parts.
    if (!data.offsetPlc.isIdentity()) {
        plc = data.offsetPlc * plc;
    }

    std::string markerName = joint->getFullName();
    auto mbdMarker = makeMbdMarker(markerName, plc);
    mbdPart->addMarker(mbdMarker);

    return "/OndselAssembly/" + mbdPart->name + "/" + markerName;
}

void AssemblyObject::getRackPinionMarkers(
    App::DocumentObject* joint,
    std::string& markerNameI,
    std::string& markerNameJ
)
{
    // ASMT rack pinion joint must get the rack as I and pinion as J.
    // - rack marker has to have Z axis parallel to pinion Z axis.
    // - rack marker has to have X axis parallel to the sliding axis.
    // The user will have selected the sliding marker so we need to transform it.
    // And we need to detect which marker is the rack.

    int slidingIndex = slidingPartIndex(joint);
    if (slidingIndex == 0) {
        return;
    }

    if (slidingIndex != 1) {
        swapJCS(joint);  // make sure that rack is first.
    }

    App::DocumentObject* part1 = getMovingPartFromRef(joint, "Reference1");
    App::DocumentObject* obj1 = getObjFromJointRef(joint, "Reference1");
    Base::Placement plc1 = getPlacementFromProp(joint, "Placement1");

    App::DocumentObject* obj2 = getObjFromJointRef(joint, "Reference2");
    Base::Placement plc2 = getPlacementFromProp(joint, "Placement2");

    if (!part1 || !obj1) {
        Base::Console().warning("Reference1 of Joint %s is bad.\n", joint->getFullName());
        return;
    }

    // For the pinion nothing special needed :
    markerNameJ = handleOneSideOfJoint(joint, "Reference2", "Placement2");

    // For the rack we need to change the placement :
    // make the pinion plc relative to the rack placement.
    auto* ref1 = dynamic_cast<App::PropertyXLinkSub*>(joint->getPropertyByName("Reference1"));
    auto* ref2 = dynamic_cast<App::PropertyXLinkSub*>(joint->getPropertyByName("Reference2"));
    if (!ref1 || !ref2) {
        return;
    }
    Base::Placement pinion_global_plc = getGlobalPlacement(obj2, ref2);
    plc2 = pinion_global_plc * plc2;
    Base::Placement rack_global_plc = getGlobalPlacement(obj1, ref1);
    plc2 = rack_global_plc.inverse() * plc2;

    // The rot of the rack placement should be the same as the pinion, but with X axis along the
    // slider axis.
    Base::Rotation rot = plc2.getRotation();
    // the yaw of rot has to be the same as plc1
    Base::Vector3d currentZAxis = rot.multVec(Base::Vector3d(0, 0, 1));
    Base::Vector3d currentXAxis = rot.multVec(Base::Vector3d(1, 0, 0));
    Base::Vector3d targetXAxis = plc1.getRotation().multVec(Base::Vector3d(0, 0, 1));

    // Calculate the angle between the current X axis and the target X axis
    double yawAdjustment = currentXAxis.GetAngle(targetXAxis);

    // Determine the direction of the yaw adjustment using cross product
    Base::Vector3d crossProd = currentXAxis.Cross(targetXAxis);
    if (currentZAxis * crossProd < 0) {  // If cross product is in opposite direction to Z axis
        yawAdjustment = -yawAdjustment;
    }

    // Create a yaw rotation around the Z axis
    Base::Rotation yawRotation(currentZAxis, yawAdjustment);

    // Combine the initial rotation with the yaw adjustment
    Base::Rotation adjustedRotation = rot * yawRotation;
    plc1.setRotation(adjustedRotation);

    // Then end of processing similar to handleOneSideOfJoint :
    MbDPartData data1 = getMbDData(part1);
    std::shared_ptr<ASMTPart> mbdPart = data1.part;
    if (obj1->getNameInDocument() != part1->getNameInDocument()) {
        plc1 = rack_global_plc * plc1;

        Base::Placement part_global_plc = getGlobalPlacement(part1, ref1);
        plc1 = part_global_plc.inverse() * plc1;
    }
    // check if we need to add an offset in case of bundled parts.
    if (!data1.offsetPlc.isIdentity()) {
        plc1 = data1.offsetPlc * plc1;
    }

    std::string markerName = joint->getFullName();
    auto mbdMarker = makeMbdMarker(markerName, plc1);
    mbdPart->addMarker(mbdMarker);

    markerNameI = "/OndselAssembly/" + mbdPart->name + "/" + markerName;
}

int AssemblyObject::slidingPartIndex(App::DocumentObject* joint)
{
    App::DocumentObject* part1 = getMovingPartFromRef(joint, "Reference1");
    App::DocumentObject* obj1 = getObjFromJointRef(joint, "Reference1");
    boost::ignore_unused(obj1);
    Base::Placement plc1 = getPlacementFromProp(joint, "Placement1");

    App::DocumentObject* part2 = getMovingPartFromRef(joint, "Reference2");
    App::DocumentObject* obj2 = getObjFromJointRef(joint, "Reference2");
    boost::ignore_unused(obj2);
    Base::Placement plc2 = getPlacementFromProp(joint, "Placement2");

    int slidingFound = 0;
    for (auto* jt : getJoints(false, false)) {
        if (getJointType(jt) == JointType::Slider) {
            App::DocumentObject* jpart1 = getMovingPartFromRef(jt, "Reference1");
            App::DocumentObject* jpart2 = getMovingPartFromRef(jt, "Reference2");
            int found = 0;
            Base::Placement plcjt, plci;
            if (jpart1 == part1 || jpart1 == part2) {
                found = (jpart1 == part1) ? 1 : 2;
                plci = (jpart1 == part1) ? plc1 : plc2;
                plcjt = getPlacementFromProp(jt, "Placement1");
            }
            else if (jpart2 == part1 || jpart2 == part2) {
                found = (jpart2 == part1) ? 1 : 2;
                plci = (jpart2 == part1) ? plc1 : plc2;
                plcjt = getPlacementFromProp(jt, "Placement2");
            }

            if (found != 0) {
                // check the placements plcjt and (jcs1 or jcs2 depending on found value) Z axis are
                // colinear ie if their pitch and roll are the same.
                double y1, p1, r1, y2, p2, r2;
                plcjt.getRotation().getYawPitchRoll(y1, p1, r1);
                plci.getRotation().getYawPitchRoll(y2, p2, r2);
                if (fabs(p1 - p2) < Precision::Confusion() && fabs(r1 - r2) < Precision::Confusion()) {
                    slidingFound = found;
                }
            }
        }
    }
    return slidingFound;
}

bool AssemblyObject::isMbDJointValid(App::DocumentObject* joint)
{
    // When dragging a part, we are bundling fixed parts together.
    // This may lead to a conflicting joint that is self referencing a MbD part.
    // The solver crash when fed such a bad joint. So we make sure it does not happen.
    App::DocumentObject* part1 = getMovingPartFromRef(joint, "Reference1");
    App::DocumentObject* part2 = getMovingPartFromRef(joint, "Reference2");
    if (!part1 || !part2) {
        return false;
    }

    // If this joint is self-referential it must be ignored.
    if (getMbDPart(part1) == getMbDPart(part2)) {
        Base::Console().warning(
            "Assembly: Ignoring joint (%s) because its parts are connected by a fixed "
            "joint bundle. This joint is a conflicting or redundant constraint.\n",
            joint->getFullLabel()
        );
        return false;
    }
    return true;
}

AssemblyObject::MbDPartData AssemblyObject::getMbDData(App::DocumentObject* part)
{
    auto it = objectPartMap.find(part);
    if (it != objectPartMap.end()) {
        // part has been associated with an ASMTPart before
        return it->second;
    }

    // part has not been associated with an ASMTPart before
    std::string str = part->getFullName();
    Base::Placement plc = getPlacementFromProp(part, "Placement");
    std::shared_ptr<ASMTPart> mbdPart = makeMbdPart(str, plc);
    mbdAssembly->addPart(mbdPart);
    MbDPartData data = {mbdPart, Base::Placement()};
    objectPartMap[part] = data;  // Store the association

    // Associate other objects connected with fixed joints
    if (bundleFixed) {
        auto addConnectedFixedParts = [&](App::DocumentObject* currentPart, auto& self) -> void {
            std::vector<App::DocumentObject*> joints = getJointsOfPart(currentPart);
            for (auto* joint : joints) {
                JointType jointType = getJointType(joint);
                if (jointType == JointType::Fixed) {
                    App::DocumentObject* part1 = getMovingPartFromRef(joint, "Reference1");
                    App::DocumentObject* part2 = getMovingPartFromRef(joint, "Reference2");
                    App::DocumentObject* partToAdd = currentPart == part1 ? part2 : part1;

                    if (objectPartMap.find(partToAdd) != objectPartMap.end()) {
                        // already added
                        continue;
                    }

                    Base::Placement plci = getPlacementFromProp(partToAdd, "Placement");
                    MbDPartData partData = {mbdPart, plc.inverse() * plci};
                    objectPartMap[partToAdd] = partData;  // Store the association

                    // Recursively call for partToAdd
                    self(partToAdd, self);
                }
            }
        };

        addConnectedFixedParts(part, addConnectedFixedParts);
    }
    return data;
}

std::shared_ptr<ASMTPart> AssemblyObject::getMbDPart(App::DocumentObject* part)
{
    if (!part) {
        return nullptr;
    }
    return getMbDData(part).part;
}

std::shared_ptr<ASMTPart> AssemblyObject::makeMbdPart(std::string& name, Base::Placement plc, double mass)
{
    auto mbdPart = CREATE<ASMTPart>::With();
    mbdPart->setName(name);

    auto massMarker = CREATE<ASMTPrincipalMassMarker>::With();
    massMarker->setMass(mass);
    massMarker->setDensity(1.0);
    massMarker->setMomentOfInertias(1.0, 1.0, 1.0);
    mbdPart->setPrincipalMassMarker(massMarker);

    Base::Vector3d pos = plc.getPosition();
    mbdPart->setPosition3D(pos.x, pos.y, pos.z);

    // TODO : replace with quaternion to simplify
    Base::Rotation rot = plc.getRotation();
    Base::Matrix4D mat;
    rot.getValue(mat);
    Base::Vector3d r0 = mat.getRow(0);
    Base::Vector3d r1 = mat.getRow(1);
    Base::Vector3d r2 = mat.getRow(2);
    mbdPart->setRotationMatrix(r0.x, r0.y, r0.z, r1.x, r1.y, r1.z, r2.x, r2.y, r2.z);

    return mbdPart;
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

    return mbdMarker;
}

std::vector<ObjRef> AssemblyObject::getDownstreamParts(
    App::DocumentObject* part,
    App::DocumentObject* joint
)
{
    if (!part) {
        return {};
    }

    // First we deactivate the joint
    bool state = false;
    if (joint) {
        state = getJointActivated(joint);
        setJointActivated(joint, false);
    }

    std::vector<App::DocumentObject*> joints = getJoints(false);

    std::vector<ObjRef> connectedParts = {{part, nullptr}};
    traverseAndMarkConnectedParts(part, connectedParts, joints);

    std::vector<ObjRef> downstreamParts;
    for (auto& parti : connectedParts) {
        if (!isPartConnected(parti.obj) && (parti.obj != part)) {
            downstreamParts.push_back(parti);
        }
    }

    if (joint) {
        setJointActivated(joint, state);
    }

    return downstreamParts;
}

App::DocumentObject* AssemblyObject::getUpstreamMovingPart(
    App::DocumentObject* part,
    App::DocumentObject*& joint,
    std::string& name,
    std::vector<App::DocumentObject*> excludeJoints
)
{
    if (!part || isPartGrounded(part)) {
        return nullptr;
    }

    excludeJoints.push_back(joint);

    joint = getJointOfPartConnectingToGround(part, name, excludeJoints);
    JointType jointType = getJointType(joint);
    if (jointType != JointType::Fixed) {
        return part;
    }

    part = getMovingPartFromRef(joint, name == "Reference1" ? "Reference2" : "Reference1");

    return getUpstreamMovingPart(part, joint, name);
}

double AssemblyObject::getObjMass(App::DocumentObject* obj)
{
    if (!obj) {
        return 0.0;
    }

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

std::vector<AssemblyLink*> AssemblyObject::getSubAssemblies()
{
    std::vector<AssemblyLink*> subAssemblies = {};

    App::Document* doc = getDocument();

    std::vector<DocumentObject*> assemblies = doc->getObjectsOfType(
        Assembly::AssemblyLink::getClassTypeId()
    );
    for (auto assembly : assemblies) {
        if (hasObject(assembly)) {
            subAssemblies.push_back(freecad_cast<AssemblyLink*>(assembly));
        }
    }

    return subAssemblies;
}

void AssemblyObject::ensureIdentityPlacements()
{
    std::vector<App::DocumentObject*> group = Group.getValues();
    for (auto* obj : group) {
        // When used in assembly, link groups must have identity placements.
        if (obj->isLinkGroup()) {
            auto* link = dynamic_cast<App::Link*>(obj);
            auto* pPlc = obj->getPlacementProperty();
            if (!pPlc || !link) {
                continue;
            }

            Base::Placement plc = pPlc->getValue();
            if (plc.isIdentity()) {
                continue;
            }

            pPlc->setValue(Base::Placement());
            obj->purgeTouched();

            // To keep the LinkElement positions, we apply plc to their placements
            std::vector<App::DocumentObject*> elts = link->ElementList.getValues();
            for (auto* elt : elts) {
                pPlc = elt->getPlacementProperty();
                pPlc->setValue(plc * pPlc->getValue());
                elt->purgeTouched();
            }
        }
    }
}

int AssemblyObject::numberOfComponents() const
{
    return getAssemblyComponents(this).size();
}

bool AssemblyObject::isEmpty() const
{
    return numberOfComponents() == 0;
}
