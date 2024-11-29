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


#ifndef ASSEMBLY_AssemblyObject_H
#define ASSEMBLY_AssemblyObject_H


#include <GeomAbs_CurveType.hxx>
#include <GeomAbs_SurfaceType.hxx>

#include <Mod/Assembly/AssemblyGlobal.h>

#include <App/FeaturePython.h>
#include <App/Part.h>
#include <App/PropertyLinks.h>

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

struct ObjRef
{
    App::DocumentObject* obj;
    App::PropertyXLinkSub* ref;
};

// This enum has to be the same as the one in JointObject.py
enum class JointType
{
    Fixed,
    Revolute,
    Cylindrical,
    Slider,
    Ball,
    Distance,
    Parallel,
    Perpendicular,
    Angle,
    RackPinion,
    Screw,
    Gears,
    Belt,
};

enum class DistanceType
{
    PointPoint,

    LineLine,
    LineCircle,
    CircleCircle,

    PlanePlane,
    PlaneCylinder,
    PlaneSphere,
    PlaneCone,
    PlaneTorus,
    CylinderCylinder,
    CylinderSphere,
    CylinderCone,
    CylinderTorus,
    ConeCone,
    ConeTorus,
    ConeSphere,
    TorusTorus,
    TorusSphere,
    SphereSphere,

    PointPlane,
    PointCylinder,
    PointSphere,
    PointCone,
    PointTorus,

    LinePlane,
    LineCylinder,
    LineSphere,
    LineCone,
    LineTorus,

    CurvePlane,
    CurveCylinder,
    CurveSphere,
    CurveCone,
    CurveTorus,

    PointLine,
    PointCurve,

    Other,
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

    /* Solve the assembly. It will update first the joints, solve, update placements of the parts
    and redraw the joints Args : enableRedo : This store initial positions to enable undo while
    being in an active transaction (joint creation).*/
    int solve(bool enableRedo = false, bool updateJCS = true);
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
    static void recomputeJointPlacements(std::vector<App::DocumentObject*> joints);
    static void redrawJointPlacements(std::vector<App::DocumentObject*> joints);
    static void redrawJointPlacement(App::DocumentObject* joint);

    // This makes sure that LinkGroups or sub-assemblies have identity placements.
    void ensureIdentityPlacements();

    // Ondsel Solver interface
    std::shared_ptr<MbD::ASMTAssembly> makeMbdAssembly();
    std::shared_ptr<MbD::ASMTPart>
    makeMbdPart(std::string& name, Base::Placement plc = Base::Placement(), double mass = 1.0);
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
    MbDPartData getMbDData(App::DocumentObject* obj);
    std::shared_ptr<MbD::ASMTMarker> makeMbdMarker(std::string& name, Base::Placement& plc);
    std::vector<std::shared_ptr<MbD::ASMTJoint>> makeMbdJoint(App::DocumentObject* joint);
    std::shared_ptr<MbD::ASMTJoint> makeMbdJointOfType(App::DocumentObject* joint,
                                                       JointType jointType);
    std::shared_ptr<MbD::ASMTJoint> makeMbdJointDistance(App::DocumentObject* joint);
    std::string handleOneSideOfJoint(App::DocumentObject* joint,
                                     const char* propRefName,
                                     const char* propPlcName);
    void getRackPinionMarkers(App::DocumentObject* joint,
                              std::string& markerNameI,
                              std::string& markerNameJ);
    int slidingPartIndex(App::DocumentObject* joint);

    void jointParts(std::vector<App::DocumentObject*> joints);
    JointGroup* getJointGroup() const;
    ViewGroup* getExplodedViewGroup() const;
    std::vector<App::DocumentObject*>
    getJoints(bool updateJCS = true, bool delBadJoints = false, bool subJoints = true);
    std::vector<App::DocumentObject*> getGroundedJoints();
    std::vector<App::DocumentObject*> getJointsOfObj(App::DocumentObject* obj);
    std::vector<App::DocumentObject*> getJointsOfPart(App::DocumentObject* part);
    App::DocumentObject* getJointOfPartConnectingToGround(App::DocumentObject* part,
                                                          std::string& name);
    std::vector<App::DocumentObject*> getGroundedParts();
    std::vector<App::DocumentObject*> fixGroundedParts();
    void fixGroundedPart(App::DocumentObject* obj, Base::Placement& plc, std::string& jointName);

    bool isJointConnectingPartToGround(App::DocumentObject* joint, const char* partPropName);
    bool isJointTypeConnecting(App::DocumentObject* joint);

    bool isObjInSetOfObjRefs(App::DocumentObject* obj, const std::vector<ObjRef>& pairs);
    void removeUnconnectedJoints(std::vector<App::DocumentObject*>& joints,
                                 std::vector<App::DocumentObject*> groundedObjs);
    void traverseAndMarkConnectedParts(App::DocumentObject* currentPart,
                                       std::vector<ObjRef>& connectedParts,
                                       const std::vector<App::DocumentObject*>& joints);
    std::vector<ObjRef> getConnectedParts(App::DocumentObject* part,
                                          const std::vector<App::DocumentObject*>& joints);
    bool isPartGrounded(App::DocumentObject* part);
    bool isPartConnected(App::DocumentObject* part);

    std::vector<ObjRef> getDownstreamParts(App::DocumentObject* part,
                                           App::DocumentObject* joint = nullptr);
    std::vector<App::DocumentObject*> getUpstreamParts(App::DocumentObject* part, int limit = 0);
    App::DocumentObject* getUpstreamMovingPart(App::DocumentObject* part,
                                               App::DocumentObject*& joint,
                                               std::string& name);

    double getObjMass(App::DocumentObject* obj);
    void setObjMasses(std::vector<std::pair<App::DocumentObject*, double>> objectMasses);

    std::vector<AssemblyLink*> getSubAssemblies();
    void updateGroundedJointsPlacements();

private:
    std::shared_ptr<MbD::ASMTAssembly> mbdAssembly;

    std::unordered_map<App::DocumentObject*, MbDPartData> objectPartMap;
    std::vector<std::pair<App::DocumentObject*, double>> objMasses;
    std::vector<App::DocumentObject*> draggedParts;

    std::vector<std::pair<App::DocumentObject*, Base::Placement>> previousPositions;

    bool bundleFixed;
    // void handleChangedPropertyType(Base::XMLReader &reader, const char *TypeName, App::Property
    // *prop) override;

public:
    // ---------------- Utils -------------------
    // Can't put the functions by themselves in AssemblyUtils.cpp :
    // see https://forum.freecad.org/viewtopic.php?p=729577#p729577

    static void swapJCS(App::DocumentObject* joint);

    static bool isEdgeType(App::DocumentObject* obj, std::string& elName, GeomAbs_CurveType type);
    static bool isFaceType(App::DocumentObject* obj, std::string& elName, GeomAbs_SurfaceType type);
    static double getFaceRadius(App::DocumentObject* obj, std::string& elName);
    static double getEdgeRadius(App::DocumentObject* obj, std::string& elName);

    static DistanceType getDistanceType(App::DocumentObject* joint);

    static JointGroup* getJointGroup(const App::Part* part);

    // getters to get from properties
    static void setJointActivated(App::DocumentObject* joint, bool val);
    static bool getJointActivated(App::DocumentObject* joint);
    static double getJointDistance(App::DocumentObject* joint);
    static double getJointDistance2(App::DocumentObject* joint);
    static JointType getJointType(App::DocumentObject* joint);
    static std::string getElementFromProp(App::DocumentObject* obj, const char* propName);
    static std::string getElementTypeFromProp(App::DocumentObject* obj, const char* propName);
    static App::DocumentObject* getObjFromProp(App::DocumentObject* joint, const char* propName);
    static App::DocumentObject* getObjFromRef(App::DocumentObject* obj, std::string& sub);
    static App::DocumentObject* getObjFromRef(App::PropertyXLinkSub* prop);
    static App::DocumentObject* getObjFromRef(App::DocumentObject* joint, const char* propName);
    App::DocumentObject* getMovingPartFromRef(App::DocumentObject* obj, std::string& sub);
    App::DocumentObject* getMovingPartFromRef(App::PropertyXLinkSub* prop);
    App::DocumentObject* getMovingPartFromRef(App::DocumentObject* joint, const char* propName);
    static App::DocumentObject* getLinkedObjFromRef(App::DocumentObject* joint,
                                                    const char* propName);
    static std::vector<std::string> getSubAsList(App::PropertyXLinkSub* prop);
    static std::vector<std::string> getSubAsList(App::DocumentObject* joint, const char* propName);
};

// using AssemblyObjectPython = App::FeaturePythonT<AssemblyObject>;

}  // namespace Assembly


#endif  // ASSEMBLY_AssemblyObject_H
