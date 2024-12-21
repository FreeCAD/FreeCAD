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

//! a class to perform common operations on subelement names.


#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include <boost_regex.hpp>

#include <Base/Tools.h>

#include "SubnameHelper.h"


using namespace Measure;


std::string SubnameHelper::pathToLongSub(std::list<App::DocumentObject*> path)
{
    std::vector<std::string> elementNames;
    for (auto& item : path) {
        auto name = item->getNameInDocument();
        if (!name) {
            continue;
        }
        elementNames.emplace_back(name);
    }
    return namesToLongSub(elementNames);
}


//! construct dot separated long subelement name from a list of elements.  the elements should be
//! in topological order.
std::string SubnameHelper::namesToLongSub(const std::vector<std::string>& pathElementNames)
{
    std::string result;
    for (auto& name : pathElementNames) {
        result += (name + ".");
    }
    return result;
}


//! return the last term of a dot separated string - A.B.C returns C
std::string SubnameHelper::getLastTerm(const std::string& inString)
{
    auto result {inString};
    size_t lastDot = inString.rfind('.');
    if (lastDot != std::string::npos) {
        result = result.substr(lastDot + 1);
    }
    return result;
}

//! return the first term of a dot separated string - A.B.C returns A
std::string SubnameHelper::getFirstTerm(const std::string& inString)
{
    auto result {inString};
    size_t lastDot = inString.find('.');
    if (lastDot != std::string::npos) {
        result = result.substr(0, lastDot);
    }
    return result;
}

//! remove the first term of a dot separated string - A.B.C returns B.C
std::string SubnameHelper::pruneFirstTerm(const std::string& inString)
{
    auto result {inString};
    size_t lastDot = inString.find('.');
    if (lastDot != std::string::npos) {
        result = result.substr(lastDot + 1);
    }
    return result;
}

//! return a dot separated string without its last term - A.B.C returns A.B.
// A.B.C. returns A.B.C
std::string SubnameHelper::pruneLastTerm(const std::string& inString)
{
    auto result {inString};
    if (result.back() == '.') {
        // remove the trailing dot
        result = result.substr(0, result.length() - 1);
    }

    size_t lastDotPos = result.rfind('.');
    if (lastDotPos != std::string::npos) {
        result = result.substr(0, lastDotPos + 1);
    }
    else {
        // no dot in string, remove everything!
        result = "";
    }

    return result;
}

//! remove that part of a long subelement name that refers to a geometric subshape.  "myObj.Vertex1"
//! would return "myObj.", "myObj.mySubObj." would return itself unchanged.  If there is no
//! geometric reference the original input is returned.
std::string SubnameHelper::removeGeometryTerm(const std::string& longSubname)
{
    auto lastTerm = getLastTerm(longSubname);
    if (longSubname.empty() || longSubname.back() == '.') {
        // not a geometric reference
        return longSubname;  // need a copy?
    }

    // brute force check for geometry names in the last term
    auto pos = lastTerm.find("Vertex");
    if (pos != std::string::npos) {
        return pruneLastTerm(longSubname);
    }

    pos = lastTerm.find("Edge");
    if (pos != std::string::npos) {
        return pruneLastTerm(longSubname);
    }

    pos = lastTerm.find("Face");
    if (pos != std::string::npos) {
        return pruneLastTerm(longSubname);
    }

    pos = lastTerm.find("Shell");
    if (pos != std::string::npos) {
        return pruneLastTerm(longSubname);
    }

    pos = lastTerm.find("Solid");
    if (pos != std::string::npos) {
        return pruneLastTerm(longSubname);
    }

    return longSubname;
}


//! remove the tnp information from a selection sub name returning a dot separated path
//! Array001.Array001_i0.Array_i1.;Vertex33;:H1116,V.Vertex33 to
//! Array001.Array001_i0.Array_i1.Vertex33
std::string SubnameHelper::removeTnpInfo(const std::string& inString)
{
    constexpr char TNPDelimiter {';'};
    size_t firstDelimiter = inString.find(TNPDelimiter);
    if (firstDelimiter == std::string::npos) {
        // no delimiter in string
        return inString;
    }
    auto geomName = getLastTerm(inString);
    auto path = inString.substr(0, firstDelimiter);
    auto result = path + geomName;
    return result;
}
