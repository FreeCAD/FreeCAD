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

#include "Solver.h"

#include <utility>

using namespace Assembly::Solver;

// ============================================================================
// Item
// ============================================================================

void Item::setName(std::string name)
{
    this->name = std::move(name);
}

std::string Item::getName() const
{
    return name;
}

// ============================================================================
// ItemIJ
// ============================================================================

void ItemIJ::setMarkerI(MarkerRef marker)
{
    this->markerI = std::move(marker);
}

void ItemIJ::setMarkerJ(MarkerRef marker)
{
    this->markerJ = std::move(marker);
}

MarkerRef ItemIJ::getMarkerI() const
{
    return markerI;
}

MarkerRef ItemIJ::getMarkerJ() const
{
    return markerJ;
}

// ============================================================================
// Joint
// ============================================================================

JointClass Joint::getJointClass() const
{
    return jointClass;
}

void Joint::setJointClass(JointClass jc)
{
    this->jointClass = jc;
}

FixedJoint::FixedJoint()
{
    setJointClass(JointClass::FIXED_JOINT);
}

RevoluteJoint::RevoluteJoint()
{
    setJointClass(JointClass::REVOLUTE_JOINT);
}

CylindricalJoint::CylindricalJoint()
{
    setJointClass(JointClass::CYLINDRICAL_JOINT);
}

TranslationalJoint::TranslationalJoint()
{
    setJointClass(JointClass::TRANSLATIONAL_JOINT);
}

SphericalJoint::SphericalJoint()
{
    setJointClass(JointClass::SPHERICAL_JOINT);
}

ParallelAxesJoint::ParallelAxesJoint()
{
    setJointClass(JointClass::PARALLEL_AXES_JOINT);
}

PerpendicularJoint::PerpendicularJoint()
{
    setJointClass(JointClass::PERPENDICULAR_JOINT);
}

PointInLineJoint::PointInLineJoint()
{
    setJointClass(JointClass::POINT_IN_LINE_JOINT);
}

AngleJoint::AngleJoint()
{
    setJointClass(JointClass::ANGLE_JOINT);
}

void AngleJoint::setAngle(double a)
{
    angle = a;
}

double AngleJoint::getAngle() const
{
    return angle;
}

SphSphJoint::SphSphJoint()
{
    setJointClass(JointClass::SPH_SPH_JOINT);
}

void SphSphJoint::setDistance(double d)
{
    distance = d;
}

double SphSphJoint::getDistance() const
{
    return distance;
}

RevCylJoint::RevCylJoint()
{
    setJointClass(JointClass::REV_CYL_JOINT);
}

void RevCylJoint::setDistance(double d)
{
    distance = d;
}

double RevCylJoint::getDistance() const
{
    return distance;
}

CylSphJoint::CylSphJoint()
{
    setJointClass(JointClass::CYL_SPH_JOINT);
}

void CylSphJoint::setDistance(double d)
{
    distance = d;
}

double CylSphJoint::getDistance() const
{
    return distance;
}

PlanarJoint::PlanarJoint()
{
    setJointClass(JointClass::PLANAR_JOINT);
}

void PlanarJoint::setOffset(double o)
{
    offset = o;
}

double PlanarJoint::getOffset() const
{
    return offset;
}

PointInPlaneJoint::PointInPlaneJoint()
{
    setJointClass(JointClass::POINT_IN_PLANE_JOINT);
}

void PointInPlaneJoint::setOffset(double o)
{
    offset = o;
}

double PointInPlaneJoint::getOffset() const
{
    return offset;
}

LineInPlaneJoint::LineInPlaneJoint()
{
    setJointClass(JointClass::LINE_IN_PLANE_JOINT);
}

void LineInPlaneJoint::setOffset(double o)
{
    offset = o;
}

double LineInPlaneJoint::getOffset() const
{
    return offset;
}

RackPinionJoint::RackPinionJoint()
{
    setJointClass(JointClass::RACK_PINION_JOINT);
}

void RackPinionJoint::setPitchRadius(double r)
{
    pitchRadius = r;
}

double RackPinionJoint::getPitchRadius() const
{
    return pitchRadius;
}

ScrewJoint::ScrewJoint()
{
    setJointClass(JointClass::SCREW_JOINT);
}

void ScrewJoint::setPitch(double p)
{
    pitch = p;
}

double ScrewJoint::getPitch() const
{
    return pitch;
}

GearJoint::GearJoint()
{
    setJointClass(JointClass::GEAR_JOINT);
}

void GearJoint::setRadiusI(double r)
{
    radiusI = r;
}

void GearJoint::setRadiusJ(double r)
{
    radiusJ = r;
}

double GearJoint::getRadiusI() const
{
    return radiusI;
}

double GearJoint::getRadiusJ() const
{
    return radiusJ;
}

BeltJoint::BeltJoint()
{
    setJointClass(JointClass::BELT_JOINT);
}

void BeltJoint::setRadiusI(double r)
{
    radiusI = r;
}

void BeltJoint::setRadiusJ(double r)
{
    radiusJ = r;
}

double BeltJoint::getRadiusI() const
{
    return radiusI;
}

double BeltJoint::getRadiusJ() const
{
    return radiusJ;
}

// ============================================================================
// Limit
// ============================================================================

void Limit::setLimitExpression(std::string expr)
{
    limitExpr = std::move(expr);
}

std::string Limit::getLimitExpression() const
{
    return limitExpr;
}

void Limit::setLimitValue(double value)
{
    limitExpr = std::to_string(value);
}

void Limit::setToleranceExpression(std::string tol)
{
    toleranceExpr = std::move(tol);
}

std::string Limit::getToleranceExpression() const
{
    return toleranceExpr;
}

void Limit::setType(LimitType t)
{
    type = t;
}

LimitType Limit::getType() const
{
    return type;
}

LimitClass Limit::getLimitClass() const
{
    return limitClass;
}

void Limit::setLimitClass(LimitClass lc)
{
    limitClass = lc;
}

RotationLimit::RotationLimit()
{
    setLimitClass(LimitClass::ROTATION_LIMIT);
}

TranslationLimit::TranslationLimit()
{
    setLimitClass(LimitClass::TRANSLATION_LIMIT);
}

// ============================================================================
// Motion
// ============================================================================

MotionClass Motion::getMotionClass() const
{
    return motionClass;
}

void Motion::setMotionClass(MotionClass mc)
{
    motionClass = mc;
}

RotationalMotion::RotationalMotion()
{
    setMotionClass(MotionClass::ROTATIONAL_MOTION);
}

void RotationalMotion::setRotationFormula(std::string formula)
{
    rotationFormula = std::move(formula);
}

std::string RotationalMotion::getRotationFormula() const
{
    return rotationFormula;
}

TranslationalMotion::TranslationalMotion()
{
    setMotionClass(MotionClass::TRANSLATIONAL_MOTION);
}

void TranslationalMotion::setTranslationFormula(std::string formula)
{
    translationFormula = std::move(formula);
}

std::string TranslationalMotion::getTranslationFormula() const
{
    return translationFormula;
}

GeneralMotion::GeneralMotion()
{
    setMotionClass(MotionClass::GENERAL_MOTION);
}

void GeneralMotion::setLinearFormula(std::string formula)
{
    linearFormula = std::move(formula);
}

std::string GeneralMotion::getLinearFormula() const
{
    return linearFormula;
}

void GeneralMotion::setAngularFormula(std::string formula)
{
    angularFormula = std::move(formula);
}

std::string GeneralMotion::getAngularFormula() const
{
    return angularFormula;
}

// ============================================================================
// SimulationParameters
// ============================================================================

void SimulationParameters::setTimeStart(double t)
{
    timeStart = t;
}

double SimulationParameters::getTimeStart() const
{
    return timeStart;
}

void SimulationParameters::setTimeEnd(double t)
{
    timeEnd = t;
}

double SimulationParameters::getTimeEnd() const
{
    return timeEnd;
}

void SimulationParameters::setTimeStepOutput(double t)
{
    timeStepOutput = t;
}

double SimulationParameters::getTimeStepOutput() const
{
    return timeStepOutput;
}

void SimulationParameters::setHmin(double h)
{
    hmin = h;
}

double SimulationParameters::getHmin() const
{
    return hmin;
}

void SimulationParameters::setHmax(double h)
{
    hmax = h;
}

double SimulationParameters::getHmax() const
{
    return hmax;
}

void SimulationParameters::setErrorTolerance(double tol)
{
    errorTolerance = tol;
}

double SimulationParameters::getErrorTolerance() const
{
    return errorTolerance;
}

// ============================================================================
// Part
// ============================================================================

void Part::setPlacement(Base::Placement plc)
{
    placement = plc;
}

Base::Placement Part::getPlacement() const
{
    return placement;
}

void Part::setMass(double m)
{
    mass = m;
}

double Part::getMass() const
{
    return mass;
}

void Part::setDensity(double d)
{
    density = d;
}

double Part::getDensity() const
{
    return density;
}

void Part::setMomentOfInertias(double x, double y, double z)
{
    ixx = x;
    iyy = y;
    izz = z;
}

void Part::getMomentOfInertias(double& x, double& y, double& z) const
{
    x = ixx;
    y = iyy;
    z = izz;
}

// ============================================================================
// AssemblySolver — default factory implementations
// ============================================================================

std::shared_ptr<FixedJoint> AssemblySolver::makeFixedJoint()
{
    return std::make_shared<FixedJoint>();
}

std::shared_ptr<RevoluteJoint> AssemblySolver::makeRevoluteJoint()
{
    return std::make_shared<RevoluteJoint>();
}

std::shared_ptr<CylindricalJoint> AssemblySolver::makeCylindricalJoint()
{
    return std::make_shared<CylindricalJoint>();
}

std::shared_ptr<TranslationalJoint> AssemblySolver::makeTranslationalJoint()
{
    return std::make_shared<TranslationalJoint>();
}

std::shared_ptr<SphericalJoint> AssemblySolver::makeSphericalJoint()
{
    return std::make_shared<SphericalJoint>();
}

std::shared_ptr<ParallelAxesJoint> AssemblySolver::makeParallelAxesJoint()
{
    return std::make_shared<ParallelAxesJoint>();
}

std::shared_ptr<PerpendicularJoint> AssemblySolver::makePerpendicularJoint()
{
    return std::make_shared<PerpendicularJoint>();
}

std::shared_ptr<AngleJoint> AssemblySolver::makeAngleJoint()
{
    return std::make_shared<AngleJoint>();
}

std::shared_ptr<RackPinionJoint> AssemblySolver::makeRackPinionJoint()
{
    return std::make_shared<RackPinionJoint>();
}

std::shared_ptr<ScrewJoint> AssemblySolver::makeScrewJoint()
{
    return std::make_shared<ScrewJoint>();
}

std::shared_ptr<GearJoint> AssemblySolver::makeGearJoint()
{
    return std::make_shared<GearJoint>();
}

std::shared_ptr<BeltJoint> AssemblySolver::makeBeltJoint()
{
    return std::make_shared<BeltJoint>();
}

std::shared_ptr<SphSphJoint> AssemblySolver::makeSphSphJoint()
{
    return std::make_shared<SphSphJoint>();
}

std::shared_ptr<RevCylJoint> AssemblySolver::makeRevCylJoint()
{
    return std::make_shared<RevCylJoint>();
}

std::shared_ptr<CylSphJoint> AssemblySolver::makeCylSphJoint()
{
    return std::make_shared<CylSphJoint>();
}

std::shared_ptr<PlanarJoint> AssemblySolver::makePlanarJoint()
{
    return std::make_shared<PlanarJoint>();
}

std::shared_ptr<PointInPlaneJoint> AssemblySolver::makePointInPlaneJoint()
{
    return std::make_shared<PointInPlaneJoint>();
}

std::shared_ptr<PointInLineJoint> AssemblySolver::makePointInLineJoint()
{
    return std::make_shared<PointInLineJoint>();
}

std::shared_ptr<LineInPlaneJoint> AssemblySolver::makeLineInPlaneJoint()
{
    return std::make_shared<LineInPlaneJoint>();
}

std::shared_ptr<RotationLimit> AssemblySolver::makeRotationLimit()
{
    return std::make_shared<RotationLimit>();
}

std::shared_ptr<TranslationLimit> AssemblySolver::makeTranslationLimit()
{
    return std::make_shared<TranslationLimit>();
}

std::shared_ptr<RotationalMotion> AssemblySolver::makeRotationalMotion()
{
    return std::make_shared<RotationalMotion>();
}

std::shared_ptr<TranslationalMotion> AssemblySolver::makeTranslationalMotion()
{
    return std::make_shared<TranslationalMotion>();
}

std::shared_ptr<GeneralMotion> AssemblySolver::makeGeneralMotion()
{
    return std::make_shared<GeneralMotion>();
}
