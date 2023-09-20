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

namespace Base
{
class Placement;
class Rotation;
}  // namespace Base

namespace Assembly
{

// This enum has to be the same as the one in JointObject.py
enum class JointType
{
    Fixed,
    Revolute,
    Cylindrical,
    Slider,
    Ball,
    Planar,
    Parallel,
    Tangent
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

    int solve();
    void exportAsASMT(std::string fileName);
    std::shared_ptr<MbD::ASMTAssembly> makeMbdAssembly();
    std::shared_ptr<MbD::ASMTPart>
    makeMbdPart(std::string& name, Base::Placement plc = Base::Placement(), double mass = 1.0);
    std::shared_ptr<MbD::ASMTPart> getMbDPart(App::DocumentObject* obj);
    std::shared_ptr<MbD::ASMTMarker> makeMbdMarker(std::string& name, Base::Placement& plc);
    std::shared_ptr<MbD::ASMTJoint> makeMbdJoint(App::DocumentObject* joint);
    std::shared_ptr<MbD::ASMTJoint> makeMbdJointOfType(JointType jointType);
    std::string handleOneSideOfJoint(App::DocumentObject* joint,
                                     const char* propObjLinkName,
                                     const char* propPlcName);
    void fixGroundedPart(App::DocumentObject* obj, Base::Placement& plc, std::string& jointName);
    bool fixGroundedParts();
    void jointParts(std::vector<App::DocumentObject*> joints);
    std::vector<App::DocumentObject*> getJoints();
    Base::Placement getPlacementFromProp(App::DocumentObject* obj, const char* propName);
    void setNewPlacements();
    void recomputeJointPlacements(std::vector<App::DocumentObject*> joints);

    double getObjMass(App::DocumentObject* obj);
    void setObjMasses(std::vector<std::pair<App::DocumentObject*, double>> objectMasses);

private:
    std::shared_ptr<MbD::ASMTAssembly> mbdAssembly;

    std::unordered_map<App::DocumentObject*, std::shared_ptr<MbD::ASMTPart>> objectPartMap;
    std::vector<std::pair<App::DocumentObject*, double>> objMasses;

    // void handleChangedPropertyType(Base::XMLReader &reader, const char *TypeName, App::Property
    // *prop) override;
};

// using AssemblyObjectPython = App::FeaturePythonT<AssemblyObject>;

}  // namespace Assembly


#endif  // ASSEMBLY_AssemblyObject_H
