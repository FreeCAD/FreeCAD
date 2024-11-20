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


#ifndef ASSEMBLY_AssemblyUtils_H
#define ASSEMBLY_AssemblyUtils_H


#include <GeomAbs_CurveType.hxx>
#include <GeomAbs_SurfaceType.hxx>

#include <Mod/Assembly/AssemblyGlobal.h>

#include <App/FeaturePython.h>
#include <App/Part.h>

namespace App
{
class DocumentObject;
}  // namespace App

namespace Base
{
class Placement;
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

class AssemblyObject;
class JointGroup;

AssemblyExport void swapJCS(App::DocumentObject* joint);

AssemblyExport bool
isEdgeType(App::DocumentObject* obj, std::string& elName, GeomAbs_CurveType type);
AssemblyExport bool
isFaceType(App::DocumentObject* obj, std::string& elName, GeomAbs_SurfaceType type);
AssemblyExport double getFaceRadius(App::DocumentObject* obj, std::string& elName);
AssemblyExport double getEdgeRadius(App::DocumentObject* obj, std::string& elName);

AssemblyExport DistanceType getDistanceType(App::DocumentObject* joint);
AssemblyExport JointGroup* getJointGroup(const App::Part* part);

// getters to get from properties
AssemblyExport void setJointActivated(App::DocumentObject* joint, bool val);
AssemblyExport bool getJointActivated(App::DocumentObject* joint);
AssemblyExport double getJointDistance(App::DocumentObject* joint);
AssemblyExport double getJointDistance2(App::DocumentObject* joint);
AssemblyExport JointType getJointType(App::DocumentObject* joint);
AssemblyExport std::string getElementFromProp(App::DocumentObject* obj, const char* propName);
AssemblyExport std::string getElementTypeFromProp(App::DocumentObject* obj, const char* propName);
AssemblyExport App::DocumentObject* getObjFromProp(App::DocumentObject* joint,
                                                   const char* propName);
AssemblyExport App::DocumentObject* getObjFromRef(App::DocumentObject* obj, std::string& sub);
AssemblyExport App::DocumentObject* getObjFromRef(App::PropertyXLinkSub* prop);
AssemblyExport App::DocumentObject* getObjFromRef(App::DocumentObject* joint, const char* propName);
AssemblyExport App::DocumentObject* getLinkedObjFromRef(App::DocumentObject* joint,
                                                        const char* propName);
AssemblyExport App::DocumentObject*
getMovingPartFromRef(AssemblyObject* assemblyObject, App::DocumentObject* obj, std::string& sub);
AssemblyExport App::DocumentObject* getMovingPartFromRef(AssemblyObject* assemblyObject,
                                                         App::PropertyXLinkSub* prop);
AssemblyExport App::DocumentObject*
getMovingPartFromRef(AssemblyObject* assemblyObject, App::DocumentObject* joint, const char* pName);
AssemblyExport std::vector<std::string> getSubAsList(App::PropertyXLinkSub* prop);
AssemblyExport std::vector<std::string> getSubAsList(App::DocumentObject* joint,
                                                     const char* propName);

}  // namespace Assembly


#endif  // ASSEMBLY_AssemblyUtils_H
