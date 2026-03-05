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

#pragma once

#include "Base/Placement.h"
#include "Base/Rotation.h"
#include "Base/Vector3D.h"

#include <memory>
#include <string>
#include <vector>

// Design notes:
//   * The document object is the source of truth for the actual state of the system.
//     Consequently, calls to AssemblyObject::solve() start by clearing the existing assembly
//     and rebuilding it from the document. This is somewhat inefficient but ensures consistency.
//   * The abstract solver interface uses opaque MarkerRef strings to identify markers. For
//     OndselSolver these are full path strings like "/OndselAssembly/partname/markername".
//     Other solver implementations may use a different format.
//   * AssemblyObject works entirely with abstract Solver types; all MbD-specific code lives
//     in OndselSolver.h/cpp.
//   * A bare minimum implementation needs to implement AssemblySolver::makeAssembly() and
//     AssemblySolver::makePart(), plus the Assembly and Part pure virtual interfaces.

namespace Assembly::Solver
{

// ============================================================================
// Opaque marker reference: an identifier for a marker within the solver.
// For OndselSolver this is the full ASMT path string.
// ============================================================================
using MarkerRef = std::string;

// ============================================================================
// Status returned by Assembly::querySolveStatus() after a solve
// ============================================================================
struct SolveStatus
{
    // Total number of constraints applied (used to compute DoF)
    int constraintsApplied = 0;

    struct JointInfo
    {
        // Cleaned joint name suitable for looking up in the FreeCAD document
        std::string name;
        bool isRedundant = false;
    };

    std::vector<JointInfo> joints;
};

// ============================================================================
// Item — base class with a name
// ============================================================================
class Item
{
public:
    virtual ~Item() = default;

    virtual void setName(std::string name);
    virtual std::string getName() const;

private:
    std::string name;
};

// ============================================================================
// ItemIJ — base for joints, limits and motions that reference two markers
// ============================================================================
class ItemIJ: public Item
{
public:
    virtual ~ItemIJ() = default;

    virtual void setMarkerI(MarkerRef marker);
    virtual void setMarkerJ(MarkerRef marker);
    virtual MarkerRef getMarkerI() const;
    virtual MarkerRef getMarkerJ() const;

private:
    MarkerRef markerI;
    MarkerRef markerJ;
};

// ============================================================================
// Joint classes
// ============================================================================

enum class JointClass
{
    JOINT,
    FIXED_JOINT,
    REVOLUTE_JOINT,
    CYLINDRICAL_JOINT,
    TRANSLATIONAL_JOINT,
    SPHERICAL_JOINT,
    PARALLEL_AXES_JOINT,
    PERPENDICULAR_JOINT,
    ANGLE_JOINT,
    RACK_PINION_JOINT,
    SCREW_JOINT,
    GEAR_JOINT,
    BELT_JOINT,
    SPH_SPH_JOINT,
    REV_CYL_JOINT,
    PLANAR_JOINT,
    CYL_SPH_JOINT,
    POINT_IN_PLANE_JOINT,
    POINT_IN_LINE_JOINT,
    LINE_IN_PLANE_JOINT,
};

class Joint: public ItemIJ
{
public:
    virtual ~Joint() = default;

    JointClass getJointClass() const;

protected:
    void setJointClass(JointClass jointClass);

private:
    JointClass jointClass = JointClass::JOINT;
};

// Simple joints (no additional parameters)
class FixedJoint: public Joint
{
public:
    FixedJoint();
    virtual ~FixedJoint() = default;
};

class RevoluteJoint: public Joint
{
public:
    RevoluteJoint();
    virtual ~RevoluteJoint() = default;
};

class CylindricalJoint: public Joint
{
public:
    CylindricalJoint();
    virtual ~CylindricalJoint() = default;
};

class TranslationalJoint: public Joint
{
public:
    TranslationalJoint();
    virtual ~TranslationalJoint() = default;
};

class SphericalJoint: public Joint
{
public:
    SphericalJoint();
    virtual ~SphericalJoint() = default;
};

class ParallelAxesJoint: public Joint
{
public:
    ParallelAxesJoint();
    virtual ~ParallelAxesJoint() = default;
};

class PerpendicularJoint: public Joint
{
public:
    PerpendicularJoint();
    virtual ~PerpendicularJoint() = default;
};

class PointInLineJoint: public Joint
{
public:
    PointInLineJoint();
    virtual ~PointInLineJoint() = default;
};

// Joints with an angle parameter
class AngleJoint: public Joint
{
public:
    AngleJoint();
    virtual ~AngleJoint() = default;

    virtual void setAngle(double angle);
    virtual double getAngle() const;

private:
    double angle = 0.0;
};

// Joints with a scalar distance parameter
class SphSphJoint: public Joint
{
public:
    SphSphJoint();
    virtual ~SphSphJoint() = default;

    virtual void setDistance(double distance);
    virtual double getDistance() const;

private:
    double distance = 0.0;
};

class RevCylJoint: public Joint
{
public:
    RevCylJoint();
    virtual ~RevCylJoint() = default;

    virtual void setDistance(double distance);
    virtual double getDistance() const;

private:
    double distance = 0.0;
};

class CylSphJoint: public Joint
{
public:
    CylSphJoint();
    virtual ~CylSphJoint() = default;

    virtual void setDistance(double distance);
    virtual double getDistance() const;

private:
    double distance = 0.0;
};

// Joints with an offset parameter
class PlanarJoint: public Joint
{
public:
    PlanarJoint();
    virtual ~PlanarJoint() = default;

    virtual void setOffset(double offset);
    virtual double getOffset() const;

private:
    double offset = 0.0;
};

class PointInPlaneJoint: public Joint
{
public:
    PointInPlaneJoint();
    virtual ~PointInPlaneJoint() = default;

    virtual void setOffset(double offset);
    virtual double getOffset() const;

private:
    double offset = 0.0;
};

class LineInPlaneJoint: public Joint
{
public:
    LineInPlaneJoint();
    virtual ~LineInPlaneJoint() = default;

    virtual void setOffset(double offset);
    virtual double getOffset() const;

private:
    double offset = 0.0;
};

// Joints with pitch/radius parameters
class RackPinionJoint: public Joint
{
public:
    RackPinionJoint();
    virtual ~RackPinionJoint() = default;

    virtual void setPitchRadius(double pitchRadius);
    virtual double getPitchRadius() const;

private:
    double pitchRadius = 1.0;
};

class ScrewJoint: public Joint
{
public:
    ScrewJoint();
    virtual ~ScrewJoint() = default;

    virtual void setPitch(double pitch);
    virtual double getPitch() const;

private:
    double pitch = 1.0;
};

class GearJoint: public Joint
{
public:
    GearJoint();
    virtual ~GearJoint() = default;

    virtual void setRadiusI(double radius);
    virtual void setRadiusJ(double radius);
    virtual double getRadiusI() const;
    virtual double getRadiusJ() const;

private:
    double radiusI = 1.0;
    double radiusJ = 1.0;
};

class BeltJoint: public Joint
{
public:
    BeltJoint();
    virtual ~BeltJoint() = default;

    virtual void setRadiusI(double radius);
    virtual void setRadiusJ(double radius);
    virtual double getRadiusI() const;
    virtual double getRadiusJ() const;

private:
    double radiusI = 1.0;
    double radiusJ = 1.0;
};

// ============================================================================
// Limit classes
// ============================================================================

enum class LimitClass
{
    LIMIT,
    ROTATION_LIMIT,
    TRANSLATION_LIMIT,
};

enum class LimitType
{
    NO_LIMIT,
    LESS_THAN_OR_EQUAL,
    GREATER_THAN_OR_EQUAL,
};

class Limit: public ItemIJ
{
public:
    virtual ~Limit() = default;

    // Set the limit as a formula expression (e.g., "30*pi/180.0" for rotation limits)
    virtual void setLimitExpression(std::string expr);
    virtual std::string getLimitExpression() const;

    // Convenience: set the limit as a plain double value
    virtual void setLimitValue(double value);

    // Set the tolerance as an expression string (e.g., "1.0e-9")
    virtual void setToleranceExpression(std::string tol);
    virtual std::string getToleranceExpression() const;

    virtual void setType(LimitType type);
    virtual LimitType getType() const;

    virtual LimitClass getLimitClass() const;

protected:
    virtual void setLimitClass(LimitClass limitClass);

private:
    LimitClass limitClass = LimitClass::LIMIT;
    LimitType type = LimitType::NO_LIMIT;
    std::string limitExpr = "0.0";
    std::string toleranceExpr = "1.0e-9";
};

class RotationLimit: public Limit
{
public:
    RotationLimit();
    virtual ~RotationLimit() = default;
};

class TranslationLimit: public Limit
{
public:
    TranslationLimit();
    virtual ~TranslationLimit() = default;
};

// ============================================================================
// Motion classes
// ============================================================================

enum class MotionClass
{
    MOTION,
    ROTATIONAL_MOTION,
    TRANSLATIONAL_MOTION,
    GENERAL_MOTION,
};

class Motion: public ItemIJ
{
public:
    virtual ~Motion() = default;

    MotionClass getMotionClass() const;

protected:
    void setMotionClass(MotionClass motionClass);

private:
    MotionClass motionClass = MotionClass::MOTION;
};

// Rotational motion: formula drives the Z-axis rotation
class RotationalMotion: public Motion
{
public:
    RotationalMotion();
    virtual ~RotationalMotion() = default;

    virtual void setRotationFormula(std::string formula);
    virtual std::string getRotationFormula() const;

private:
    std::string rotationFormula;
};

// Translational motion: formula drives the Z-axis translation
class TranslationalMotion: public Motion
{
public:
    TranslationalMotion();
    virtual ~TranslationalMotion() = default;

    virtual void setTranslationFormula(std::string formula);
    virtual std::string getTranslationFormula() const;

private:
    std::string translationFormula;
};

// General motion: separate formulas for linear (Z) and angular (Z) components.
// Used for joints that have both rotation and translation (e.g., cylindrical with two motions).
class GeneralMotion: public Motion
{
public:
    GeneralMotion();
    virtual ~GeneralMotion() = default;

    virtual void setLinearFormula(std::string formula);
    virtual std::string getLinearFormula() const;

    virtual void setAngularFormula(std::string formula);
    virtual std::string getAngularFormula() const;

private:
    std::string linearFormula;
    std::string angularFormula;
};

// ============================================================================
// SimulationParameters
// ============================================================================

class SimulationParameters: public Item
{
public:
    virtual ~SimulationParameters() = default;

    virtual void setTimeStart(double timeStart);
    virtual double getTimeStart() const;

    virtual void setTimeEnd(double timeEnd);
    virtual double getTimeEnd() const;

    virtual void setTimeStepOutput(double timeStepOutput);
    virtual double getTimeStepOutput() const;

    virtual void setHmin(double hmin);
    virtual double getHmin() const;

    virtual void setHmax(double hmax);
    virtual double getHmax() const;

    virtual void setErrorTolerance(double tol);
    virtual double getErrorTolerance() const;

private:
    double timeStart = 0.0;
    double timeEnd = 1.0;
    double timeStepOutput = 1.0 / 30.0;
    double hmin = 1.0e-9;
    double hmax = 1.0;
    double errorTolerance = 1.0e-6;
};

// ============================================================================
// Part — represents a rigid body to be solved
// ============================================================================

class Part: public Item
{
public:
    virtual ~Part() = default;

    // Set the initial placement (position/orientation) of this part.
    // Called once when the part is created from the document.
    virtual void setPlacement(Base::Placement plc);
    virtual Base::Placement getPlacement() const;

    // Push the current (dragged) placement into the solver during interactive dragging.
    virtual void pushPlacement(Base::Placement plc) = 0;

    // Add a named marker at the given local placement.
    // Returns the opaque MarkerRef that can be passed to joint/limit/motion constructors.
    virtual MarkerRef addMarker(std::string name, Base::Placement plc) = 0;

    // Mass properties
    virtual void setMass(double mass);
    virtual double getMass() const;

    virtual void setDensity(double density);
    virtual double getDensity() const;

    virtual void setMomentOfInertias(double ixx, double iyy, double izz);
    virtual void getMomentOfInertias(double& ixx, double& iyy, double& izz) const;

private:
    Base::Placement placement;
    double mass = 1.0;
    double density = 1.0;
    double ixx = 1.0;
    double iyy = 1.0;
    double izz = 1.0;
};

// ============================================================================
// Assembly — the root solver object
// ============================================================================

class Assembly: public Item
{
public:
    virtual ~Assembly() = default;

    // Add a rigid body part to this assembly
    virtual void addPart(std::shared_ptr<Part> part) = 0;

    // Add a marker attached to the assembly ground (world frame).
    // Returns the opaque MarkerRef for use in joints.
    virtual MarkerRef addGroundMarker(std::string name, Base::Placement plc) = 0;

    // Add constraints
    virtual void addJoint(std::shared_ptr<Joint> joint) = 0;
    virtual void addLimit(std::shared_ptr<Limit> limit) = 0;
    virtual void addMotion(std::shared_ptr<Motion> motion) = 0;

    // Static solve: positions all parts to satisfy constraints.
    // Internally uses the solver's pre-drag (static equilibrium) method.
    // Returns 0 on success, non-zero on failure.
    virtual int solveStatic() = 0;

    // Kinematic simulation: run a full time-stepped kinematic simulation.
    // Requires simulation parameters to have been set.
    // Returns 0 on success, non-zero on failure.
    virtual int runKinematic() = 0;

    // Interactive drag support
    virtual void preDrag() = 0;
    virtual void dragStep(std::vector<std::shared_ptr<Part>> parts) = 0;
    virtual void postDrag() = 0;

    // Simulation parameter management
    virtual void setSimulationParameters(std::shared_ptr<SimulationParameters> params) = 0;
    virtual std::shared_ptr<SimulationParameters> getSimulationParameters() const = 0;
    virtual size_t numberOfFrames() const = 0;
    virtual void updateForFrame(size_t index) = 0;

    // Post-solve status queries
    virtual bool hasSolvedSystem() const = 0;
    virtual SolveStatus querySolveStatus() = 0;

    // Export the assembly to a solver-native file format
    virtual void exportFile(std::string filename) = 0;

    // Enable/disable solver debug logging
    virtual void setDebug(bool debug) = 0;
};

// ============================================================================
// AssemblySolver — factory for creating solver-specific objects
// ============================================================================

class AssemblySolver
{
public:
    virtual ~AssemblySolver() = default;

    // Create the assembly object (must be called before any add* calls)
    virtual std::shared_ptr<Assembly> makeAssembly() = 0;

    // Create a part
    virtual std::shared_ptr<Part> makePart() = 0;

    // Joint factories — default implementations return base Joint subclasses.
    // Override in the solver implementation to return solver-specific subtypes.
    virtual std::shared_ptr<FixedJoint> makeFixedJoint();
    virtual std::shared_ptr<RevoluteJoint> makeRevoluteJoint();
    virtual std::shared_ptr<CylindricalJoint> makeCylindricalJoint();
    virtual std::shared_ptr<TranslationalJoint> makeTranslationalJoint();
    virtual std::shared_ptr<SphericalJoint> makeSphericalJoint();
    virtual std::shared_ptr<ParallelAxesJoint> makeParallelAxesJoint();
    virtual std::shared_ptr<PerpendicularJoint> makePerpendicularJoint();
    virtual std::shared_ptr<AngleJoint> makeAngleJoint();
    virtual std::shared_ptr<RackPinionJoint> makeRackPinionJoint();
    virtual std::shared_ptr<ScrewJoint> makeScrewJoint();
    virtual std::shared_ptr<GearJoint> makeGearJoint();
    virtual std::shared_ptr<BeltJoint> makeBeltJoint();
    virtual std::shared_ptr<SphSphJoint> makeSphSphJoint();
    virtual std::shared_ptr<RevCylJoint> makeRevCylJoint();
    virtual std::shared_ptr<CylSphJoint> makeCylSphJoint();
    virtual std::shared_ptr<PlanarJoint> makePlanarJoint();
    virtual std::shared_ptr<PointInPlaneJoint> makePointInPlaneJoint();
    virtual std::shared_ptr<PointInLineJoint> makePointInLineJoint();
    virtual std::shared_ptr<LineInPlaneJoint> makeLineInPlaneJoint();

    // Limit factories
    virtual std::shared_ptr<RotationLimit> makeRotationLimit();
    virtual std::shared_ptr<TranslationLimit> makeTranslationLimit();

    // Motion factories
    virtual std::shared_ptr<RotationalMotion> makeRotationalMotion();
    virtual std::shared_ptr<TranslationalMotion> makeTranslationalMotion();
    virtual std::shared_ptr<GeneralMotion> makeGeneralMotion();
};

}  // namespace Assembly::Solver
