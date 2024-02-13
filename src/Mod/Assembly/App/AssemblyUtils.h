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
/*
// This enum has to be the same as the one in JointObject.py
enum class JointType
{
    Fixed,
    Revolute,
    Cylindrical,
    Slider,
    Ball,
    Distance
};

// getters to get from properties
double getJointDistance(App::DocumentObject* joint);
JointType getJointType(App::DocumentObject* joint);
const char* getElementFromProp(App::DocumentObject* obj, const char* propName);
std::string getElementTypeFromProp(App::DocumentObject* obj, const char* propName);
App::DocumentObject* getLinkObjFromProp(App::DocumentObject* joint, const char* propName);
App::DocumentObject* getObjFromNameProp(App::DocumentObject* joint, const char* pObjName, const
char* pPart); App::DocumentObject* getLinkedObjFromNameProp(App::DocumentObject* joint, const char*
pObjName, const char* pPart); Base::Placement getPlacementFromProp(App::DocumentObject* obj, const
char* propName);*/

}  // namespace Assembly


#endif  // ASSEMBLY_AssemblyUtils_H
