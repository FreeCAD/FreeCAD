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

#include "OndselSolver.h"

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Placement.h>

#include <OndselSolver/CREATE.h>
#include <OndselSolver/ASMTAssembly.h>
#include <OndselSolver/ASMTAngleJoint.h>
#include <OndselSolver/ASMTCylSphJoint.h>
#include <OndselSolver/ASMTCylindricalJoint.h>
#include <OndselSolver/ASMTFixedJoint.h>
#include <OndselSolver/ASMTGearJoint.h>
#include <OndselSolver/ASMTGeneralMotion.h>
#include <OndselSolver/ASMTJoint.h>
#include <OndselSolver/ASMTLineInPlaneJoint.h>
#include <OndselSolver/ASMTMarker.h>
#include <OndselSolver/ASMTParallelAxesJoint.h>
#include <OndselSolver/ASMTPerpendicularJoint.h>
#include <OndselSolver/ASMTPart.h>
#include <OndselSolver/ASMTPlanarJoint.h>
#include <OndselSolver/ASMTPointInLineJoint.h>
#include <OndselSolver/ASMTPointInPlaneJoint.h>
#include <OndselSolver/ASMTRackPinionJoint.h>
#include <OndselSolver/ASMTRevCylJoint.h>
#include <OndselSolver/ASMTRevoluteJoint.h>
#include <OndselSolver/ASMTRotationLimit.h>
#include <OndselSolver/ASMTRotationalMotion.h>
#include <OndselSolver/ASMTScrewJoint.h>
#include <OndselSolver/ASMTSimulationParameters.h>
#include <OndselSolver/ASMTSphericalJoint.h>
#include <OndselSolver/ASMTSphSphJoint.h>
#include <OndselSolver/ASMTTranslationLimit.h>
#include <OndselSolver/ASMTTranslationalJoint.h>
#include <OndselSolver/ASMTTranslationalMotion.h>
#include <OndselSolver/Constraint.h>
#include <OndselSolver/ExternalSystem.h>
#include <OndselSolver/FullColumn.h>
#include <OndselSolver/Joint.h>

#include "AssemblyObject.h"

FC_LOG_LEVEL_INIT("OndselSolver", true, true, true)

using namespace Assembly;
using namespace Assembly::Solver;

// Helper: convert a Base::Placement into a rotation matrix row vectors
static void placementToRotMatrix(
    const Base::Placement& plc,
    Base::Vector3d& r0,
    Base::Vector3d& r1,
    Base::Vector3d& r2
)
{
    Base::Matrix4D mat;
    plc.getRotation().getValue(mat);
    r0 = mat.getRow(0);
    r1 = mat.getRow(1);
    r2 = mat.getRow(2);
}

// ============================================================================
// OndselPart
// ============================================================================

OndselPart::OndselPart(std::string asmName)
    : assemblyName(std::move(asmName))
{
    asmtPart = MbD::CREATE<MbD::ASMTPart>::With();
}

void OndselPart::setPlacement(Base::Placement plc)
{
    Part::setPlacement(plc);

    Base::Vector3d pos = plc.getPosition();
    asmtPart->setPosition3D(pos.x, pos.y, pos.z);

    Base::Vector3d r0, r1, r2;
    placementToRotMatrix(plc, r0, r1, r2);
    asmtPart->setRotationMatrix(r0.x, r0.y, r0.z, r1.x, r1.y, r1.z, r2.x, r2.y, r2.z);
}

void OndselPart::pushPlacement(Base::Placement plc)
{
    Base::Vector3d pos = plc.getPosition();
    asmtPart->updateMbDFromPosition3D(pos.x, pos.y, pos.z);

    Base::Vector3d r0, r1, r2;
    placementToRotMatrix(plc, r0, r1, r2);
    asmtPart->updateMbDFromRotationMatrix(r0.x, r0.y, r0.z, r1.x, r1.y, r1.z, r2.x, r2.y, r2.z);
}

MarkerRef OndselPart::addMarker(std::string name, Base::Placement plc)
{
    auto asmtMarker = MbD::CREATE<MbD::ASMTMarker>::With();
    asmtMarker->setName(name);

    Base::Vector3d pos = plc.getPosition();
    asmtMarker->setPosition3D(pos.x, pos.y, pos.z);

    Base::Vector3d r0, r1, r2;
    placementToRotMatrix(plc, r0, r1, r2);
    asmtMarker->setRotationMatrix(r0.x, r0.y, r0.z, r1.x, r1.y, r1.z, r2.x, r2.y, r2.z);

    asmtPart->addMarker(asmtMarker);

    return "/" + assemblyName + "/" + asmtPart->name + "/" + name;
}

Base::Placement OndselPart::getPlacement() const
{
    double x, y, z;
    asmtPart->getPosition3D(x, y, z);

    // Note: OndselSolver uses a different quaternion component ordering
    double q0, q1, q2, q3;
    asmtPart->getQuarternions(q3, q0, q1, q2);

    return Base::Placement(Base::Vector3d(x, y, z), Base::Rotation(q0, q1, q2, q3));
}

std::shared_ptr<MbD::ASMTPart> OndselPart::getAsmtPart() const
{
    return asmtPart;
}

// ============================================================================
// OndselAssembly
// ============================================================================

OndselAssembly::OndselAssembly(AssemblyObject* assemblyObject)
{
    asmtAssembly = MbD::CREATE<MbD::ASMTAssembly>::With();
    asmtAssembly->externalSystem->freecadAssemblyObject = assemblyObject;
    asmtAssembly->setName("OndselAssembly");

    ParameterGrp::handle hPgr = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Assembly"
    );
    asmtAssembly->setDebug(hPgr->GetBool("LogSolverDebug", false));
}

void OndselAssembly::addPart(std::shared_ptr<Part> part)
{
    auto ondselPart = std::static_pointer_cast<OndselPart>(part);

    // Initialise the MbD part from the abstract Part's data
    auto asmtPart = ondselPart->getAsmtPart();
    asmtPart->setName(part->getName());

    // Mass properties
    auto massMarker = MbD::CREATE<MbD::ASMTPrincipalMassMarker>::With();
    massMarker->setMass(part->getMass());
    massMarker->setDensity(part->getDensity());
    double ixx, iyy, izz;
    part->getMomentOfInertias(ixx, iyy, izz);
    massMarker->setMomentOfInertias(ixx, iyy, izz);
    asmtPart->setPrincipalMassMarker(massMarker);

    // Initial placement
    Base::Placement plc = part->getPlacement();
    Base::Vector3d pos = plc.getPosition();
    asmtPart->setPosition3D(pos.x, pos.y, pos.z);

    Base::Vector3d r0, r1, r2;
    placementToRotMatrix(plc, r0, r1, r2);
    asmtPart->setRotationMatrix(r0.x, r0.y, r0.z, r1.x, r1.y, r1.z, r2.x, r2.y, r2.z);

    asmtAssembly->addPart(asmtPart);
}

MarkerRef OndselAssembly::addGroundMarker(std::string name, Base::Placement plc)
{
    auto asmtMarker = MbD::CREATE<MbD::ASMTMarker>::With();
    asmtMarker->setName(name);

    Base::Vector3d pos = plc.getPosition();
    asmtMarker->setPosition3D(pos.x, pos.y, pos.z);

    Base::Vector3d r0, r1, r2;
    placementToRotMatrix(plc, r0, r1, r2);
    asmtMarker->setRotationMatrix(r0.x, r0.y, r0.z, r1.x, r1.y, r1.z, r2.x, r2.y, r2.z);

    asmtAssembly->addMarker(asmtMarker);

    return "/" + asmtAssembly->name + "/" + name;
}

void OndselAssembly::addJoint(std::shared_ptr<Joint> joint)
{
    const std::string& name = joint->getName();
    const MarkerRef& mi = joint->getMarkerI();
    const MarkerRef& mj = joint->getMarkerJ();

    auto setCommon = [&](auto& asmtJoint) {
        asmtJoint->setName(name);
        asmtJoint->setMarkerI(mi);
        asmtJoint->setMarkerJ(mj);
    };

    switch (joint->getJointClass()) {
        case JointClass::FIXED_JOINT: {
            auto asmtJoint = MbD::CREATE<MbD::ASMTFixedJoint>::With();
            setCommon(asmtJoint);
            asmtAssembly->addJoint(asmtJoint);
            break;
        }
        case JointClass::REVOLUTE_JOINT: {
            auto asmtJoint = MbD::CREATE<MbD::ASMTRevoluteJoint>::With();
            setCommon(asmtJoint);
            asmtAssembly->addJoint(asmtJoint);
            break;
        }
        case JointClass::CYLINDRICAL_JOINT: {
            auto asmtJoint = MbD::CREATE<MbD::ASMTCylindricalJoint>::With();
            setCommon(asmtJoint);
            asmtAssembly->addJoint(asmtJoint);
            break;
        }
        case JointClass::TRANSLATIONAL_JOINT: {
            auto asmtJoint = MbD::CREATE<MbD::ASMTTranslationalJoint>::With();
            setCommon(asmtJoint);
            asmtAssembly->addJoint(asmtJoint);
            break;
        }
        case JointClass::SPHERICAL_JOINT: {
            auto asmtJoint = MbD::CREATE<MbD::ASMTSphericalJoint>::With();
            setCommon(asmtJoint);
            asmtAssembly->addJoint(asmtJoint);
            break;
        }
        case JointClass::PARALLEL_AXES_JOINT: {
            auto asmtJoint = MbD::CREATE<MbD::ASMTParallelAxesJoint>::With();
            setCommon(asmtJoint);
            asmtAssembly->addJoint(asmtJoint);
            break;
        }
        case JointClass::PERPENDICULAR_JOINT: {
            auto asmtJoint = MbD::CREATE<MbD::ASMTPerpendicularJoint>::With();
            setCommon(asmtJoint);
            asmtAssembly->addJoint(asmtJoint);
            break;
        }
        case JointClass::ANGLE_JOINT: {
            auto solverJoint = std::static_pointer_cast<AngleJoint>(joint);
            auto asmtJoint = MbD::CREATE<MbD::ASMTAngleJoint>::With();
            setCommon(asmtJoint);
            asmtJoint->theIzJz = solverJoint->getAngle();
            asmtAssembly->addJoint(asmtJoint);
            break;
        }
        case JointClass::RACK_PINION_JOINT: {
            auto solverJoint = std::static_pointer_cast<RackPinionJoint>(joint);
            auto asmtJoint = MbD::CREATE<MbD::ASMTRackPinionJoint>::With();
            setCommon(asmtJoint);
            asmtJoint->pitchRadius = solverJoint->getPitchRadius();
            asmtAssembly->addJoint(asmtJoint);
            break;
        }
        case JointClass::SCREW_JOINT: {
            auto solverJoint = std::static_pointer_cast<ScrewJoint>(joint);
            auto asmtJoint = MbD::CREATE<MbD::ASMTScrewJoint>::With();
            setCommon(asmtJoint);
            asmtJoint->pitch = solverJoint->getPitch();
            asmtAssembly->addJoint(asmtJoint);
            break;
        }
        case JointClass::GEAR_JOINT: {
            auto solverJoint = std::static_pointer_cast<GearJoint>(joint);
            auto asmtJoint = MbD::CREATE<MbD::ASMTGearJoint>::With();
            setCommon(asmtJoint);
            asmtJoint->radiusI = solverJoint->getRadiusI();
            asmtJoint->radiusJ = solverJoint->getRadiusJ();
            asmtAssembly->addJoint(asmtJoint);
            break;
        }
        case JointClass::BELT_JOINT: {
            auto solverJoint = std::static_pointer_cast<BeltJoint>(joint);
            // Belt joints use ASMTGearJoint with negated J radius
            auto asmtJoint = MbD::CREATE<MbD::ASMTGearJoint>::With();
            setCommon(asmtJoint);
            asmtJoint->radiusI = solverJoint->getRadiusI();
            asmtJoint->radiusJ = solverJoint->getRadiusJ();  // caller negates J for belts
            asmtAssembly->addJoint(asmtJoint);
            break;
        }
        case JointClass::SPH_SPH_JOINT: {
            auto solverJoint = std::static_pointer_cast<SphSphJoint>(joint);
            auto asmtJoint = MbD::CREATE<MbD::ASMTSphSphJoint>::With();
            setCommon(asmtJoint);
            asmtJoint->distanceIJ = solverJoint->getDistance();
            asmtAssembly->addJoint(asmtJoint);
            break;
        }
        case JointClass::REV_CYL_JOINT: {
            auto solverJoint = std::static_pointer_cast<RevCylJoint>(joint);
            auto asmtJoint = MbD::CREATE<MbD::ASMTRevCylJoint>::With();
            setCommon(asmtJoint);
            asmtJoint->distanceIJ = solverJoint->getDistance();
            asmtAssembly->addJoint(asmtJoint);
            break;
        }
        case JointClass::CYL_SPH_JOINT: {
            auto solverJoint = std::static_pointer_cast<CylSphJoint>(joint);
            auto asmtJoint = MbD::CREATE<MbD::ASMTCylSphJoint>::With();
            setCommon(asmtJoint);
            asmtJoint->distanceIJ = solverJoint->getDistance();
            asmtAssembly->addJoint(asmtJoint);
            break;
        }
        case JointClass::PLANAR_JOINT: {
            auto solverJoint = std::static_pointer_cast<PlanarJoint>(joint);
            auto asmtJoint = MbD::CREATE<MbD::ASMTPlanarJoint>::With();
            setCommon(asmtJoint);
            asmtJoint->offset = solverJoint->getOffset();
            asmtAssembly->addJoint(asmtJoint);
            break;
        }
        case JointClass::POINT_IN_PLANE_JOINT: {
            auto solverJoint = std::static_pointer_cast<PointInPlaneJoint>(joint);
            auto asmtJoint = MbD::CREATE<MbD::ASMTPointInPlaneJoint>::With();
            setCommon(asmtJoint);
            asmtJoint->offset = solverJoint->getOffset();
            asmtAssembly->addJoint(asmtJoint);
            break;
        }
        case JointClass::POINT_IN_LINE_JOINT: {
            auto asmtJoint = MbD::CREATE<MbD::ASMTPointInLineJoint>::With();
            setCommon(asmtJoint);
            asmtAssembly->addJoint(asmtJoint);
            break;
        }
        case JointClass::LINE_IN_PLANE_JOINT: {
            auto solverJoint = std::static_pointer_cast<LineInPlaneJoint>(joint);
            auto asmtJoint = MbD::CREATE<MbD::ASMTLineInPlaneJoint>::With();
            setCommon(asmtJoint);
            asmtJoint->offset = solverJoint->getOffset();
            asmtAssembly->addJoint(asmtJoint);
            break;
        }
        case JointClass::JOINT:
        default:
            FC_WARN("OndselAssembly: Unknown joint type " << static_cast<int>(joint->getJointClass()));
    }
}

void OndselAssembly::addLimit(std::shared_ptr<Limit> limit)
{
    const std::string& name = limit->getName();
    const MarkerRef& mi = limit->getMarkerI();
    const MarkerRef& mj = limit->getMarkerJ();

    auto applyTypeAndValues = [&](auto& asmtLimit) {
        asmtLimit->setName(name);
        asmtLimit->setMarkerI(mi);
        asmtLimit->setMarkerJ(mj);
        switch (limit->getType()) {
            case LimitType::LESS_THAN_OR_EQUAL:
                asmtLimit->settype("=<");
                break;
            case LimitType::GREATER_THAN_OR_EQUAL:
                asmtLimit->settype("=>");
                break;
            default:
                FC_WARN("OndselAssembly: Unknown limit type");
        }
        asmtLimit->setlimit(limit->getLimitExpression());
        asmtLimit->settol(limit->getToleranceExpression());
    };

    switch (limit->getLimitClass()) {
        case LimitClass::ROTATION_LIMIT: {
            auto asmtLimit = MbD::ASMTRotationLimit::With();
            applyTypeAndValues(asmtLimit);
            asmtAssembly->addLimit(asmtLimit);
            break;
        }
        case LimitClass::TRANSLATION_LIMIT: {
            auto asmtLimit = MbD::ASMTTranslationLimit::With();
            applyTypeAndValues(asmtLimit);
            asmtAssembly->addLimit(asmtLimit);
            break;
        }
        default:
            FC_WARN("OndselAssembly: Unknown limit class");
    }
}

void OndselAssembly::addMotion(std::shared_ptr<Motion> motion)
{
    const std::string& name = motion->getName();
    const MarkerRef& mi = motion->getMarkerI();
    const MarkerRef& mj = motion->getMarkerJ();

    switch (motion->getMotionClass()) {
        case MotionClass::ROTATIONAL_MOTION: {
            auto solverMotion = std::static_pointer_cast<RotationalMotion>(motion);
            auto asmtMotion = MbD::CREATE<MbD::ASMTRotationalMotion>::With();
            asmtMotion->setName(name);
            asmtMotion->setMarkerI(mi);
            asmtMotion->setMarkerJ(mj);
            asmtMotion->setRotationZ(solverMotion->getRotationFormula());
            asmtAssembly->addMotion(asmtMotion);
            break;
        }
        case MotionClass::TRANSLATIONAL_MOTION: {
            auto solverMotion = std::static_pointer_cast<TranslationalMotion>(motion);
            auto asmtMotion = MbD::CREATE<MbD::ASMTTranslationalMotion>::With();
            asmtMotion->setName(name);
            asmtMotion->setMarkerI(mi);
            asmtMotion->setMarkerJ(mj);
            asmtMotion->setTranslationZ(solverMotion->getTranslationFormula());
            asmtAssembly->addMotion(asmtMotion);
            break;
        }
        case MotionClass::GENERAL_MOTION: {
            auto solverMotion = std::static_pointer_cast<GeneralMotion>(motion);
            auto asmtMotion = MbD::CREATE<MbD::ASMTGeneralMotion>::With();
            asmtMotion->setName(name);
            asmtMotion->setMarkerI(mi);
            asmtMotion->setMarkerJ(mj);
            asmtMotion->rIJI->atiput(2, solverMotion->getLinearFormula());
            asmtMotion->angIJJ->atiput(2, solverMotion->getAngularFormula());
            asmtAssembly->addMotion(asmtMotion);
            break;
        }
        default:
            FC_WARN("OndselAssembly: Unknown motion type");
    }
}

int OndselAssembly::solveStatic()
{
    asmtAssembly->runPreDrag();
    return 0;
}

int OndselAssembly::runKinematic()
{
    if (simulationParameters) {
        applySimulationParametersToMbD();
    }
    asmtAssembly->runKINEMATIC();
    return 0;
}

void OndselAssembly::preDrag()
{
    asmtAssembly->runPreDrag();
}

void OndselAssembly::dragStep(std::vector<std::shared_ptr<Part>> parts)
{
    auto dragMbdParts = std::make_shared<std::vector<std::shared_ptr<MbD::ASMTPart>>>();
    for (const auto& part : parts) {
        auto ondselPart = std::static_pointer_cast<OndselPart>(part);
        dragMbdParts->push_back(ondselPart->getAsmtPart());
    }
    asmtAssembly->runDragStep(dragMbdParts);
}

void OndselAssembly::postDrag()
{
    asmtAssembly->runPostDrag();
}

void OndselAssembly::setSimulationParameters(std::shared_ptr<SimulationParameters> params)
{
    simulationParameters = params;
}

std::shared_ptr<SimulationParameters> OndselAssembly::getSimulationParameters() const
{
    return simulationParameters;
}

size_t OndselAssembly::numberOfFrames() const
{
    return asmtAssembly->numberOfFrames();
}

void OndselAssembly::updateForFrame(size_t index)
{
    asmtAssembly->updateForFrame(index);
}

bool OndselAssembly::hasSolvedSystem() const
{
    return asmtAssembly && asmtAssembly->mbdSystem != nullptr;
}

SolveStatus OndselAssembly::querySolveStatus()
{
    SolveStatus status;

    if (!hasSolvedSystem()) {
        return status;
    }

    // Helper: extract the FreeCAD joint name from an ASMT solver path.
    // MbD names look like "/OndselAssembly/ground_moves#Joint001"; we want "Joint001".
    auto cleanJointName = [](const std::string& rawName) -> std::string {
        size_t hashPos = rawName.find_last_of('#');
        if (hashPos != std::string::npos) {
            return rawName.substr(hashPos + 1);
        }
        return rawName;
    };

    asmtAssembly->mbdSystem->jointsMotionsDo([&](std::shared_ptr<MbD::Joint> jm) {
        if (!jm) {
            return;
        }

        SolveStatus::JointInfo info;
        info.name = cleanJointName(jm->name);
        info.isRedundant = false;

        jm->constraintsDo([&](std::shared_ptr<MbD::Constraint> con) {
            if (!con) {
                return;
            }
            ++status.constraintsApplied;
            if (con->constraintSpec().rfind("Redundant", 0) == 0) {
                info.isRedundant = true;
            }
        });

        status.joints.push_back(info);
    });

    return status;
}

void OndselAssembly::exportFile(std::string filename)
{
    asmtAssembly->outputFile(filename);
}

void OndselAssembly::setDebug(bool debug)
{
    asmtAssembly->setDebug(debug);
}

void OndselAssembly::applySimulationParametersToMbD()
{
    if (!simulationParameters) {
        return;
    }
    auto mbdSim = asmtAssembly->simulationParameters;
    mbdSim->settstart(simulationParameters->getTimeStart());
    mbdSim->settend(simulationParameters->getTimeEnd());
    mbdSim->sethout(simulationParameters->getTimeStepOutput());
    mbdSim->sethmin(simulationParameters->getHmin());
    mbdSim->sethmax(simulationParameters->getHmax());
    mbdSim->seterrorTol(simulationParameters->getErrorTolerance());
}

// ============================================================================
// OndselSolver
// ============================================================================

OndselSolver::OndselSolver(AssemblyObject* asmObj)
    : assemblyObject(asmObj)
{}

std::shared_ptr<Solver::Assembly> OndselSolver::makeAssembly()
{
    auto assembly = std::make_shared<OndselAssembly>(assemblyObject);
    assembly->setName("OndselAssembly");
    return assembly;
}

std::shared_ptr<Part> OndselSolver::makePart()
{
    return std::make_shared<OndselPart>("OndselAssembly");
}
