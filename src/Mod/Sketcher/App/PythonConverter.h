/***************************************************************************
 *   Copyright (c) 2022 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#ifndef SKETCHER_PythonConverter_H
#define SKETCHER_PythonConverter_H

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
    explicit PythonConverter() = delete;
    ~PythonConverter() = delete;

    /// Convert a geometry into the string representing the command creating it
    static std::string convert(const Part::Geometry* geo);

    /// Convert a vector of geometries into the string representing the command creating them
    static std::string convert(const std::string& doc, const std::vector<Part::Geometry*>& geos);

    static std::string convert(const Sketcher::Constraint* constraint);

    static std::string convert(const std::string& doc,
                               const std::vector<Sketcher::Constraint*>& constraints);

    static std::vector<std::string> multiLine(std::string&& singlestring);

private:
    static SingleGeometry process(const Part::Geometry* geo);

    static std::string process(const Sketcher::Constraint* constraint);
};

}  // namespace Sketcher


#endif  // SKETCHER_PythonConverter_H
