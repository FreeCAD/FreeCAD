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

#pragma once

#include "Solver.h"

#include <memory>

namespace MbD
{
class ASMTPart;
class ASMTAssembly;
}  // namespace MbD

namespace Assembly
{
class AssemblyObject;
}  // namespace Assembly

namespace Assembly::Solver
{

// ============================================================================
// OndselPart — wraps a single MbD::ASMTPart
// ============================================================================

class OndselPart: public Part
{
public:
    explicit OndselPart(std::string assemblyName);
    virtual ~OndselPart() = default;

    // Set the initial placement of this part (uses ASMT setPosition3D/setRotationMatrix).
    // Must be called before the MbD system is initialized.
    void setPlacement(Base::Placement plc) override;

    // Push current placement into the MbD part for drag interaction.
    // Requires the MbD system to already be initialized (after a solve).
    void pushPlacement(Base::Placement plc) override;

    // Create a named marker on this part at the given local placement.
    // Returns the full ASMT path: "/assemblyName/partName/markerName"
    MarkerRef addMarker(std::string name, Base::Placement plc) override;

    // Get the solved placement from the MbD part after a solve
    Base::Placement getPlacement() const override;

    // Access the underlying MbD part (used by OndselAssembly::addPart/dragStep)
    std::shared_ptr<MbD::ASMTPart> getAsmtPart() const;

private:
    std::shared_ptr<MbD::ASMTPart> asmtPart;
    std::string assemblyName;  // e.g. "OndselAssembly"
};

// ============================================================================
// OndselAssembly — wraps a MbD::ASMTAssembly
// ============================================================================

class OndselAssembly: public Assembly
{
public:
    explicit OndselAssembly(AssemblyObject* assemblyObject);
    virtual ~OndselAssembly() = default;

    void addPart(std::shared_ptr<Part> part) override;

    // Add a marker to the assembly ground; returns the full ASMT path.
    MarkerRef addGroundMarker(std::string name, Base::Placement plc) override;

    void addJoint(std::shared_ptr<Joint> joint) override;
    void addLimit(std::shared_ptr<Limit> limit) override;
    void addMotion(std::shared_ptr<Motion> motion) override;

    // Static solve: positions parts using the pre-drag (static equilibrium) solver
    int solveStatic() override;

    // Kinematic simulation
    int runKinematic() override;

    // Interactive drag
    void preDrag() override;
    void dragStep(std::vector<std::shared_ptr<Part>> parts) override;
    void postDrag() override;

    // Simulation parameters
    void setSimulationParameters(std::shared_ptr<SimulationParameters> params) override;
    std::shared_ptr<SimulationParameters> getSimulationParameters() const override;
    size_t numberOfFrames() const override;
    void updateForFrame(size_t index) override;

    // Post-solve status
    bool hasSolvedSystem() const override;
    SolveStatus querySolveStatus() override;

    void exportFile(std::string filename) override;
    void setDebug(bool debug) override;

private:
    void applySimulationParametersToMbD();

    std::shared_ptr<MbD::ASMTAssembly> asmtAssembly;
    std::shared_ptr<SimulationParameters> simulationParameters;
};

// ============================================================================
// OndselSolver — factory that creates OndselAssembly and OndselPart objects
// ============================================================================

class OndselSolver: public AssemblySolver
{
public:
    explicit OndselSolver(AssemblyObject* assemblyObject);
    virtual ~OndselSolver() = default;

    std::shared_ptr<Assembly> makeAssembly() override;
    std::shared_ptr<Part> makePart() override;

private:
    AssemblyObject* assemblyObject;
};

}  // namespace Assembly::Solver
