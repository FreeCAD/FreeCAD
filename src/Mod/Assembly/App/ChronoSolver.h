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

// ChronoSolver is only compiled when Project Chrono is available.
// Translation units that do not define HAVE_CHRONO will see an empty header.
#ifdef HAVE_CHRONO

# include "Solver.h"

// ChFrame<double> is used as a value type in member variables, so the full
// template definition must be visible here.
# include <chrono/core/ChFrame.h>

# include <map>
# include <memory>
# include <string>
# include <vector>

// Forward-declare Chrono types that are only used via pointer/unique_ptr.
namespace chrono
{
class ChBody;
class ChSystemNSC;
}  // namespace chrono

namespace Assembly
{
class AssemblyObject;
}  // namespace Assembly


namespace Assembly::Solver
{

// ============================================================================
// ChronoPart — wraps a Chrono ChBody rigid body
// ============================================================================

class ChronoPart: public Part
{
public:
    ChronoPart();
    virtual ~ChronoPart() = default;

    // Set the initial placement before any solve.
    void setPlacement(Base::Placement plc) override;

    // Push the dragged placement into the ChBody.
    void pushPlacement(Base::Placement plc) override;

    // Add a named marker at the given local placement.
    // Returns a MarkerRef of the form "partName|markerName".
    MarkerRef addMarker(std::string name, Base::Placement plc) override;

    // Read back the solved placement from the ChBody.
    Base::Placement getPlacement() const override;

    // Access the underlying ChBody.
    std::shared_ptr<chrono::ChBody> getBody() const;

    // Get the local frame of a marker by name.
    // Returns false if the marker is not found.
    bool getMarkerFrame(const std::string& markerName, chrono::ChFrame<double>& outFrame) const;

private:
    std::shared_ptr<chrono::ChBody> body;
    // Map from marker name to body-relative ChFrame
    std::map<std::string, chrono::ChFrame<double>> localMarkers;
};

// ============================================================================
// ChronoAssembly — wraps a Chrono ChSystemNSC and performs kinematic solving
// ============================================================================

class ChronoAssembly: public Assembly
{
public:
    ChronoAssembly();
    virtual ~ChronoAssembly() = default;

    void addPart(std::shared_ptr<Part> part) override;

    // Add a marker to the assembly ground; returns "__ground__|markerName".
    MarkerRef addGroundMarker(std::string name, Base::Placement plc) override;

    void addJoint(std::shared_ptr<Joint> joint) override;
    void addLimit(std::shared_ptr<Limit> limit) override;
    void addMotion(std::shared_ptr<Motion> motion) override;

    // Position-level kinematic solve (satisfies all constraints).
    int solveStatic() override;

    // Time-stepped kinematic simulation.
    int runKinematic() override;

    // Interactive drag support
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
    struct MarkerInfo
    {
        std::shared_ptr<chrono::ChBody> body;
        chrono::ChFrame<double> localFrame;
    };

    // Look up the ChBody and body-local frame for a MarkerRef.
    // Returns false if the ref is not found.
    bool resolveMarker(
        const MarkerRef& ref,
        std::shared_ptr<chrono::ChBody>& outBody,
        chrono::ChFrame<double>& outFrame
    ) const;

    // The Chrono physics system
    std::unique_ptr<chrono::ChSystemNSC> sys;

    // A fixed "ground" body anchored at the world origin
    std::shared_ptr<chrono::ChBody> groundBody;

    // All parts added to this assembly
    std::vector<std::shared_ptr<ChronoPart>> parts;

    // Ground marker local frames (keyed by marker name, not full ref)
    std::map<std::string, chrono::ChFrame<double>> groundMarkers;

    // Map from full MarkerRef string to resolved MarkerInfo (populated lazily in addJoint)
    std::map<MarkerRef, MarkerInfo> markerRegistry;

    // Simulation parameter storage
    std::shared_ptr<SimulationParameters> simulationParameters;

    // Per-frame body state for kinematic simulation playback
    struct BodyFrameSnapshot
    {
        std::shared_ptr<chrono::ChBody> body;
        chrono::ChFrame<double> frame;
    };
    std::vector<std::vector<BodyFrameSnapshot>> kinematicFrames;

    bool solved = false;
    bool debugLogging = false;
};

// ============================================================================
// ChronoSolver — factory that creates ChronoAssembly and ChronoPart objects
// ============================================================================

class ChronoSolver: public AssemblySolver
{
public:
    explicit ChronoSolver(AssemblyObject* assemblyObject);
    virtual ~ChronoSolver() = default;

    std::shared_ptr<Assembly> makeAssembly() override;
    std::shared_ptr<Part> makePart() override;

private:
    AssemblyObject* assemblyObject;
};

}  // namespace Assembly::Solver

#endif  // HAVE_CHRONO
