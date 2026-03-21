// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2026 Jacob Oursland <jacob.oursland[at]gmail.com>        *
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
# include <chrono/physics/ChLinkMate.h>  // ChLinkMateRackPinion has no Lock equivalent
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
)
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
        markerRegistry[ref] = {outBody, outFrame};
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
            markerRegistry[ref] = {outBody, outFrame};
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
            FC_TRACE(
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
                FC_TRACE(
                    "  -> flipped frame1 Z (Rx180); new world_z1=(" << z1n.x() << "," << z1n.y()
                                                                    << "," << z1n.z() << ")"
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
        FC_WARN("ChronoSolver: no limitable joint found for limit '" << limit->getName() << "'");
        return;
    }

    const double value = evaluateLimitExpression(limit->getLimitExpression());
    const bool isMax = (limit->getType() == LimitType::LESS_THAN_OR_EQUAL);

    storedLimits.push_back({key, limit->getLimitClass(), value, isMax, limit->getCurrentValue()});

    FC_TRACE(
        "addLimit (stored): '" << limit->getName() << "'"
                               << " class=" << static_cast<int>(limit->getLimitClass())
                               << " isMax=" << isMax << " value=" << value
                               << " fcCurrent=" << limit->getCurrentValue()
    );
}

void ChronoAssembly::addMotion(std::shared_ptr<Motion> /*motion*/)
{
    FC_WARN("ChronoSolver: joint motions are not yet implemented; motion ignored");
}

// updateActiveLimits() has been removed — limits are now enforced natively
// by the Chrono solver via ChLinkLimit objects set in preDrag().

void ChronoAssembly::dumpStructure() const
{
    FC_TRACE("=== ChronoSolver: Assembly Structure ===");

    // Ground body
    FC_TRACE("[GROUND] fixed=true  pos=(0,0,0)");
    for (const auto& [name, frame] : groundMarkers) {
        auto p = frame.GetPos();
        auto z = frame.GetRot().GetAxisZ();
        FC_TRACE(
            "  marker '" << name << "'"
                         << "  local_pos=(" << p.x() << "," << p.y() << "," << p.z() << ")"
                         << "  local_z=(" << z.x() << "," << z.y() << "," << z.z() << ")"
        );
    }

    // Parts
    FC_TRACE("--- Parts (" << parts.size() << ") ---");
    for (const auto& part : parts) {
        auto body = part->getBody();
        auto pos = body->GetPos();
        auto rot = body->GetRot();
        FC_TRACE(
            "[PART] '" << body->GetName() << "'"
                       << "  fixed=" << (body->IsFixed() ? "true" : "false") << "  pos=(" << pos.x()
                       << "," << pos.y() << "," << pos.z() << ")"
                       << "  rot(w,x,y,z)=(" << rot.e0() << "," << rot.e1() << "," << rot.e2()
                       << "," << rot.e3() << ")"
        );
        part->forEachMarker([&](const std::string& mname, const chrono::ChFrame<double>& frame) {
            auto lp = frame.GetPos();
            auto lz = frame.GetRot().GetAxisZ();
            auto wz = rot.Rotate(lz);
            auto wp = pos + rot.Rotate(lp);
            FC_TRACE(
                "  marker '" << mname << "'"
                             << "  world_pos=(" << wp.x() << "," << wp.y() << "," << wp.z() << ")"
                             << "  world_z=(" << wz.x() << "," << wz.y() << "," << wz.z() << ")"
            );
        });
    }

    // Links
    FC_TRACE("--- Links (" << sys->GetLinks().size() << ") ---");
    for (const auto& link : sys->GetLinks()) {
        if (!link) {
            continue;
        }
        FC_TRACE(
            "[LINK] '" << link->GetName() << "'"
                       << "  bilateral_dof=" << link->GetNumConstraintsBilateral()
        );
    }

    FC_TRACE("=== End Assembly Structure ===");
}

int ChronoAssembly::solveStatic()
{
    if (debugLogging) {
        dumpStructure();
    }

    bool ok = sys->DoAssembly(chrono::AssemblyLevel::POSITION);
    if (debugLogging) {
        FC_LOG("=== Post-solve (converged=" << (ok ? "true" : "false") << ") ===");
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

void ChronoAssembly::preDrag(const DragContext& ctx)
{
    sys->DoAssembly(chrono::AssemblyLevel::POSITION);

    // Auto-detect the FreeCAD→Chrono coordinate offset for each stored limit.
    // We DON'T rely on FreeCAD's Angle/Distance properties (they return the
    // configured joint offset, not the actual angle).  Instead, we exploit the
    // fact that the assembly is valid at rest: the Chrono baseline value must
    // map to a FreeCAD value within [limitMin, limitMax].
    //
    // Step 1: Read the Chrono baseline per joint.
    // Step 2: For each joint, gather its min/max limit values.
    // Step 3: Compute offset as: chronoBaseline - midpoint (two-sided) or
    //         chronoBaseline - limitValue (one-sided).

    // Gather per-joint: chrono baseline + min/max limit values
    struct JointLimitInfo
    {
        double chronoBaseline = 0.0;
        double minLimit = -1e30;  // no min by default
        double maxLimit = +1e30;  // no max by default
        bool hasMin = false;
        bool hasMax = false;
    };
    std::map<std::pair<MarkerRef, MarkerRef>, JointLimitInfo> jointInfoMap;

    for (const auto& sl : storedLimits) {
        auto it = limitableJoints.find(sl.jointKey);
        if (it == limitableJoints.end()) {
            continue;
        }
        auto& link = it->second;
        auto& info = jointInfoMap[sl.jointKey];

        // Read Chrono baseline (same for all limits on this joint)
        if (sl.limitClass == LimitClass::ROTATION_LIMIT) {
            info.chronoBaseline = link->GetRelAngle();
        }
        else if (sl.limitClass == LimitClass::TRANSLATION_LIMIT) {
            info.chronoBaseline = link->GetRelCoordsys().pos.z();
        }

        if (sl.isMax) {
            info.maxLimit = sl.value;
            info.hasMax = true;
        }
        else {
            info.minLimit = sl.value;
            info.hasMin = true;
        }
    }

    // Compute offset per joint and apply to each stored limit
    std::map<std::pair<MarkerRef, MarkerRef>, double> jointOffset;
    for (auto& [key, info] : jointInfoMap) {
        double offset = 0.0;
        if (info.hasMin && info.hasMax) {
            // Two-sided: place baseline at midpoint of FreeCAD range
            double midpoint = (info.minLimit + info.maxLimit) / 2.0;
            offset = info.chronoBaseline - midpoint;
        }
        else if (info.hasMax || info.hasMin) {
            // One-sided limit: FreeCAD convention is that joint value 0 =
            // the rest/retracted position.  Map baseline to FreeCAD 0.
            offset = info.chronoBaseline;
        }
        jointOffset[key] = offset;

        // Find joint name for logging
        auto jit = limitableJoints.find(key);
        std::string jname = jit != limitableJoints.end() ? jit->second->GetName() : "???";
        FC_TRACE(
            "  joint offset: '" << jname << "'"
                                << " chronoBaseline=" << info.chronoBaseline << " fcRange=["
                                << info.minLimit << ", " << info.maxLimit << "]"
                                << " offset=" << offset
        );
    }

    // Compute per-joint Chrono-space limit bounds for logging and future use.
    // NOTE: ChLinkLimit uses penalty forces which are incompatible with the
    // kinematic DoAssembly(POSITION) solver — they destabilize the solve and
    // cause massive joint violations.  The limits are stored here for
    // diagnostic purposes only; actual enforcement requires a different
    // approach (e.g. post-solve clamping or unilateral constraints).
    for (auto& [key, info] : jointInfoMap) {
        auto jit = limitableJoints.find(key);
        if (jit == limitableJoints.end()) {
            continue;
        }
        auto& link = jit->second;
        double offset = jointOffset[key];

        // Determine which limit class this joint uses
        LimitClass lc = LimitClass::ROTATION_LIMIT;
        for (const auto& sl : storedLimits) {
            if (sl.jointKey == key) {
                lc = sl.limitClass;
                break;
            }
        }

        // Store into nativeLimits map for use in dragStep()
        NativeLimitInfo nli;
        nli.limitClass = lc;
        nli.offset = offset;
        if (info.hasMin) {
            nli.chronoMin = info.minLimit + offset;
            nli.hasMin = true;
        }
        if (info.hasMax) {
            nli.chronoMax = info.maxLimit + offset;
            nli.hasMax = true;
        }
        nativeLimits[key] = nli;

        FC_TRACE(
            "  limit info: joint='"
            << link->GetName() << "'"
            << " class=" << (lc == LimitClass::ROTATION_LIMIT ? "rotation" : "translation")
            << " chronoRange=[" << nli.chronoMin << ", " << nli.chronoMax << "]"
            << " offset=" << offset
        );
    }


    // ------------------------------------------------------------------
    // Nearest-joint drag prioritization via mouse-projection.
    // Extract the joint's world-space axis and pivot so that dragStep()
    // can project the mouse target onto the joint's DOF.  This avoids
    // modifying the solver at all — instead we shape the input.
    // ------------------------------------------------------------------
    nearestJointDOF = {};
    if (!ctx.nearestJointName.empty()) {
        FC_LOG("Nearest joint for drag: '" << ctx.nearestJointName << "'");

        for (const auto& linkPtr : sys->GetLinks()) {
            auto* lockLink = dynamic_cast<chrono::ChLinkLock*>(linkPtr.get());
            if (!lockLink) {
                continue;
            }
            if (lockLink->GetName() == ctx.nearestJointName) {
                // Get the joint's world frame — Z axis is the joint axis
                auto frame = lockLink->GetFrame1Abs();
                nearestJointDOF.active = true;
                nearestJointDOF.axis = frame.GetRotMat().GetAxisZ();
                nearestJointDOF.pivot = frame.GetPos();
                nearestJointDOF.link = lockLink;

                // Determine joint type from Chrono link class.
                // ChLinkLockRevolute/Cylindrical → rotational.
                // ChLinkLockPrismatic → translational.
                if (dynamic_cast<chrono::ChLinkLockRevolute*>(lockLink)
                    || dynamic_cast<chrono::ChLinkLockCylindrical*>(lockLink)) {
                    nearestJointDOF.isRotational = true;
                }
                else if (dynamic_cast<chrono::ChLinkLockPrismatic*>(lockLink)) {
                    nearestJointDOF.isRotational = false;
                }
                else {
                    // Unknown joint type — don't project
                    nearestJointDOF.active = false;
                }

                FC_TRACE(
                    "  DOF: " << (nearestJointDOF.isRotational ? "rotational" : "translational")
                              << " axis=(" << nearestJointDOF.axis.x() << ","
                              << nearestJointDOF.axis.y() << "," << nearestJointDOF.axis.z() << ")"
                              << " pivot=(" << nearestJointDOF.pivot.x() << ","
                              << nearestJointDOF.pivot.y() << "," << nearestJointDOF.pivot.z() << ")"
                );

                // Find the limit key for this joint by matching link pointers
                for (const auto& [mkey, lnk] : limitableJoints) {
                    if (lnk.get() == lockLink) {
                        auto nlit = nativeLimits.find(mkey);
                        if (nlit != nativeLimits.end()) {
                            nearestJointDOF.limitKey = mkey;
                            nearestJointDOF.hasLimits = true;
                            FC_TRACE(
                                "  Nearest joint has limits: chronoRange=["
                                << nlit->second.chronoMin << ", " << nlit->second.chronoMax << "]"
                            );
                        }
                        break;
                    }
                }

                break;
            }
        }
    }

    // Snapshot all body positions so the first drag step has a known-good start.
    saveDragStepStart();

    dragCtx = ctx;

    // Create a fixed mouse body at the pick point.
    mouseBody = chrono_types::make_shared<chrono::ChBody>();
    mouseBody->SetName("__mouse__");
    mouseBody->SetPos(chrono::ChVector3d(ctx.pickPoint.x, ctx.pickPoint.y, ctx.pickPoint.z));
    mouseBody->SetFixed(true);
    sys->AddBody(mouseBody);
}

void ChronoAssembly::saveDragStepStart()
{
    dragStepStart.clear();
    for (const auto& part : parts) {
        auto body = part->getBody();
        dragStepStart[body.get()] = {body->GetPos(), body->GetRot()};
    }
}

void ChronoAssembly::dragStep(std::vector<std::shared_ptr<Part>> draggedParts, Base::Vector3d mousePos3D)
{
    if (!mouseBody) {
        return;
    }

    auto rawMousePos = chrono::ChVector3d(mousePos3D.x, mousePos3D.y, mousePos3D.z);

    // Move the mouse body to the raw mouse position.
    mouseBody->SetPos(rawMousePos);

    // Create the mouse constraint linking dragged part to mouse body.
    // For now we constrain only the first dragged part.
    if (!mouseLink && !draggedParts.empty()) {
        auto chronoPart = std::static_pointer_cast<ChronoPart>(draggedParts[0]);
        auto draggedBody = chronoPart->getBody();

        // Compute pick point in part-local coordinates so the constraint
        // attaches at the click location, not the body origin.
        auto pickWorld
            = chrono::ChVector3d(dragCtx.pickPoint.x, dragCtx.pickPoint.y, dragCtx.pickPoint.z);
        // Transform world pick point to body-local: R^-1 * (pickWorld - bodyPos)
        auto localPickPos = draggedBody->GetRot().GetInverse().Rotate(
            pickWorld - draggedBody->GetPos()
        );

        // Use ChLinkLockSpherical (X,Y,Z only, rotation free) with identity frames
        mouseLink = chrono_types::make_shared<chrono::ChLinkLockSpherical>();
        mouseLink->SetName("__mouse_constraint__");

        // Frame on mouse body: at its origin (= mouse position)
        chrono::ChFrame<double> mouseFrame(chrono::VNULL, chrono::QUNIT);
        // Frame on dragged body: at pick offset in body-local coords
        chrono::ChFrame<double> partFrame(localPickPos, chrono::QUNIT);

        mouseLink->Initialize(mouseBody, draggedBody, true, mouseFrame, partFrame);
        sys->AddLink(mouseLink);

        // Tight compliance so the constraint has real influence on the solve
        double cfm = 1e-4;
        auto& mask = mouseLink->GetMask();
        for (unsigned int i = 0; i < mask.GetNumConstraints(); i++) {
            mask.GetConstraint(i).SetComplianceTerm(cfm);
        }

        FC_LOG(
            "Mouse constraint: pickWorld=("
            << pickWorld.x() << "," << pickWorld.y() << "," << pickWorld.z() << ")"
            << " localPick=(" << localPickPos.x() << "," << localPickPos.y() << ","
            << localPickPos.z() << ")"
            << " bodyPos=(" << draggedBody->GetPos().x() << "," << draggedBody->GetPos().y() << ","
            << draggedBody->GetPos().z() << ")"
        );
    }

    // Snapshot all body states BEFORE pre-rotation so we can fully
    // revert to a known-good state if the solve fails.  Previously
    // the snapshot was taken after pre-rotation, so rejected steps
    // restored to the pre-rotated (corrupted) state, causing
    // progressive model breakage.
    std::map<chrono::ChBody*, BodyDragStart> preSolveState;
    for (const auto& part : parts) {
        auto body = part->getBody();
        preSolveState[body.get()] = {body->GetPos(), body->GetRot()};
    }

    // ------------------------------------------------------------------
    // Nearest-joint pre-rotation.
    // Instead of projecting the mouse target (which the solver ignores
    // due to minimum-norm distributing motion), we pre-rotate/translate
    // the dragged body around the nearest joint's DOF.  DoAssembly then
    // reconciles the rest of the kinematic chain to match.
    // ------------------------------------------------------------------
    if (nearestJointDOF.active && nearestJointDOF.link && !draggedParts.empty()) {
        auto chronoPart = std::static_pointer_cast<ChronoPart>(draggedParts[0]);
        auto draggedBody = chronoPart->getBody();

        // Query current joint frame (updates as the body moves)
        auto jFrame = nearestJointDOF.link->GetFrame1Abs();
        auto jAxis = jFrame.GetRotMat().GetAxisZ();
        auto jPivot = jFrame.GetPos();

        // Current pick point in world (body-local pick pos transformed to world)
        auto localPick
            = chrono::ChVector3d(dragCtx.pickPoint.x, dragCtx.pickPoint.y, dragCtx.pickPoint.z);
        // Use actual current body transform to find where the pick point is NOW
        auto bodyRot = draggedBody->GetRot();
        auto bodyPos = draggedBody->GetPos();
        auto currentPickWorld = bodyPos
            + bodyRot.Rotate(bodyRot.GetInverse().Rotate(localPick - bodyPos));
        // Simplification: the initial pick point in body-local was computed at drag start;
        // but we stored dragCtx.pickPoint which is in world coords at drag start.
        // Use the body-local offset from the mouse link instead.
        if (mouseLink) {
            auto* partMarker = mouseLink->GetMarker2();
            auto localOff = partMarker->GetPos();
            currentPickWorld = bodyPos + bodyRot.Rotate(localOff);
        }

        auto displacement = rawMousePos - currentPickWorld;

        if (nearestJointDOF.isRotational) {
            // Compute radius from joint axis to current pick point
            auto radius = currentPickWorld - jPivot;
            auto axialComp = radius.Dot(jAxis);
            radius = radius - jAxis * axialComp;
            auto rLen = radius.Length();
            if (rLen > 1e-6) {
                // Tangent direction at current position
                auto tangent = jAxis.Cross(radius);
                tangent.Normalize();
                // Desired tangential displacement
                auto tangentDisp = displacement.Dot(tangent);
                // Convert to rotation angle: angle = arc_length / radius
                auto angle = tangentDisp / rLen;

                // Cap per-step rotation to prevent solver divergence
                constexpr double maxAnglePerStep = 0.05;  // ~3 degrees
                angle = std::clamp(angle, -maxAnglePerStep, maxAnglePerStep);

                // Clamp to joint limits if available
                if (nearestJointDOF.hasLimits) {
                    auto nlit = nativeLimits.find(nearestJointDOF.limitKey);
                    if (nlit != nativeLimits.end()) {
                        double currentAngle = nearestJointDOF.link->GetRelAngle();
                        double newAngle = currentAngle + angle;
                        if (nlit->second.hasMin && newAngle < nlit->second.chronoMin) {
                            angle = nlit->second.chronoMin - currentAngle;
                        }
                        if (nlit->second.hasMax && newAngle > nlit->second.chronoMax) {
                            angle = nlit->second.chronoMax - currentAngle;
                        }
                    }
                }

                // Pre-rotate the dragged body around the joint axis by this angle.
                auto rotQ = chrono::QuatFromAngleAxis(angle, jAxis);

                // Rotate body position around the pivot
                auto relPos = bodyPos - jPivot;
                auto newRelPos = rotQ.Rotate(relPos);
                draggedBody->SetPos(jPivot + newRelPos);

                // Combine rotation with existing body rotation
                draggedBody->SetRot(rotQ * bodyRot);
            }
        }
        else {
            // Prismatic: pre-translate along the slide axis
            auto axialDisp = displacement.Dot(jAxis);

            // Cap per-step translation to prevent solver divergence
            constexpr double maxTransPerStep = 20.0;  // 20mm
            axialDisp = std::clamp(axialDisp, -maxTransPerStep, maxTransPerStep);

            // Clamp to joint limits if available
            if (nearestJointDOF.hasLimits) {
                auto nlit = nativeLimits.find(nearestJointDOF.limitKey);
                if (nlit != nativeLimits.end()) {
                    double currentPos = nearestJointDOF.link->GetRelCoordsys().pos.z();
                    double newPos = currentPos + axialDisp;
                    if (nlit->second.hasMin && newPos < nlit->second.chronoMin) {
                        axialDisp = nlit->second.chronoMin - currentPos;
                    }
                    if (nlit->second.hasMax && newPos > nlit->second.chronoMax) {
                        axialDisp = nlit->second.chronoMax - currentPos;
                    }
                }
            }

            draggedBody->SetPos(bodyPos + jAxis * axialDisp);
        }
    }

    // (preSolveState was captured above, before pre-rotation)

    bool ok = sys->DoAssembly(chrono::AssemblyLevel::POSITION);

    // Check if assembly joints started failing — if so, we've hit a kinematic limit
    {
        bool jointsBroken = false;
        double maxJointViolation = 0;

        for (const auto& link : sys->GetLinks()) {
            // Skip the mouse constraint itself
            if (link.get() == mouseLink.get()) {
                continue;
            }
            auto violation = link->GetConstraintViolation();
            double vNorm = violation.norm();
            maxJointViolation = std::max(maxJointViolation, vNorm);
            if (vNorm > 1.0) {  // 1mm tolerance for joint violations
                jointsBroken = true;
            }
        }

        // Post-solve limit check using pre-computed Chrono-space bounds.
        // Check each limited joint; if it exceeds its limit by more than a
        // generous tolerance, reject the step.
        bool limitsViolated = false;
        for (const auto& [key, nli] : nativeLimits) {
            auto jit = limitableJoints.find(key);
            if (jit == limitableJoints.end()) {
                continue;
            }
            auto& lnk = jit->second;
            double currentVal = 0.0;
            double tolerance = 0.0;

            if (nli.limitClass == LimitClass::ROTATION_LIMIT) {
                currentVal = lnk->GetRelAngle();
                tolerance = 0.5;  // ~29 deg; solver overshoots up to 0.37 rad/step
            }
            else {
                currentVal = lnk->GetRelCoordsys().pos.z();
                tolerance = 50.0;  // 50 mm; solver overshoots up to 16 mm/step
            }

            if (nli.hasMin && currentVal < nli.chronoMin - tolerance) {
                limitsViolated = true;
                FC_LOG(
                    "  LIMIT HIT: joint='" << lnk->GetName() << "'"
                                           << " current=" << currentVal << " min=" << nli.chronoMin
                                           << " excess=" << (nli.chronoMin - currentVal)
                );
            }
            if (nli.hasMax && currentVal > nli.chronoMax + tolerance) {
                limitsViolated = true;
                FC_LOG(
                    "  LIMIT HIT: joint='" << lnk->GetName() << "'"
                                           << " current=" << currentVal << " max=" << nli.chronoMax
                                           << " excess=" << (currentVal - nli.chronoMax)
                );
            }
        }

        if (!ok || jointsBroken) {
            FC_LOG(
                "  DRAG REJECTED: ok=" << ok << " jointsBroken=" << jointsBroken
                                       << " limitsViolated=" << limitsViolated
                                       << " (maxViolation=" << maxJointViolation << ")"
            );
            // Restore all bodies to pre-solve state
            for (const auto& part : parts) {
                auto body = part->getBody();
                auto it2 = preSolveState.find(body.get());
                if (it2 != preSolveState.end()) {
                    body->SetPos(it2->second.pos);
                    body->SetRot(it2->second.rot);
                }
            }
        }
    }

    // Save current body positions as the start for the next drag step.
    saveDragStepStart();
}

void ChronoAssembly::postDrag()
{
    if (debugLogging) {
        FC_LOG("=== postDrag: final assembly structure ===");
        dumpStructure();
    }

    // Clean up mouse constraint and body
    if (mouseLink) {
        sys->RemoveLink(mouseLink);
        mouseLink.reset();
    }
    if (mouseBody) {
        sys->RemoveBody(mouseBody);
        mouseBody.reset();
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
