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

#include <boost/signals2.hpp>

#include <Mod/Assembly/AssemblyGlobal.h>

#include <App/FeaturePython.h>
#include <App/Part.h>
#include <App/PropertyLinks.h>

#include <OndselSolver/enum.h>

namespace MbD
{
class ASMTPart;
class ASMTAssembly;
class ASMTJoint;
class ASMTMarker;
class ASMTPart;
}  // namespace MbD

namespace App
{
class PropertyXLinkSub;
}  // namespace App

namespace Base
{
class Placement;
class Rotation;
}  // namespace Base


namespace Assembly
{

class AssemblyLink;
class JointGroup;
class ViewGroup;
enum class JointType;


struct ObjRef
{
    App::DocumentObject* obj;
    App::PropertyXLinkSub* ref;
};

class AssemblyExport AssemblyObject: public App::Part
{
    PROPERTY_HEADER_WITH_OVERRIDE(Assembly::AssemblyObject);

public:
    AssemblyObject();
    ~AssemblyObject() override;

    PyObject* getPyObject() override;

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override
    {
        return "AssemblyGui::ViewProviderAssembly";
    }

    App::DocumentObjectExecReturn* execute() override;
    void onChanged(const App::Property* prop) override;
    /* Solve the assembly. It will update first the joints, solve, update placements of the parts
    and redraw the joints Args : enableRedo : This store initial positions to enable undo while
    being in an active transaction (joint creation).*/
    int solve(bool enableRedo = false, bool updateJCS = true);
    int generateSimulation(App::DocumentObject* sim);
    int updateForFrame(size_t index, bool updateJCS = true);
    size_t numberOfFrames();
    void preDrag(std::vector<App::DocumentObject*> dragParts);
    void doDragStep();
    void postDrag();
    void savePlacementsForUndo();
    void undoSolve();
    void clearUndo();

    void exportAsASMT(std::string fileName);

    Base::Placement getMbdPlacement(std::shared_ptr<MbD::ASMTPart> mbdPart);
    bool validateNewPlacements();
    void setNewPlacements();
    static void redrawJointPlacements(std::vector<App::DocumentObject*> joints);
    static void redrawJointPlacement(App::DocumentObject* joint);

    // This makes sure that LinkGroups or sub-assemblies have identity placements.
    void ensureIdentityPlacements();

    // Ondsel Solver interface
    std::shared_ptr<MbD::ASMTAssembly> makeMbdAssembly();
    void create_mbdSimulationParameters(App::DocumentObject* sim);
    std::shared_ptr<MbD::ASMTPart> makeMbdPart(
        std::string& name,
        Base::Placement plc = Base::Placement(),
        double mass = 1.0
    );
    std::shared_ptr<MbD::ASMTPart> getMbDPart(App::DocumentObject* obj);
    // To help the solver, during dragging, we are bundling parts connected by a fixed joint.
    // So several assembly components are bundled in a single ASMTPart.
    // So we need to store the plc of each bundled object relative to the bundle origin (first obj
    // of objectPartMap).
    struct MbDPartData
    {
        std::shared_ptr<MbD::ASMTPart> part;
        Base::Placement offsetPlc;  // This is the offset within the bundled parts
    };
    MbDPartData getMbDData(App::DocumentObject* part);
    std::shared_ptr<MbD::ASMTMarker> makeMbdMarker(std::string& name, Base::Placement& plc);
    std::vector<std::shared_ptr<MbD::ASMTJoint>> makeMbdJoint(App::DocumentObject* joint);
    std::shared_ptr<MbD::ASMTJoint> makeMbdJointOfType(App::DocumentObject* joint, JointType jointType);
    std::shared_ptr<MbD::ASMTJoint> makeMbdJointDistance(App::DocumentObject* joint);
    std::string handleOneSideOfJoint(
        App::DocumentObject* joint,
        const char* propRefName,
        const char* propPlcName
    );
    void getRackPinionMarkers(
        App::DocumentObject* joint,
        std::string& markerNameI,
        std::string& markerNameJ
    );
    int slidingPartIndex(App::DocumentObject* joint);

    void jointParts(std::vector<App::DocumentObject*> joints);
    JointGroup* getJointGroup() const;
    ViewGroup* getExplodedViewGroup() const;
    template<typename T>
    T* getGroup();

    std::vector<App::DocumentObject*> getJoints(
        bool updateJCS = true,
        bool delBadJoints = false,
        bool subJoints = true
    );
    std::vector<App::DocumentObject*> getGroundedJoints();
    std::vector<App::DocumentObject*> getJointsOfObj(App::DocumentObject* obj);
    std::vector<App::DocumentObject*> getJointsOfPart(App::DocumentObject* part);
    App::DocumentObject* getJointOfPartConnectingToGround(
        App::DocumentObject* part,
        std::string& name,
        const std::vector<App::DocumentObject*>& excludeJoints = {}
    );
    std::unordered_set<App::DocumentObject*> getGroundedParts();
    std::unordered_set<App::DocumentObject*> fixGroundedParts();
    void fixGroundedPart(App::DocumentObject* obj, Base::Placement& plc, std::string& jointName);

    bool isJointConnectingPartToGround(App::DocumentObject* joint, const char* partPropName);
    bool isJointTypeConnecting(App::DocumentObject* joint);

    bool isObjInSetOfObjRefs(App::DocumentObject* obj, const std::vector<ObjRef>& pairs);
    void removeUnconnectedJoints(
        std::vector<App::DocumentObject*>& joints,
        std::unordered_set<App::DocumentObject*> groundedObjs
    );
    void traverseAndMarkConnectedParts(
        App::DocumentObject* currentPart,
        std::vector<ObjRef>& connectedParts,
        const std::vector<App::DocumentObject*>& joints
    );
    std::vector<ObjRef> getConnectedParts(
        App::DocumentObject* part,
        const std::vector<App::DocumentObject*>& joints
    );
    bool isPartGrounded(App::DocumentObject* part);
    bool isPartConnected(App::DocumentObject* part);

    std::vector<ObjRef> getDownstreamParts(
        App::DocumentObject* part,
        App::DocumentObject* joint = nullptr
    );
    App::DocumentObject* getUpstreamMovingPart(
        App::DocumentObject* part,
        App::DocumentObject*& joint,
        std::string& name,
        std::vector<App::DocumentObject*> excludeJoints = {}
    );

    double getObjMass(App::DocumentObject* obj);
    void setObjMasses(std::vector<std::pair<App::DocumentObject*, double>> objectMasses);

    std::vector<AssemblyLink*> getSubAssemblies();

    std::vector<App::DocumentObject*> getMotionsFromSimulation(App::DocumentObject* sim);

    bool isMbDJointValid(App::DocumentObject* joint);

    bool isEmpty() const;
    int numberOfComponents() const;

    void updateSolveStatus();
    inline int getLastDoF() const
    {
        return lastDoF;
    }
    inline bool getLastHasConflicts() const
    {
        return lastHasConflict;
    }
    inline bool getLastHasRedundancies() const
    {
        return lastHasRedundancies;
    }
    inline bool getLastHasPartialRedundancies() const
    {
        return lastHasPartialRedundancies;
    }
    inline bool getLastHasMalformedConstraints() const
    {
        return lastHasMalformedConstraints;
    }
    inline int getLastSolverStatus() const
    {
        return lastSolverStatus;
    }
    inline const std::vector<std::string>& getLastConflicting() const
    {
        return lastConflictingJoints;
    }
    inline const std::vector<std::string>& getLastRedundant() const
    {
        return lastRedundantJoints;
    }
    inline const std::vector<std::string>& getLastPartiallyRedundant() const
    {
        return lastPartialRedundantJoints;
    }
    inline const std::vector<std::string>& getLastMalformed() const
    {
        return lastMalformedJoints;
    }
    fastsignals::signal<void()> signalSolverUpdate;

private:
    std::shared_ptr<MbD::ASMTAssembly> mbdAssembly;

    std::unordered_map<App::DocumentObject*, MbDPartData> objectPartMap;
    std::vector<std::pair<App::DocumentObject*, double>> objMasses;
    std::vector<App::DocumentObject*> draggedParts;
    std::vector<App::DocumentObject*> motions;

    std::vector<std::pair<App::DocumentObject*, Base::Placement>> previousPositions;

    bool bundleFixed;

    int lastDoF;
    bool lastHasConflict;
    bool lastHasRedundancies;
    bool lastHasPartialRedundancies;
    bool lastHasMalformedConstraints;
    int lastSolverStatus;

    std::vector<std::string> lastRedundantJoints;
    std::vector<std::string> lastConflictingJoints;
    std::vector<std::string> lastPartialRedundantJoints;
    std::vector<std::string> lastMalformedJoints;
};

}  // namespace Assembly
