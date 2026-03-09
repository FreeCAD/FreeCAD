// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Jacob Oursland <jacob.oursland[at]gmail.com>        *
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

#include "ChronoSolver.h"

// The rest of this file is only meaningful when Chrono is available.
#ifdef HAVE_CHRONO

# include <Base/Console.h>
# include <Base/Placement.h>
# include <App/Application.h>

// Chrono headers
# include <chrono/physics/ChBody.h>
# include <chrono/physics/ChLinkDistance.h>
# include <chrono/physics/ChLinkLockGear.h>
# include <chrono/physics/ChLinkLockPulley.h>
# include <chrono/physics/ChLinkLockScrew.h>
# include <chrono/physics/ChLinkLock.h>
# include <chrono/physics/ChLinkMate.h>   // ChLinkMateRackPinion has no Lock equivalent
# include <chrono/physics/ChSystemNSC.h>
# include <chrono/solver/ChDirectSolverLS.h>
# include <chrono/timestepper/ChAssemblyAnalysis.h>

FC_LOG_LEVEL_INIT("ChronoSolver", true, true, true)

namespace
{

/// Evaluate the simple expression strings produced by AssemblyObject for limits.
/// Translation limits are stored as plain number strings (e.g. "0.500000").
/// Rotation limits are stored as degree-to-radian expressions (e.g. "30.000000*pi/180.0").
static double evaluateLimitExpression(const std::string& expr)
{
    static const std::string kSuffix = "*pi/180.0";
    if (expr.size() > kSuffix.size()
        && expr.compare(expr.size() - kSuffix.size(), kSuffix.size(), kSuffix) == 0) {
        return std::stod(expr.substr(0, expr.size() - kSuffix.size())) * M_PI / 180.0;
    }
    return std::stod(expr);
}

}  // namespace

using namespace Assembly;
using namespace Assembly::Solver;

// ============================================================================
// Conversion helpers between FreeCAD Base::Placement and Chrono ChFrame
// ============================================================================

// FreeCAD stores quaternion as (x, y, z, w) — i.e., getValue(q0,q1,q2,q3) gives (x,y,z,w).
// Chrono ChQuaternion stores as (e0=w, e1=x, e2=y, e3=z).

static chrono::ChFrame<double> placementToChFrame(const Base::Placement& plc)
{
    const Base::Vector3d& p = plc.getPosition();
    double qx, qy, qz, qw;
    plc.getRotation().getValue(qx, qy, qz, qw);
    return chrono::ChFrame<double>(
        chrono::ChVector3d(p.x, p.y, p.z),
        chrono::ChQuaternion<double>(qw, qx, qy, qz)
    );
}

static Base::Placement chFrameToPlacement(const chrono::ChFrame<double>& frame)
{
    const auto& pos = frame.GetPos();
    const auto& rot = frame.GetRot();
    // rot: e0=w, e1=x, e2=y, e3=z → FreeCAD Rotation(x,y,z,w)
    return Base::Placement(
        Base::Vector3d(pos.x(), pos.y(), pos.z()),
        Base::Rotation(rot.e1(), rot.e2(), rot.e3(), rot.e0())
    );
}

// ============================================================================
// MarkerRef parsing
// The separator '|' is used between part name and marker name.
// Ground marker refs use the prefix "__ground__|".
// ============================================================================

static constexpr char kSep = '|';
static const std::string kGroundPrefix = "__ground__";

static MarkerRef makePartMarkerRef(const std::string& partName, const std::string& markerName)
{
    return partName + kSep + markerName;
}

static MarkerRef makeGroundMarkerRef(const std::string& markerName)
{
    return kGroundPrefix + kSep + markerName;
}

// Split a MarkerRef into (prefix, markerName).
// Returns false if the separator is not found.
static bool splitMarkerRef(const MarkerRef& ref, std::string& prefix, std::string& markerName)
{
    const auto sep = ref.find(kSep);
    if (sep == std::string::npos) {
        return false;
    }
    prefix = ref.substr(0, sep);
    markerName = ref.substr(sep + 1);
    return true;
}

// ============================================================================
// ChronoPart
// ============================================================================

ChronoPart::ChronoPart()
{
    body = std::make_shared<chrono::ChBody>();
}

void ChronoPart::setPlacement(Base::Placement plc)
{
    Part::setPlacement(plc);
    auto frame = placementToChFrame(plc);
    body->SetPos(frame.GetPos());
    body->SetRot(frame.GetRot());
}

void ChronoPart::pushPlacement(Base::Placement plc)
{
    auto frame = placementToChFrame(plc);
    body->SetPos(frame.GetPos());
    body->SetRot(frame.GetRot());
}

MarkerRef ChronoPart::addMarker(std::string name, Base::Placement plc)
{
    localMarkers[name] = placementToChFrame(plc);
    return makePartMarkerRef(getName(), name);
}

Base::Placement ChronoPart::getPlacement() const
{
    chrono::ChFrame<double> frame(body->GetPos(), body->GetRot());
    return chFrameToPlacement(frame);
}

std::shared_ptr<chrono::ChBody> ChronoPart::getBody() const
{
    return body;
}

bool ChronoPart::getMarkerFrame(const std::string& markerName, chrono::ChFrame<double>& outFrame) const
{
    auto it = localMarkers.find(markerName);
    if (it == localMarkers.end()) {
        return false;
    }
    outFrame = it->second;
    return true;
}

void ChronoPart::forEachMarker(
    std::function<void(const std::string&, const chrono::ChFrame<double>&)> cb
) const
{
    for (const auto& [name, frame] : localMarkers) {
        cb(name, frame);
    }
}

// ============================================================================
// ChronoAssembly
// ============================================================================

ChronoAssembly::ChronoAssembly()
{
    sys = std::make_unique<chrono::ChSystemNSC>();
    sys->SetGravitationalAcceleration(chrono::ChVector3d(0, 0, 0));

    // Switch from SparseLU to SparseQR.  QR computes a minimum-norm least-squares
    // solution and handles rank-deficient constraint Jacobians without requiring
    // compliance regularisation.  This is more robust for closed kinematic loops
    // where some constraints are structurally redundant.
    sys->SetSolver(chrono_types::make_shared<chrono::ChSolverSparseQR>());

    // Create a fixed ground body at the world origin
    groundBody = std::make_shared<chrono::ChBody>();
    groundBody->SetFixed(true);
    groundBody->SetName("__ground__");
    sys->AddBody(groundBody);
}

void ChronoAssembly::addPart(std::shared_ptr<Part> part)
{
    auto chronoPart = std::static_pointer_cast<ChronoPart>(part);

    // Configure the body
    auto body = chronoPart->getBody();
    body->SetName(part->getName());
    body->SetMass(part->getMass());
    double ixx, iyy, izz;
    part->getMomentOfInertias(ixx, iyy, izz);
    body->SetInertiaXX(chrono::ChVector3d(ixx, iyy, izz));

    // Initial placement was already set via setPlacement(); sync to body coords.
    auto frame = placementToChFrame(part->getPlacement());
    body->SetPos(frame.GetPos());
    body->SetRot(frame.GetRot());

    sys->AddBody(body);
    parts.push_back(chronoPart);
}

MarkerRef ChronoAssembly::addGroundMarker(std::string name, Base::Placement plc)
{
    groundMarkers[name] = placementToChFrame(plc);
    return makeGroundMarkerRef(name);
}

bool ChronoAssembly::resolveMarker(
    const MarkerRef& ref,
    std::shared_ptr<chrono::ChBody>& outBody,
    chrono::ChFrame<double>& outFrame
) const
{
    // Check cache first
    {
        auto it = markerRegistry.find(ref);
        if (it != markerRegistry.end()) {
            outBody = it->second.body;
            outFrame = it->second.localFrame;
            return true;
        }
    }

    std::string prefix, markerName;
    if (!splitMarkerRef(ref, prefix, markerName)) {
        FC_ERR("ChronoSolver: malformed MarkerRef: " << ref);
        return false;
    }

    if (prefix == kGroundPrefix) {
        auto it = groundMarkers.find(markerName);
        if (it == groundMarkers.end()) {
            FC_ERR("ChronoSolver: ground marker not found: " << markerName);
            return false;
        }
        outBody = groundBody;
        outFrame = it->second;
        // Cache
        const_cast<ChronoAssembly*>(this)->markerRegistry[ref] = {outBody, outFrame};
        return true;
    }

    // Search parts by name
    for (const auto& part : parts) {
        if (part->getName() == prefix) {
            chrono::ChFrame<double> frame;
            if (!part->getMarkerFrame(markerName, frame)) {
                FC_ERR(
                    "ChronoSolver: marker '" << markerName << "' not found on part '" << prefix << "'"
                );
                return false;
            }
            outBody = part->getBody();
            outFrame = frame;
            // Cache
            const_cast<ChronoAssembly*>(this)->markerRegistry[ref] = {outBody, outFrame};
            return true;
        }
    }

    FC_ERR("ChronoSolver: part not found for MarkerRef: " << ref);
    return false;
}

void ChronoAssembly::addJoint(std::shared_ptr<Joint> joint)
{
    std::shared_ptr<chrono::ChBody> body1, body2;
    chrono::ChFrame<double> frame1, frame2;

    if (!resolveMarker(joint->getMarkerI(), body1, frame1)) {
        FC_WARN("ChronoSolver: skipping joint '" << joint->getName() << "': cannot resolve markerI");
        return;
    }
    if (!resolveMarker(joint->getMarkerJ(), body2, frame2)) {
        FC_WARN("ChronoSolver: skipping joint '" << joint->getName() << "': cannot resolve markerJ");
        return;
    }

    // Normalize frame Z-axes: axis-based constraints (revolute, cylindrical,
    // prismatic, etc.) require the two Z-axes to be parallel, not anti-parallel.
    // FreeCAD's JCS frames for a pin-hole pair commonly have opposing Z-axes
    // (one face-normal points "in", the other "out").  If we detect anti-parallel
    // world Z-axes, flip frame1 by Rx(180°) — quaternion (e0=0,e1=1,e2=0,e3=0)
    // — which negates its Z and Y axes while keeping X, making the axes parallel.
    {
        chrono::ChVector3d z1 = body1->GetRot().Rotate(frame1.GetRot().GetAxisZ());
        chrono::ChVector3d z2 = body2->GetRot().Rotate(frame2.GetRot().GetAxisZ());
        const double dot = z1.Dot(z2);
        if (debugLogging) {
            auto p1 = body1->GetPos() + body1->GetRot().Rotate(frame1.GetPos());
            auto p2 = body2->GetPos() + body2->GetRot().Rotate(frame2.GetPos());
            FC_MSG(
                "  joint '" << joint->getName() << "'"
                << "  body1='" << body1->GetName() << "'"
                << "  body2='" << body2->GetName() << "'"
                << "\n    world_pos1=(" << p1.x() << "," << p1.y() << "," << p1.z() << ")"
                << "  world_pos2=(" << p2.x() << "," << p2.y() << "," << p2.z() << ")"
                << "\n    world_z1=(" << z1.x() << "," << z1.y() << "," << z1.z() << ")"
                << "  world_z2=(" << z2.x() << "," << z2.y() << "," << z2.z() << ")"
                << "  z_dot=" << dot
            );
        }
        if (dot < 0.0) {
            frame1.SetRot(frame1.GetRot() * chrono::ChQuaternion<double>(0.0, 1.0, 0.0, 0.0));
            if (debugLogging) {
                auto z1n = body1->GetRot().Rotate(frame1.GetRot().GetAxisZ());
                FC_MSG(
                    "  -> flipped frame1 Z (Rx180); new world_z1=("
                    << z1n.x() << "," << z1n.y() << "," << z1n.z() << ")"
                );
            }
        }
    }

    // Helper: Initialize a ChLinkMate-based joint with body-relative frames.
    // Used only for joints with no ChLinkLock equivalent (e.g. RackPinion).
    auto initMate = [&](auto link) {
        link->SetName(joint->getName());
        link->Initialize(body1, body2, true, frame1, frame2);
        sys->AddLink(link);
    };

    // Helper: Initialize a ChLinkLock-based joint with body-relative frames.
    auto initLock = [&](auto link) {
        link->SetName(joint->getName());
        link->Initialize(body1, body2, true, frame1, frame2);
        sys->AddLink(link);
    };

    // Helper: Initialize a ChLinkLock-based joint and register it for limit support.
    // Joints with free DOFs that can have limits (Revolute, Prismatic, Cylindrical)
    // are stored in limitableJoints keyed by (markerI, markerJ) so addLimit() can
    // find the right link and call LimitRz()/LimitZ() on it.
    auto initLockLimitable = [&](auto link) {
        link->SetName(joint->getName());
        link->Initialize(body1, body2, true, frame1, frame2);
        sys->AddLink(link);
        limitableJoints[{joint->getMarkerI(), joint->getMarkerJ()}] = link;
    };

    switch (joint->getJointClass()) {
        case JointClass::FIXED_JOINT:
            // If one body is the ground, mark the other as truly fixed in Chrono
            // instead of adding a constraint.  SetFixed(true) removes the body from
            // the solver's degrees of freedom entirely, which is more robust than a
            // constraint link whose Newton-Raphson residual can cause tiny drift.
            if (body1 == groundBody) {
                body2->SetFixed(true);
            }
            else if (body2 == groundBody) {
                body1->SetFixed(true);
            }
            else {
                initLock(std::make_shared<chrono::ChLinkLockLock>());
            }
            break;

        case JointClass::REVOLUTE_JOINT:
            // Free DOF: Rz.  Limits applied via LimitRz().
            initLockLimitable(std::make_shared<chrono::ChLinkLockRevolute>());
            break;

        case JointClass::CYLINDRICAL_JOINT:
            // Free DOFs: Z (translation) and Rz (rotation).
            // Limits applied via LimitZ() and LimitRz().
            // ChSolverSparseQR handles any redundant constraints in closed kinematic
            // loops natively (minimum-norm least-squares), so no compliance is needed.
            initLockLimitable(std::make_shared<chrono::ChLinkLockCylindrical>());
            break;

        case JointClass::TRANSLATIONAL_JOINT:
            // Free DOF: Z (translation along the joint axis).  Limits via LimitZ().
            initLockLimitable(std::make_shared<chrono::ChLinkLockPrismatic>());
            break;

        case JointClass::SPHERICAL_JOINT:
            // Free DOFs: Rx, Ry, Rz.
            initLock(std::make_shared<chrono::ChLinkLockSpherical>());
            break;

        case JointClass::PARALLEL_AXES_JOINT:
            initLock(std::make_shared<chrono::ChLinkLockParallel>());
            break;

        case JointClass::PERPENDICULAR_JOINT:
            initLock(std::make_shared<chrono::ChLinkLockPerpend>());
            break;

        case JointClass::PLANAR_JOINT:
            // Free DOFs: X, Y (in-plane translation) and Rz (in-plane rotation).
            // Constrains Z (normal) and Rx, Ry (tilt).
            initLock(std::make_shared<chrono::ChLinkLockPlanar>());
            break;

        case JointClass::POINT_IN_PLANE_JOINT:
            // Constrains Z (normal to plane); leaves X, Y, Rx, Ry, Rz free.
            initLock(std::make_shared<chrono::ChLinkLockPointPlane>());
            break;

        case JointClass::POINT_IN_LINE_JOINT:
            // Constrains Y and Z; leaves X (along the line), Rx, Ry, Rz free.
            initLock(std::make_shared<chrono::ChLinkLockPointLine>());
            break;

        case JointClass::LINE_IN_PLANE_JOINT:
            // Constrains Z (normal), Rx and Ry (line parallel to plane);
            // leaves X, Y, Rz free.  Same DOF pattern as ChLinkLockPlanar.
            initLock(std::make_shared<chrono::ChLinkLockPlanar>());
            break;

        case JointClass::RACK_PINION_JOINT: {
            auto solverJoint = std::static_pointer_cast<RackPinionJoint>(joint);
            auto link = std::make_shared<chrono::ChLinkMateRackPinion>();
            link->SetPinionRadius(solverJoint->getPitchRadius());
            initMate(link);
            break;
        }

        case JointClass::SCREW_JOINT: {
            auto solverJoint = std::static_pointer_cast<ScrewJoint>(joint);
            auto link = std::make_shared<chrono::ChLinkLockScrew>();
            // Chrono thread = pitch * 2*pi → SetThread(pitch)
            link->SetThread(solverJoint->getPitch());
            initLock(link);
            break;
        }

        case JointClass::GEAR_JOINT: {
            auto solverJoint = std::static_pointer_cast<GearJoint>(joint);
            auto link = std::make_shared<chrono::ChLinkLockGear>();
            link->SetTransmissionRatio(solverJoint->getRadiusI(), solverJoint->getRadiusJ());
            initLock(link);
            break;
        }

        case JointClass::BELT_JOINT: {
            auto solverJoint = std::static_pointer_cast<BeltJoint>(joint);
            auto link = std::make_shared<chrono::ChLinkLockPulley>();
            link->SetRadius1(solverJoint->getRadiusI());
            link->SetRadius2(solverJoint->getRadiusJ());
            initLock(link);
            break;
        }

        case JointClass::SPH_SPH_JOINT: {
            // Enforce a fixed distance between the two marker origins.
            auto solverJoint = std::static_pointer_cast<SphSphJoint>(joint);
            auto link = std::make_shared<chrono::ChLinkDistance>();
            link->SetName(joint->getName());
            // Marker positions in body-local coords
            link->Initialize(
                body1,
                body2,
                true,
                frame1.GetPos(),
                frame2.GetPos(),
                false,
                solverJoint->getDistance()
            );
            sys->AddLink(link);
            break;
        }

        case JointClass::ANGLE_JOINT:
            FC_WARN(
                "ChronoSolver: AngleJoint not directly supported; joint '" << joint->getName()
                                                                           << "' skipped"
            );
            break;

        case JointClass::REV_CYL_JOINT:
            FC_WARN(
                "ChronoSolver: RevCylJoint not supported; joint '" << joint->getName() << "' skipped"
            );
            break;

        case JointClass::CYL_SPH_JOINT:
            FC_WARN(
                "ChronoSolver: CylSphJoint not supported; joint '" << joint->getName() << "' skipped"
            );
            break;

        case JointClass::JOINT:
        default:
            FC_WARN(
                "ChronoSolver: unknown joint type " << static_cast<int>(joint->getJointClass())
                                                    << "; joint '" << joint->getName() << "' skipped"
            );
            break;
    }
}

void ChronoAssembly::addLimit(std::shared_ptr<Limit> limit)
{
    auto key = std::make_pair(limit->getMarkerI(), limit->getMarkerJ());
    auto it = limitableJoints.find(key);
    if (it == limitableJoints.end()) {
        FC_WARN(
            "ChronoSolver: no limitable joint found for limit '" << limit->getName() << "'"
        );
        return;
    }
    auto& link = it->second;

    const double value = evaluateLimitExpression(limit->getLimitExpression());
    const bool isMax = (limit->getType() == LimitType::LESS_THAN_OR_EQUAL);

    switch (limit->getLimitClass()) {
        case LimitClass::ROTATION_LIMIT:
            // The free rotational DOF for Revolute and Cylindrical joints is Rz
            // (rotation about the joint's Z axis).
            if (isMax) {
                link->LimitRz().SetMax(value);
            }
            else {
                link->LimitRz().SetMin(value);
            }
            link->LimitRz().SetActive(true);
            break;

        case LimitClass::TRANSLATION_LIMIT:
            // The free translational DOF for Prismatic and Cylindrical joints is Z
            // (translation along the joint's Z axis).
            if (isMax) {
                link->LimitZ().SetMax(value);
            }
            else {
                link->LimitZ().SetMin(value);
            }
            link->LimitZ().SetActive(true);
            break;

        default:
            FC_WARN(
                "ChronoSolver: unknown limit class for limit '" << limit->getName() << "'"
            );
    }
}

void ChronoAssembly::addMotion(std::shared_ptr<Motion> /*motion*/)
{
    FC_WARN("ChronoSolver: joint motions are not yet implemented; motion ignored");
}

void ChronoAssembly::dumpStructure() const
{
    FC_MSG("=== ChronoSolver: Assembly Structure ===");

    // Ground body
    FC_MSG("[GROUND] fixed=true  pos=(0,0,0)");
    for (const auto& [name, frame] : groundMarkers) {
        auto p = frame.GetPos();
        auto z = frame.GetRot().GetAxisZ();
        FC_MSG(
            "  marker '" << name << "'"
            << "  local_pos=(" << p.x() << "," << p.y() << "," << p.z() << ")"
            << "  local_z=(" << z.x() << "," << z.y() << "," << z.z() << ")"
        );
    }

    // Parts
    FC_MSG("--- Parts (" << parts.size() << ") ---");
    for (const auto& part : parts) {
        auto body = part->getBody();
        auto pos = body->GetPos();
        auto rot = body->GetRot();
        FC_MSG(
            "[PART] '" << body->GetName() << "'"
            << "  fixed=" << (body->IsFixed() ? "true" : "false")
            << "  pos=(" << pos.x() << "," << pos.y() << "," << pos.z() << ")"
            << "  rot(w,x,y,z)=(" << rot.e0() << "," << rot.e1() << "," << rot.e2() << "," << rot.e3() << ")"
        );
        part->forEachMarker([&](const std::string& mname, const chrono::ChFrame<double>& frame) {
            auto lp = frame.GetPos();
            auto lz = frame.GetRot().GetAxisZ();
            auto wz = rot.Rotate(lz);
            auto wp = pos + rot.Rotate(lp);
            FC_MSG(
                "  marker '" << mname << "'"
                << "  world_pos=(" << wp.x() << "," << wp.y() << "," << wp.z() << ")"
                << "  world_z=(" << wz.x() << "," << wz.y() << "," << wz.z() << ")"
            );
        });
    }

    // Links
    FC_MSG("--- Links (" << sys->GetLinks().size() << ") ---");
    for (const auto& link : sys->GetLinks()) {
        if (!link) {
            continue;
        }
        FC_MSG(
            "[LINK] '" << link->GetName() << "'"
            << "  bilateral_dof=" << link->GetNumConstraintsBilateral()
        );
    }

    FC_MSG("=== End Assembly Structure ===");
}

int ChronoAssembly::solveStatic()
{
    if (debugLogging) {
        dumpStructure();
    }

    bool ok = sys->DoAssembly(chrono::AssemblyLevel::POSITION);
    if (debugLogging) {
        FC_MSG("=== Post-solve (converged=" << (ok ? "true" : "false") << ") ===");
        dumpStructure();
    }
    if (ok) {
        solved = true;
        return 0;
    }
    FC_WARN("ChronoSolver: DoAssembly did not converge");
    return 1;
}

int ChronoAssembly::runKinematic()
{
    if (!simulationParameters) {
        FC_ERR("ChronoSolver: runKinematic called without simulation parameters");
        return 1;
    }

    kinematicFrames.clear();

    const double tStart = simulationParameters->getTimeStart();
    const double tEnd = simulationParameters->getTimeEnd();
    const double dt = simulationParameters->getTimeStepOutput();

    sys->SetChTime(tStart);

    for (double t = tStart; t <= tEnd + dt * 0.5; t += dt) {
        sys->DoAssembly(chrono::AssemblyLevel::POSITION);

        // Snapshot current body positions
        std::vector<BodyFrameSnapshot> snapshot;
        for (const auto& part : parts) {
            auto body = part->getBody();
            snapshot.push_back({body, chrono::ChFrame<double>(body->GetPos(), body->GetRot())});
        }
        kinematicFrames.push_back(std::move(snapshot));

        sys->SetChTime(t + dt);
    }

    solved = true;
    return 0;
}

void ChronoAssembly::preDrag()
{
    sys->DoAssembly(chrono::AssemblyLevel::POSITION);

    // Snapshot all body positions so the first drag step has a known-good start.
    saveDragStepStart();
}

void ChronoAssembly::saveDragStepStart()
{
    dragStepStart.clear();
    for (const auto& part : parts) {
        auto body = part->getBody();
        dragStepStart[body.get()] = {body->GetPos(), body->GetRot()};
    }
}

void ChronoAssembly::dragStep(std::vector<std::shared_ptr<Part>> draggedParts)
{
    // Sub-step the drag to prevent Newton-Raphson from jumping to a different
    // kinematic branch.  pushPlacement() has already moved each dragged body to
    // its final target position.  We interpolate each dragged body linearly
    // from its position at the start of this drag event step to its target
    // across N sub-steps, calling DoAssembly for each.  This keeps each
    // Newton-Raphson solve close to the previous solution, so the solver tracks
    // the correct branch through the constraint manifold.
    static constexpr int kSubSteps = 20;

    // Collect dragged bodies and their target positions (already set by pushPlacement).
    struct BodyTarget
    {
        std::shared_ptr<chrono::ChBody> body;
        chrono::ChVector3d targetPos;
        chrono::ChQuaternion<double> targetRot;
        chrono::ChVector3d startPos;
        chrono::ChQuaternion<double> startRot;
    };
    std::vector<BodyTarget> targets;
    for (const auto& part : draggedParts) {
        auto chronoPart = std::static_pointer_cast<ChronoPart>(part);
        auto body = chronoPart->getBody();
        BodyTarget t;
        t.body = body;
        t.targetPos = body->GetPos();
        t.targetRot = body->GetRot();
        auto it = dragStepStart.find(body.get());
        if (it != dragStepStart.end()) {
            t.startPos = it->second.pos;
            t.startRot = it->second.rot;
        }
        else {
            t.startPos = t.targetPos;
            t.startRot = t.targetRot;
        }
        targets.push_back(t);
    }

    if (debugLogging) {
        FC_MSG("=== dragStep: " << kSubSteps << " sub-steps, " << targets.size() << " dragged body/bodies ===");
        for (const auto& t : targets) {
            auto delta = (t.targetPos - t.startPos).Length();
            FC_MSG(
                "  dragged '" << t.body->GetName() << "'"
                << "  start=(" << t.startPos.x() << "," << t.startPos.y() << "," << t.startPos.z() << ")"
                << "  target=(" << t.targetPos.x() << "," << t.targetPos.y() << "," << t.targetPos.z() << ")"
                << "  delta=" << delta
            );
        }
    }

    for (int k = 1; k <= kSubSteps; k++) {
        const double alpha = static_cast<double>(k) / kSubSteps;

        // Move each dragged body to the interpolated position for this sub-step.
        for (const auto& t : targets) {
            t.body->SetPos(t.startPos + (t.targetPos - t.startPos) * alpha);
            // Normalized linear interpolation (NLERP) of the rotation.
            // For the small per-step rotations involved in interactive drag this
            // is a good approximation of SLERP and avoids relying on a
            // quaternion slerp API that Chrono does not expose directly.
            chrono::ChQuaternion<double> q(
                t.startRot.e0() + (t.targetRot.e0() - t.startRot.e0()) * alpha,
                t.startRot.e1() + (t.targetRot.e1() - t.startRot.e1()) * alpha,
                t.startRot.e2() + (t.targetRot.e2() - t.startRot.e2()) * alpha,
                t.startRot.e3() + (t.targetRot.e3() - t.startRot.e3()) * alpha
            );
            q.Normalize();
            t.body->SetRot(q);
        }

        bool ok = sys->DoAssembly(chrono::AssemblyLevel::POSITION);

        if (debugLogging) {
            FC_MSG("  sub-step " << k << "/" << kSubSteps << " converged=" << (ok ? "true" : "false"));
        }
    }

    if (debugLogging) {
        FC_MSG("=== dragStep complete: post-solve structure ===");
        dumpStructure();
    }

    // Fix quaternion sign: DoAssembly can flip a body's quaternion to the
    // antipodal representation (-q instead of +q, same physical rotation).
    // If the sign differs from our target quaternion, the NLERP in the NEXT
    // drag step would interpolate through the antipodal path (≈360° rotation).
    // Canonicalize each dragged body's quaternion so it lies in the same
    // hemisphere as the target.
    //
    // NOTE: We deliberately do NOT restore the body's position to t.targetPos.
    // DoAssembly has adjusted the dragged body to satisfy hard constraints
    // (e.g., keeping a revolute pivot coincident).  Forcing the position back
    // to the exact user-dragged target re-introduces that constraint violation,
    // and saveDragStepStart() would then record an inconsistent state that
    // causes Newton-Raphson in the next step to diverge, producing growing
    // positional drift in the joint markers.
    for (const auto& t : targets) {
        auto rot = t.body->GetRot();
        if (rot.Dot(t.targetRot) < 0.0) {
            t.body->SetRot(chrono::ChQuaternion<double>(-rot.e0(), -rot.e1(), -rot.e2(), -rot.e3()));
        }
    }

    // Save current body positions as the start for the next drag step.
    saveDragStepStart();
}

void ChronoAssembly::postDrag()
{
    if (debugLogging) {
        FC_MSG("=== postDrag: final assembly structure ===");
        dumpStructure();
    }
    dragStepStart.clear();
}

void ChronoAssembly::setSimulationParameters(std::shared_ptr<SimulationParameters> params)
{
    simulationParameters = params;
}

std::shared_ptr<SimulationParameters> ChronoAssembly::getSimulationParameters() const
{
    return simulationParameters;
}

size_t ChronoAssembly::numberOfFrames() const
{
    return kinematicFrames.size();
}

void ChronoAssembly::updateForFrame(size_t index)
{
    if (index >= kinematicFrames.size()) {
        return;
    }
    for (const auto& snap : kinematicFrames[index]) {
        snap.body->SetPos(snap.frame.GetPos());
        snap.body->SetRot(snap.frame.GetRot());
    }
}

bool ChronoAssembly::hasSolvedSystem() const
{
    return solved;
}

SolveStatus ChronoAssembly::querySolveStatus()
{
    SolveStatus status;

    if (!solved) {
        return status;
    }

    // Count bilateral constraints contributed by each link.
    for (const auto& link : sys->GetLinks()) {
        if (!link) {
            continue;
        }
        SolveStatus::JointInfo info;
        info.name = link->GetName();
        info.isRedundant = false;

        const unsigned int ncon = link->GetNumConstraintsBilateral();
        status.constraintsApplied += static_cast<int>(ncon);
        status.joints.push_back(info);
    }

    return status;
}

void ChronoAssembly::exportFile(std::string /*filename*/)
{
    FC_WARN("ChronoSolver: exportFile is not supported");
}

void ChronoAssembly::setDebug(bool debug)
{
    debugLogging = debug;
}

// ============================================================================
// ChronoSolver
// ============================================================================

ChronoSolver::ChronoSolver(AssemblyObject* asmObj)
    : assemblyObject(asmObj)
{}


std::shared_ptr<Solver::Assembly> ChronoSolver::makeAssembly()
{
    auto assembly = std::make_shared<ChronoAssembly>();
    assembly->setName("ChronoAssembly");

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Assembly"
    );
    assembly->setDebug(hGrp->GetBool("LogSolverDebug", false));

    return assembly;
}

std::shared_ptr<Solver::Part> ChronoSolver::makePart()
{
    return std::make_shared<ChronoPart>();
}

#endif  // HAVE_CHRONO
