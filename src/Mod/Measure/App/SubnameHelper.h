// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2024 wandererfan <wandererfan@gmail.com>                 *
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

#include <Mod/Measure/MeasureGlobal.h>

#include <TopoDS_Shape.hxx>

#include <App/DocumentObject.h>
#include <App/DocumentObserver.h>
#include <Base/Placement.h>
#include <Base/Matrix.h>

#include <Mod/Part/App/TopoShape.h>

namespace Measure
{

//! a class to perform common operations on subelement names.
class MeasureExport SubnameHelper
{
public:
    static std::string getLastTerm(const std::string& inString);
    static std::string getFirstTerm(const std::string& inString);
    static std::string namesToLongSub(const std::vector<std::string>& pathElementNames);
    static std::string pruneLastTerm(const std::string& inString);
    static std::string pruneFirstTerm(const std::string& inString);
    static std::string removeGeometryTerm(const std::string& longSubname);
    static std::string pathToLongSub(std::list<App::DocumentObject*> path);
    static std::string removeTnpInfo(const std::string& inString);
};

}  // namespace Measure
