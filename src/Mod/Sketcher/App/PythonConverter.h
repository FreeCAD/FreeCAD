// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2022 Abdullah Tahiri
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/


#pragma once

#include <string>
#include <vector>

#include <Mod/Sketcher/SketcherGlobal.h>

namespace Part
{
class Geometry;
}

namespace Sketcher
{
class Constraint;

/** @brief      Class for generating python code
 *  @details
 *  Given C++ structures, it generates the python code that should be written in the console to
 *  create such objects.
 */

class SketcherExport PythonConverter
{
    class SingleGeometry
    {
    public:
        std::string creation;
        bool construction;
    };

public:
    enum class Mode
    {
        CreateInternalGeometry,
        OmitInternalGeometry
    };

    enum class GeoIdMode
    {
        DoNotChangeGeoIds,
        AddLastGeoIdToGeoIds,
    };

    PythonConverter() = delete;
    ~PythonConverter() = delete;

    /// Convert a geometry into the string representing the command creating it
    static std::string convert(const Part::Geometry* geo, Mode mode = Mode::CreateInternalGeometry);

    /// Convert a vector of geometries into the string representing the command creating them
    static std::string convert(
        const std::string& doc,
        const std::vector<Part::Geometry*>& geos,
        Mode mode = Mode::CreateInternalGeometry
    );

    static std::string convert(
        const Sketcher::Constraint* constraint,
        GeoIdMode geoIdMode = GeoIdMode::DoNotChangeGeoIds
    );

    static std::string convert(
        const std::string& doc,
        const std::vector<Sketcher::Constraint*>& constraints,
        GeoIdMode geoIdMode = GeoIdMode::DoNotChangeGeoIds
    );

    static std::vector<std::string> multiLine(std::string&& singlestring);

private:
    static SingleGeometry process(const Part::Geometry* geo);

    static std::string process(
        const Sketcher::Constraint* constraint,
        GeoIdMode geoIdMode = GeoIdMode::DoNotChangeGeoIds
    );
};

}  // namespace Sketcher
